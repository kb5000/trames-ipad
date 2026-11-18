// SPDX-License-Identifier: GPL-3.0-or-later
//! Read-only platform memory metadata for configured auto mode. No reservation,
//! polling thread or additional allocation policy is introduced.
use std::sync::Arc;
use trames_linux::HostMemoryInfo;
pub fn source() -> Option<Arc<dyn Fn() -> HostMemoryInfo + Send + Sync>> {
    #[cfg(target_vendor = "apple")]
    {
        let total = apple::physical_bytes().ok()?;
        Some(Arc::new(move || apple::snapshot(total)))
    }
    #[cfg(not(target_vendor = "apple"))]
    {
        Some(Arc::new(|| {
            let mut info: libc::sysinfo = unsafe { std::mem::zeroed() };
            // SAFETY: writable, complete local SDK structure; no guest pointer.
            if unsafe { libc::sysinfo(&mut info) } != 0 {
                return HostMemoryInfo::default();
            }
            let unit = (info.mem_unit as usize).max(1);
            let total = (info.totalram as usize).saturating_mul(unit);
            let free = (info.freeram as usize).saturating_mul(unit).min(total);
            HostMemoryInfo {
                total,
                free,
                available: free
                    .saturating_add((info.bufferram as usize).saturating_mul(unit))
                    .min(total),
            }
        }))
    }
}
#[cfg(target_vendor = "apple")]
mod apple {
    use trames_linux::HostMemoryInfo;
    // SDK Mach ABI. libc deprecates its convenience accessors, not these OS APIs.
    unsafe extern "C" {
        fn mach_host_self() -> libc::mach_port_t;
        fn mach_port_deallocate(
            task: libc::mach_port_t,
            name: libc::mach_port_t,
        ) -> libc::kern_return_t;
        static mach_task_self_: libc::mach_port_t;
    }

    pub fn physical_bytes() -> std::io::Result<usize> {
        let mut bytes = 0u64;
        let mut len = std::mem::size_of::<u64>();
        // SAFETY: exact initialized output/length for the hw.memsize scalar.
        if unsafe {
            libc::sysctlbyname(
                c"hw.memsize".as_ptr(),
                (&mut bytes as *mut u64).cast(),
                &mut len,
                std::ptr::null_mut(),
                0,
            )
        } != 0
        {
            return Err(std::io::Error::last_os_error());
        }
        if len != std::mem::size_of::<u64>() || bytes == 0 {
            return Err(std::io::Error::other("invalid host physical memory size"));
        }
        usize::try_from(bytes).map_err(std::io::Error::other)
    }

    pub fn snapshot(total: usize) -> HostMemoryInfo {
        // A failed metadata query is conservative; it cannot impose an allocation
        // limit. The physical size was validated once when creating the session.
        let mut result = HostMemoryInfo {
            total,
            free: 0,
            available: 0,
        };
        let mut stats: libc::vm_statistics64 = unsafe { std::mem::zeroed() };
        let mut count = libc::HOST_VM_INFO64_COUNT;
        // SAFETY: local complete zeroed structure, SDK-sized output and temporary
        // host send right, deallocated on both success and failure. No guest pointer.
        let status = unsafe {
            let host = mach_host_self();
            let status = libc::host_statistics64(
                host,
                libc::HOST_VM_INFO64,
                (&mut stats as *mut libc::vm_statistics64).cast(),
                &mut count,
            );
            mach_port_deallocate(mach_task_self_, host);
            status
        };
        if status == libc::KERN_SUCCESS {
            let page = unsafe { libc::sysconf(libc::_SC_PAGESIZE) }.max(0) as usize;
            result.free = (stats.free_count as usize).saturating_mul(page).min(total);
            // SDK vm_statistics.h: speculative pages are already in free_count.
            // Purgeable pages can overlap the queues too. Count neither twice.
            // Free + inactive is an availability estimate, not Linux's full heuristic.
            result.available = (stats.free_count as usize)
                .saturating_add(stats.inactive_count as usize)
                .saturating_mul(page)
                .min(total);
        }
        #[cfg(all(target_os = "ios", not(target_abi = "sim")))]
        {
            unsafe extern "C" {
                fn os_proc_available_memory() -> usize;
            }
            // Public iOS accounting includes this process's current system
            // allowance. This is a report, never an extra Trames hard limit.
            result.available = unsafe { os_proc_available_memory() }.min(total);
            result.free = result.free.min(result.available);
        }
        result
    }
}
#[cfg(test)]
mod tests {
    #[test]
    fn live_accounting_has_real_capacity_without_reserving_guest_ram() {
        let read = super::source().unwrap();
        let value = read();
        assert!(value.total > 0);
        assert!(value.free <= value.available);
        assert!(value.available <= value.total);
    }
}
