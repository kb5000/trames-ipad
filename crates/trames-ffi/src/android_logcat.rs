// SPDX-License-Identifier: GPL-3.0-or-later
//! Explicit, bounded logcat capture using the original host-spawn workers.
use std::sync::{mpsc, Arc};
use std::time::Duration;
use trames_linux::{HostProcess, HostSpawn, HOST_RUNNING};
use trames_vfs::{Vfs, AT_FDCWD, O_NOFOLLOW};

pub(crate) const PATH: &[u8] = b"/data/local/tmp/trames-logcat.txt";
const CAPTURE: &str = "printf '=== Android properties ===\\n'; \
    getprop ro.hw_timeout_multiplier; getprop sys.boot_completed; \
    getprop init.svc.surfaceflinger; getprop debug.hwui.renderer; \
    printf '=== Android input ===\\n'; dumpsys -t 5 input; \
    printf '=== Android displays ===\\n'; dumpsys -t 5 window displays; \
    printf '=== Android warning and crash logs ===\\n'; \
    exec /system/bin/logcat -d -b main -b system -b crash '*:W'";

#[derive(Default)]
pub(crate) struct LaunchQueue {
    closed: bool,
    pending: Option<HostSpawn>,
}
impl LaunchQueue {
    pub(crate) fn insert(&mut self, launch: HostSpawn) -> Result<(), String> {
        if self.closed || self.pending.is_some() {
            return Err("Android diagnostic source is closed or busy".into());
        }
        self.pending = Some(launch);
        Ok(())
    }
    pub(crate) fn take(&mut self) -> Option<HostSpawn> {
        self.pending.take()
    }
    pub(crate) fn close(&mut self) {
        self.closed = true;
        if let Some(launch) = self.pending.take() {
            launch.process.fail(125);
        }
    }
}

pub(crate) fn prepare(root: &Vfs) -> Result<HostSpawn, String> {
    let result = (|| {
        let mut child = root.clone();
        child.detach_file_table();
        child.set_lock_owner(0);
        child.close_all();
        child.set_controlling_terminal(None)?;
        child.chdir(b"/")?;
        child.open_at(AT_FDCWD, b"/dev/null", 0, 0)?;
        let output = child.open_at(AT_FDCWD, PATH, 1 | 0x40 | 0x200 | O_NOFOLLOW, 0o600)?;
        child.dup3(output, 2, 0)?;
        // Preserve execution faults before the diagnostic shell itself runs.
        // This is a non-consuming snapshot of the existing bounded kernel log.
        child.write(output, b"=== Trames kernel log ===\n")?;
        child.write(output, &root.syslog(3, trames_vfs::kernel_log_capacity())?)?;
        Ok::<_, trames_vfs::Errno>(child)
    })();
    Ok(HostSpawn {
        // Fixed host diagnostic only, with no interpolated guest/user input.
        // Filter before output so verbose codec/init messages cannot hide an
        // older graphics failure behind a raw last-N-message tail.
        argv: ["/system/bin/sh", "-c", CAPTURE]
            .into_iter()
            .map(|s| s.as_bytes().to_vec())
            .collect(),
        environment: trames_distro::android::environment()
            .into_iter()
            .map(String::into_bytes)
            .collect(),
        vfs: result.map_err(|e| e.to_string())?,
        process: Arc::new(HostProcess::default()),
    })
}

struct Completion(mpsc::SyncSender<()>);
impl trames_shared::IoWake for Completion {
    fn wake_io(&self, _: &Arc<trames_shared::IoSignal>) {
        let _ = self.0.try_send(());
    }
}
pub(crate) fn wait(process: &Arc<HostProcess>, timeout: Duration) -> Result<(), String> {
    let (sender, receiver) = mpsc::sync_channel(1);
    let wake: Arc<dyn trames_shared::IoWake> = Arc::new(Completion(sender));
    let signal = trames_shared::IoSignal::new(0, Arc::downgrade(&wake));
    let _subscription = process.completion().subscribe(&signal);
    if process.status() == HOST_RUNNING && receiver.recv_timeout(timeout).is_err() {
        process.signal(9);
        process.cancel();
        return Err("Android logcat capture timed out".into());
    }
    match process.status() {
        0 => Ok(()),
        status => Err(format!("Android logcat exited with wait status {status}")),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn capture_owns_descriptors_and_rejects_a_replaced_log_symlink() {
        let mut root = Vfs::default();
        for path in [b"/data".as_slice(), b"/data/local", b"/data/local/tmp"] {
            root.mkdir_at(AT_FDCWD, path, 0o755).unwrap();
        }
        let original = root.open_at(AT_FDCWD, b"/dev/null", 0, 0).unwrap();
        root.log(4, "original execution fault");
        let mut launch = prepare(&root).unwrap();
        assert_eq!(launch.argv[0], b"/system/bin/sh");
        launch.vfs.write(1, b"graphics error\n").unwrap();
        launch.vfs.write(2, b"stderr\n").unwrap();
        assert_eq!(root.read(original, 1).unwrap(), b"");
        let captured = root.open_at(AT_FDCWD, PATH, 0, 0).unwrap();
        let bytes = root.read(captured, 4096).unwrap();
        let text = String::from_utf8(bytes).unwrap();
        assert!(text.starts_with("=== Trames kernel log ===\n"));
        assert!(text.contains("original execution fault"));
        assert!(text.ends_with("graphics error\nstderr\n"));
        root.close(captured).unwrap();
        assert!(String::from_utf8(root.syslog(3, 4096).unwrap())
            .unwrap()
            .contains("original execution fault"));
        let process = launch.process.clone();
        let mut queue = LaunchQueue::default();
        queue.insert(launch).unwrap();
        queue.close();
        assert!(queue.take().is_none());
        assert!(wait(&process, Duration::ZERO).unwrap_err().contains("-125"));
        assert!(queue.insert(prepare(&root).unwrap()).is_err());
        root.unlink_at(AT_FDCWD, PATH, false).unwrap();
        root.symlink_at(b"/dev/null", AT_FDCWD, PATH).unwrap();
        assert!(prepare(&root).is_err());
    }
    #[test]
    fn completion_before_subscription_is_not_lost() {
        let process = Arc::new(HostProcess::default());
        process.fail(5);
        assert!(wait(&process, Duration::ZERO).unwrap_err().contains("-5"));
    }
    #[test]
    fn completion_notification_wakes_the_calling_waiter() {
        let process = Arc::new(HostProcess::default());
        let other = process.clone();
        let worker = std::thread::spawn(move || other.fail(9));
        assert!(wait(&process, Duration::from_secs(5))
            .unwrap_err()
            .contains("-9"));
        worker.join().unwrap();
    }
}
