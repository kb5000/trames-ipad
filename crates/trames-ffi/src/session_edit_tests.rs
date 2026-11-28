// SPDX-License-Identifier: GPL-3.0-or-later
//! Real editor commands preserve their staged source and original comparison.
use super::super::memory_tests::{disk, run_host, test_adapter};
use super::*;

struct Temp(PathBuf);
impl Temp {
    fn new() -> Self {
        let path = std::env::temp_dir().join(format!(
            "trames-live-edit-{}-{}",
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
fn read(v: &mut Vfs, path: &[u8]) -> Vec<u8> {
    let fd = v.open_at(trames_vfs::AT_FDCWD, path, 0, 0).unwrap();
    let bytes = v.pread(fd, 1 << 20, 0).unwrap();
    v.close(fd).unwrap();
    bytes
}
fn setup(temp: &Temp) -> (Vfs, Vec<u8>, Vec<u8>, TransferVersion) {
    let mut v = Vfs::open_disk(temp.0.join("store"), Default::default()).unwrap();
    v.set_runtime_info(1, Default::default());
    let original = [vec![1; 4096], vec![2; 4096], vec![3; 4096]].concat();
    let replacement = [vec![7; 65536], vec![9; 65536]].concat();
    v.import_file(b"/data", original.clone()).unwrap();
    let expected = TransferVersion {
        inode: file_inode(&v, b"/data").unwrap(),
        sha256: format!("{:x}", Sha256::digest(&original)),
        replacement_sha256: format!("{:x}", Sha256::digest(&replacement)),
    };
    write(&mut v, b"/sys/fs/cgroup/cgroup.subtree_control", b"+io");
    v.mkdir_at(trames_vfs::AT_FDCWD, b"/sys/fs/cgroup/a", 0o755)
        .unwrap();
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 riops=1 wiops=1");
    write(&mut v, b"/sys/fs/cgroup/a/cgroup.procs", b"1");
    (v, original, replacement, expected)
}
fn enqueue(
    adapter: &mut Adapter,
    host: PathBuf,
    expected: TransferVersion,
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
                expected: Some(expected),
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
fn live_edit_waits_for_staging_and_validation_and_uses_the_once_opened_native_source() {
    let temp = Temp::new();
    let (mut v, _, replacement, expected) = setup(&temp);
    let journal_before = fs::metadata(temp.0.join("store/manifest.wal"))
        .unwrap()
        .len();
    let old_inode = expected.inode;
    let source = temp.0.join("source");
    fs::write(&source, &replacement).unwrap();
    let mut adapter = test_adapter(None);
    let queue = disk(&mut adapter);
    let response = enqueue(
        &mut adapter,
        source.clone(),
        expected,
        0,
        Arc::new(AtomicBool::new(false)),
    );
    adapter.progress_commands(&v).unwrap();
    run_host(&adapter, &queue);
    assert!(matches!(
        response.try_recv(),
        Err(mpsc::TryRecvError::Empty)
    ));
    assert_eq!(file_inode(&v, b"/data").unwrap(), old_inode);
    fs::rename(&source, temp.0.join("accepted-source")).unwrap();
    fs::write(&source, b"unrelated replacement at native path").unwrap();
    write(&mut v, b"/sys/fs/cgroup/cgroup.procs", b"1");
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 wiops=max");
    run_host(&adapter, &queue);
    assert!(matches!(
        response.try_recv(),
        Err(mpsc::TryRecvError::Empty)
    ));
    assert_eq!(file_inode(&v, b"/data").unwrap(), old_inode);
    assert!(adapter.host_files.as_ref().unwrap().deadline().is_some());
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 riops=max");
    run_host(&adapter, &queue);
    assert_ne!(response.try_recv().unwrap().unwrap(), old_inode);
    assert_eq!(read(&mut v, b"/data"), replacement);
    assert_eq!(
        fs::read(&source).unwrap(),
        b"unrelated replacement at native path"
    );
    let stats = String::from_utf8(read(&mut v, b"/sys/fs/cgroup/a/io.stat")).unwrap();
    for field in ["rios=3", "rbytes=12288", "wios=3"] {
        assert!(
            stats.split_whitespace().any(|word| word == field),
            "{stats}"
        );
    }
    let written = replacement.len() as u64
        + fs::metadata(temp.0.join("store/manifest.wal"))
            .unwrap()
            .len()
        - journal_before;
    assert!(
        stats
            .split_whitespace()
            .any(|word| word == format!("wbytes={written}")),
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
fn cancelling_staging_or_validation_releases_the_copy_and_original_file_transaction() {
    for (close, validation) in [(false, false), (true, false), (false, true), (true, true)] {
        let temp = Temp::new();
        let (mut v, original, replacement, expected) = setup(&temp);
        let baseline = v.capacity().used_bytes;
        if validation {
            write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 wiops=max");
        }
        let source = temp.0.join("source");
        fs::write(&source, &replacement).unwrap();
        let mut adapter = test_adapter(None);
        let queue = disk(&mut adapter);
        let token = adapter.session.begin_transfer().unwrap();
        let cancelled = adapter.session.transfers.lock().unwrap()[&token]
            .cancelled
            .clone();
        let response = enqueue(&mut adapter, source.clone(), expected, token, cancelled);
        adapter.progress_commands(&v).unwrap();
        run_host(&adapter, &queue);
        assert!(v.capacity().used_bytes > baseline);
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
        assert_eq!(read(&mut v, b"/data"), original);
        assert_eq!(v.capacity().used_bytes, baseline);
        assert_eq!(
            adapter.session.buffers.lock().unwrap().status.used_bytes,
            baseline
        );
        assert_eq!(fs::read(&source).unwrap(), replacement);
        assert!(!adapter.host_files.as_ref().unwrap().pending());
        // The comparison lease is gone; a later ordinary writer can proceed.
        write(&mut v, b"/sys/fs/cgroup/cgroup.procs", b"1");
        write(&mut v, b"/data", b"later");
    }
}
#[test]
fn a_competing_guest_path_is_retained_when_the_original_checksum_finishes() {
    let temp = Temp::new();
    let (mut v, original, replacement, expected) = setup(&temp);
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 wiops=max");
    let source = temp.0.join("source");
    fs::write(&source, &replacement).unwrap();
    let mut adapter = test_adapter(None);
    let queue = disk(&mut adapter);
    let response = enqueue(
        &mut adapter,
        source.clone(),
        expected,
        0,
        Arc::new(AtomicBool::new(false)),
    );
    adapter.progress_commands(&v).unwrap();
    run_host(&adapter, &queue);
    v.rename_at(
        trames_vfs::AT_FDCWD,
        b"/data",
        trames_vfs::AT_FDCWD,
        b"/old",
        0,
    )
    .unwrap();
    write(&mut v, b"/sys/fs/cgroup/cgroup.procs", b"1");
    v.import_file(b"/data", b"competing guest inode".to_vec())
        .unwrap();
    let with_stage = v.capacity().used_bytes;
    write(&mut v, b"/sys/fs/cgroup/a/io.max", b"253:1 riops=max");
    run_host(&adapter, &queue);
    assert!(response
        .try_recv()
        .unwrap()
        .unwrap_err()
        .contains("changed since it was opened"));
    assert_eq!(read(&mut v, b"/data"), b"competing guest inode");
    assert_eq!(read(&mut v, b"/old"), original);
    assert_eq!(fs::read(&source).unwrap(), replacement);
    assert_eq!(
        v.capacity().used_bytes,
        with_stage - replacement.len() as u64
    );
    assert!(!adapter.host_files.as_ref().unwrap().pending());
}
