// SPDX-License-Identifier: GPL-3.0-or-later
//! Live native file commands retain original input and cancellation readiness.
use super::super::memory_tests::{disk, run_host, test_adapter};
use super::*;

struct Temp(PathBuf);
impl Temp {
    fn new() -> Self {
        let path = std::env::temp_dir().join(format!(
            "trames-live-import-{}-{}",
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
fn read(v: &mut Vfs, path: &[u8], count: usize) -> Vec<u8> {
    let fd = v.open_at(trames_vfs::AT_FDCWD, path, 0, 0).unwrap();
    let bytes = v.pread(fd, count, 0).unwrap();
    v.close(fd).unwrap();
    bytes
}
fn setup(temp: &Temp) -> Vfs {
    let mut v = Vfs::open_disk(temp.0.join("store"), Default::default()).unwrap();
    v.set_runtime_info(1, Default::default());
    v.import_file(b"/data", b"old".to_vec()).unwrap();
    write(&mut v, b"/sys/fs/cgroup/cgroup.subtree_control", b"+io");
    v.mkdir_at(trames_vfs::AT_FDCWD, b"/sys/fs/cgroup/a", 0o755)
        .unwrap();
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 wiops=1");
    write(&mut v, b"/sys/fs/cgroup/a/cgroup.procs", b"1");
    v
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
                export: false,
                replace: true,
                expected: None,
                token,
                maximum_bytes: Some(131072),
            },
            result: response,
            cancelled,
        }))
        .unwrap();
    result
}
#[test]
fn live_import_wait_preserves_the_open_host_file_and_publishes_only_after_resume() {
    let temp = Temp::new();
    let mut v = setup(&temp);
    let mut adapter = test_adapter(None);
    let queue = disk(&mut adapter);
    let source = temp.0.join("source");
    let bytes = [vec![7; 65536], vec![9; 65536]].concat();
    fs::write(&source, &bytes).unwrap();
    let response = enqueue(
        &mut adapter,
        source.clone(),
        0,
        Arc::new(AtomicBool::new(false)),
    );
    adapter.progress_commands(&v).unwrap();
    run_host(&adapter, &queue);
    assert!(matches!(
        response.try_recv(),
        Err(mpsc::TryRecvError::Empty)
    ));
    assert_eq!(read(&mut v, b"/data", 3), b"old");
    assert!(!adapter
        .host_active
        .as_ref()
        .unwrap()
        .load(Ordering::Acquire));
    assert!(adapter.host_files.as_ref().unwrap().deadline().is_some());
    fs::rename(&source, temp.0.join("accepted-source")).unwrap();
    fs::write(&source, b"replacement host path").unwrap();
    write(&mut v, b"/sys/fs/cgroup/cgroup.procs", b"1");
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 wiops=max");
    run_host(&adapter, &queue);
    let inode = response.try_recv().unwrap().unwrap();
    assert_eq!(read(&mut v, b"/data", bytes.len()), bytes);
    let stat = v.stat_at(trames_vfs::AT_FDCWD, b"/data").unwrap();
    assert_eq!(u64::from_le_bytes(stat[8..16].try_into().unwrap()), inode);
    let stats = String::from_utf8(read(&mut v, b"/sys/fs/cgroup/a/io.stat", 4096)).unwrap();
    assert!(stats.split_whitespace().any(|field| field == "wios=3"));
    let written = bytes.len() as u64 + fs::metadata(temp.0.join("store/manifest")).unwrap().len();
    assert!(
        stats
            .split_whitespace()
            .any(|field| field == format!("wbytes={written}")),
        "{stats}"
    );
    assert!(adapter
        .host_active
        .as_ref()
        .unwrap()
        .load(Ordering::Acquire));
    assert!(!adapter.host_files.as_ref().unwrap().pending());
}
#[test]
fn transfer_or_session_cancellation_retires_a_parked_import_without_waiting_for_io_credit() {
    for close in [false, true] {
        let temp = Temp::new();
        let mut v = setup(&temp);
        let baseline = v.capacity().used_bytes;
        let mut adapter = test_adapter(None);
        let queue = disk(&mut adapter);
        let source = temp.0.join("source");
        fs::write(&source, vec![8; 131072]).unwrap();
        let token = adapter.session.begin_transfer().unwrap();
        let cancelled = adapter.session.transfers.lock().unwrap()[&token]
            .cancelled
            .clone();
        let response = enqueue(&mut adapter, source, token, cancelled);
        adapter.progress_commands(&v).unwrap();
        run_host(&adapter, &queue);
        assert_eq!(v.capacity().used_bytes, baseline + 65536);
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
        assert_eq!(read(&mut v, b"/data", 3), b"old");
        assert_eq!(v.capacity().used_bytes, baseline);
        assert_eq!(
            adapter.session.buffers.lock().unwrap().status.used_bytes,
            baseline
        );
        assert!(adapter
            .host_active
            .as_ref()
            .unwrap()
            .load(Ordering::Acquire));
        assert!(!adapter.host_files.as_ref().unwrap().pending());
    }
}
