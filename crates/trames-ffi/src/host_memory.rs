// SPDX-License-Identifier: GPL-3.0-or-later
//! Read-only platform memory metadata for configured auto mode. No reservation,
//! polling thread or additional allocation policy is introduced.
use std::sync::Arc;
use trames_linux::HostMemoryInfo;

/// An explicit caller-thread diagnostic, including after the last session closes.
/// This is process accounting: an embedded guest shares the app's host process,
/// so these bytes must never be presented as guest RSS or added to guest RSS.
#[derive(serde::Serialize)]
pub struct ProcessMemory {
    pid: u32,
    resident_bytes: u64,
    physical_footprint_bytes: Option<u64>,
    runtime_workers: usize,
}

pub fn process() -> std::io::Result<ProcessMemory> {
    #[cfg(target_vendor = "apple")]
    let (resident_bytes, physical_footprint_bytes) = apple::process()?;
    #[cfg(not(target_vendor = "apple"))]
    let (resident_bytes, physical_footprint_bytes) = {
        let statm = std::fs::read_to_string("/proc/self/statm")?;
        let pages = statm
            .split_whitespace()
            .nth(1)
            .and_then(|s| s.parse::<u64>().ok())
            .ok_or_else(|| std::io::Error::other("invalid process resident size"))?;
        // SAFETY: sysconf has no pointer arguments.
        let page_size = unsafe { libc::sysconf(libc::_SC_PAGESIZE) };
        let resident = pages
            .checked_mul(u64::try_from(page_size).unwrap_or(0))
            .filter(|_| page_size > 0)
            .ok_or_else(|| std::io::Error::other("invalid host page size or resident size"))?;
        (resident, None)
    };
    Ok(ProcessMemory {
        pid: std::process::id(),
        resident_bytes,
        physical_footprint_bytes,
        runtime_workers: crate::session::worker_count(),
    })
}

/// Compute a conservative guest memory ceiling from the host process's current
/// available allowance so a runaway guest receives Linux ENOMEM instead of
/// driving the embedding iOS process into an abrupt Jetsam termination.
#[cfg_attr(not(all(target_os = "ios", not(target_abi = "sim"))), allow(dead_code))]
pub fn safe_guest_limit_for_available(available_bytes: usize) -> Option<usize> {
    const PAGE: usize = 4096;
    const MIB: usize = 1024 * 1024;
    const MIN_GUEST: usize = 64 * MIB;
    const MIN_RESERVE_NORMAL: usize = 256 * MIB;
    const MIN_RESERVE_TIGHT: usize = 128 * MIB;
    const FLOOR_RESERVE: usize = 64 * MIB;

    if available_bytes < MIN_GUEST + FLOOR_RESERVE {
        return None;
    }
    let reserve = if available_bytes >= 512 * MIB {
        (available_bytes / 4).max(MIN_RESERVE_NORMAL)
    } else {
        (available_bytes * 3 / 10).max(MIN_RESERVE_TIGHT)
    };
    let usable = available_bytes
        .saturating_sub(reserve)
        .max(MIN_GUEST)
        .min(available_bytes.saturating_sub(FLOOR_RESERVE));
    let aligned = usable & !(PAGE - 1);
    (aligned >= MIN_GUEST).then_some(aligned)
}

/// Clamp configured guest memory limits to a host safety cap when present.
pub fn clamp_guest_limits(
    limits: trames_core::MemoryLimits,
    safe_cap: Option<usize>,
) -> trames_core::MemoryLimits {
    let Some(cap) = safe_cap else {
        return limits;
    };
    trames_core::MemoryLimits {
        total: limits.total.min(cap),
        per_process: limits.per_process.min(cap),
    }
}

