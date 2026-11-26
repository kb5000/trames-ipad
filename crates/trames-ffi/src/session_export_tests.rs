// SPDX-License-Identifier: GPL-3.0-or-later
//! Native output handles survive waits; cancellation checks output identity.
use super::super::memory_tests::{disk, run_host, test_adapter};
use super::*;

struct Temp(PathBuf);
impl Temp {
    fn new() -> Self {
        let path = std::env::temp_dir().join(format!(
            "trames-live-export-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        fs::create_dir(&path).unwrap();
        Self(path)
    }
}
impl Drop for Temp {
    fn drop(&mut self) {
        let _ = fs::remove_dir_all(&self.0);
    }
}
fn write(v: &mut Vfs, path: &[u8], bytes: &[u8]) {
    let fd = v.open_at(trames_vfs::AT_FDCWD, path, 2, 0).unwrap();
    assert_eq!(v.write(fd, bytes).unwrap(), bytes.len());
    v.close(fd).unwrap();
}
fn setup(temp: &Temp) -> (Vfs, Vec<u8>) {
    let mut v = Vfs::open_disk(temp.0.join("store"), Default::default()).unwrap();
    v.set_runtime_info(1, Default::default());
    let bytes = [vec![7; 4096], vec![9; 4096]].concat();
    v.import_file(b"/data", bytes.clone()).unwrap();
    write(&mut v, b"/sys/fs/cgroup/cgroup.subtree_control", b"+io");
    v.mkdir_at(trames_vfs::AT_FDCWD, b"/sys/fs/cgroup/a", 0o755)
        .unwrap();
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 riops=1");
    write(&mut v, b"/sys/fs/cgroup/a/cgroup.procs", b"1");
    (v, bytes)
}
fn enqueue(
    adapter: &mut Adapter,
    host: PathBuf,
    token: u64,
    cancelled: Arc<AtomicBool>,
) -> mpsc::Receiver<Result<u64, String>> {
    let (response, result) = mpsc::sync_channel(1);
    adapter
        .session
        .commands
        .send(Command::Transfer(Transfer {
            options: TransferOptions {
                guest: "/data".into(),
                host,
                export: true,
                replace: false,
                expected: None,
                token,
                maximum_bytes: Some(8192),
            },
            result: response,
            cancelled,
        }))
        .unwrap();
    result
}
#[test]
fn live_export_resumes_original_guest_and_native_files_after_both_paths_are_replaced() {
    let temp = Temp::new();
    let (mut v, bytes) = setup(&temp);
    let mut adapter = test_adapter(None);
    let queue = disk(&mut adapter);
    let output = temp.0.join("output");
    let accepted = temp.0.join("accepted-output");
    let response = enqueue(
        &mut adapter,
        output.clone(),
        0,
        Arc::new(AtomicBool::new(false)),
    );
    adapter.progress_commands(&v).unwrap();
    run_host(&adapter, &queue);
    assert!(matches!(
        response.try_recv(),
        Err(mpsc::TryRecvError::Empty)
    ));
    assert_eq!(fs::read(&output).unwrap(), bytes[..4096]);
    assert!(adapter.host_files.as_ref().unwrap().deadline().is_some());
    fs::rename(&output, &accepted).unwrap();
    fs::write(&output, b"another native inode").unwrap();
    v.rename_at(
        trames_vfs::AT_FDCWD,
        b"/data",
        trames_vfs::AT_FDCWD,
        b"/accepted",
        0,
    )
    .unwrap();
    write(&mut v, b"/sys/fs/cgroup/cgroup.procs", b"1");
    v.import_file(b"/data", b"another guest inode".to_vec())
        .unwrap();
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 riops=max");
    run_host(&adapter, &queue);
    assert_eq!(response.try_recv().unwrap().unwrap(), 0);
    assert_eq!(fs::read(&accepted).unwrap(), bytes);
    assert_eq!(fs::read(&output).unwrap(), b"another native inode");
    assert!(adapter
        .host_active
        .as_ref()
        .unwrap()
        .load(Ordering::Acquire));
    assert!(!adapter.host_files.as_ref().unwrap().pending());
}
#[test]
fn export_cancellation_closes_original_output_and_preserves_a_replaced_native_name() {
    for (close, replaced) in [(false, false), (true, false), (false, true), (true, true)] {
        let temp = Temp::new();
        let (v, bytes) = setup(&temp);
        let mut adapter = test_adapter(None);
        let queue = disk(&mut adapter);
        let output = temp.0.join("output");
        let accepted = temp.0.join("accepted-output");
        let token = adapter.session.begin_transfer().unwrap();
        let cancelled = adapter.session.transfers.lock().unwrap()[&token]
            .cancelled
            .clone();
        let response = enqueue(&mut adapter, output.clone(), token, cancelled);
        adapter.progress_commands(&v).unwrap();
        run_host(&adapter, &queue);
        assert_eq!(fs::read(&output).unwrap(), bytes[..4096]);
        if replaced {
            fs::rename(&output, &accepted).unwrap();
            fs::write(&output, b"replacement survives").unwrap();
        }
        if close {
            adapter.session.cancelled.store(true, Ordering::Release);
            adapter.session.console_readiness.notify_locked();
        } else {
            adapter.session.cancel_transfer(token).unwrap();
        }
        run_host(&adapter, &queue);
        assert!(response
            .try_recv()
            .unwrap()
            .unwrap_err()
            .contains("cancelled"));
        if replaced {
            assert_eq!(fs::read(&output).unwrap(), b"replacement survives");
            assert_eq!(fs::read(&accepted).unwrap(), bytes[..4096]);
        } else {
            assert!(!output.exists());
        }
        assert!(adapter
            .host_active
            .as_ref()
            .unwrap()
            .load(Ordering::Acquire));
        assert!(!adapter.host_files.as_ref().unwrap().pending());
    }
}