/// Return a conservative guest memory cap on physical iOS devices based on
/// `os_proc_available_memory()`. Returns `None` on non-iOS hosts and simulators.
pub fn ios_safe_guest_limit() -> Option<usize> {
    #[cfg(all(target_os = "ios", not(target_abi = "sim")))]
    {
        unsafe extern "C" {
            fn os_proc_available_memory() -> usize;
        }
        // SAFETY: public Darwin syscall returning this process's remaining
        // Jetsam memory headroom in bytes; takes no pointers.
        let available = unsafe { os_proc_available_memory() };
        safe_guest_limit_for_available(available)
    }
    #[cfg(not(all(target_os = "ios", not(target_abi = "sim"))))]
    {
        None
    }
}

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
    // Public mach/task_info.h TASK_VM_INFO rev1 prefix. Later SDK revisions
    // append fields; requesting this prefix supports both iOS and macOS without
    // macOS-only libproc APIs. Its resident_size and phys_footprint offsets are
    // 16 and 144 bytes, with a 38-natural_t input/output count.
    #[repr(C)]
    #[derive(Default)]
    struct TaskVmInfoRev1 {
        _virtual_size: u64,
        _region_count: i32,
        _page_size: i32,
        resident_size: u64,
        _resident_size_peak: u64,
        _sizes: [u64; 14],
        phys_footprint: u64,
    }
    const _: () = {
        assert!(std::mem::size_of::<TaskVmInfoRev1>() == 152);
        assert!(std::mem::offset_of!(TaskVmInfoRev1, phys_footprint) == 144);
    };
    // SDK Mach ABI. libc deprecates its convenience accessors, not these OS APIs.
    unsafe extern "C" {
        fn mach_host_self() -> libc::mach_port_t;
        fn mach_port_deallocate(
            task: libc::mach_port_t,
            name: libc::mach_port_t,
        ) -> libc::kern_return_t;
        static mach_task_self_: libc::mach_port_t;
    }

    pub fn process() -> std::io::Result<(u64, Option<u64>)> {
        let mut info = TaskVmInfoRev1::default();
        let expected = (std::mem::size_of::<TaskVmInfoRev1>()
            / std::mem::size_of::<libc::natural_t>())
            as libc::mach_msg_type_number_t;
        let mut count = expected;
        // SAFETY: the SDK's rev1-sized output and matching word count; the send
        // right names only this calling process and is not acquired or retained.
        let status = unsafe {
            libc::task_info(
                mach_task_self_,
                22, // TASK_VM_INFO
                (&mut info as *mut TaskVmInfoRev1).cast(),
                &mut count,
            )
        };
        if status != libc::KERN_SUCCESS || count < expected {
            return Err(std::io::Error::other(format!(
                "task_info(TASK_VM_INFO): status={status}, count={count}, expected={expected}"
            )));
        }
        Ok((info.resident_size, Some(info.phys_footprint)))
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
    fn process_accounting_queries_the_current_process() {
        let value = super::process().unwrap();
        assert_eq!(value.pid, std::process::id());
        assert!(value.resident_bytes > 0);
        #[cfg(target_vendor = "apple")]
        assert!(value
            .physical_footprint_bytes
            .is_some_and(|bytes| bytes > 0));
        #[cfg(not(target_vendor = "apple"))]
        assert_eq!(value.physical_footprint_bytes, None);
    }

    #[test]
    fn live_accounting_has_real_capacity_without_reserving_guest_ram() {
        let read = super::source().unwrap();
        let value = read();
        assert!(value.total > 0);
        assert!(value.free <= value.available);
        assert!(value.available <= value.total);
    }

    #[test]
    fn safe_guest_limit_preserves_host_headroom_and_page_alignment() {
        const MIB: usize = 1024 * 1024;
        const GIB: usize = 1024 * MIB;
        assert_eq!(super::safe_guest_limit_for_available(0), None);
        assert_eq!(super::safe_guest_limit_for_available(96 * MIB), None);

        // 4 GiB available -> keeps 1 GiB (25%) headroom, 3 GiB for guest.
        assert_eq!(super::safe_guest_limit_for_available(4 * GIB), Some(3 * GIB));

        // 768 MiB available -> keeps at least 256 MiB headroom, 512 MiB for guest.
        assert_eq!(
            super::safe_guest_limit_for_available(768 * MIB),
            Some(512 * MIB)
        );

        // 300 MiB available -> tight mode reserves 128 MiB, 172 MiB for guest.
        let tight = super::safe_guest_limit_for_available(300 * MIB).unwrap();
        assert_eq!(tight % 4096, 0);
        assert_eq!(tight, 172 * MIB);

        // Clamping caps both host_managed() and oversized manual budgets.
        let host_managed = super::clamp_guest_limits(
            trames_core::MemoryLimits::host_managed(),
            Some(2 * GIB),
        );
        assert_eq!(host_managed.total, 2 * GIB);
        assert_eq!(host_managed.per_process, 2 * GIB);

        let smaller = super::clamp_guest_limits(
            trames_core::MemoryLimits {
                total: 512 * MIB,
                per_process: 256 * MIB,
            },
            Some(2 * GIB),
        );
        assert_eq!(smaller.total, 512 * MIB);
        assert_eq!(smaller.per_process, 256 * MIB);
    }
}
