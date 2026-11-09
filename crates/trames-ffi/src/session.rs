// SPDX-License-Identifier: GPL-3.0-or-later
#![deny(unsafe_op_in_unsafe_fn)]
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::BTreeMap,
    fs::{self, File, OpenOptions},
    io::{self, Read, Seek, Write},
    path::PathBuf,
    sync::{
        atomic::{AtomicBool, AtomicU64, AtomicUsize, Ordering},
        mpsc, Arc, Mutex,
    },
    time::{Duration, Instant},
};
use trames_core::{Runtime, RuntimeError, Terminal};
use trames_linux::{GuestOutput, OutputFd, TerminalEvent};
use trames_vfs::{DiskOptions, NetworkMode, Vfs, VolumeSpace};

pub const QUEUE_LIMIT: usize = trames_distro::terminals::QUEUE_LIMIT;
static WORKERS: AtomicUsize = AtomicUsize::new(0);
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Options {
    pub store: PathBuf,
    pub rootfs: Option<PathBuf>,
    #[serde(default = "default_distro")]
    pub distro: String,
    pub rootfs_sha256: Option<String>,
    #[serde(default = "default_init")]
    pub init: String,
    pub command: Option<Vec<String>>,
    pub debian_setup: Option<DebianSetup>,
    pub musl_libc: Option<PathBuf>,
    pub volume_total: u64,
    pub volume_available: u64,
    #[serde(default = "default_rows")]
    pub rows: u16,
    #[serde(default = "default_columns")]
    pub columns: u16,
    #[serde(default = "default_network")]
    pub network: String,
    pub budget_bytes: Option<u64>,
    pub memory_budget_bytes: Option<usize>,
    pub process_memory_budget_bytes: Option<usize>,
    #[serde(default)]
    pub pure_accelerator: bool,
    #[serde(default)]
    pub native_bsearch: bool,
    #[serde(default)]
    pub native_allocator: bool,
    #[serde(default)]
    pub vulkan_accelerator: bool,
    #[serde(default)]
    pub wayland: bool,
    pub xwayland_launcher: Option<PathBuf>,
    #[serde(default)]
    pub multiple_terminals: bool,
    pub vulkan_icd: Option<PathBuf>,
    pub timezone: Option<HostTimezone>,
    #[serde(default)]
    pub initially_paused: bool,
    pub store_owner_lock: Option<PathBuf>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct DebianSetup {
    pub packages: String,
    #[serde(default)]
    pub systemd: bool,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct HostTimezone {
    pub identifier: String,
    pub data: Vec<u8>,
}
enum Command {
    Transfer(Transfer),
    Timezone(HostTimezone, mpsc::SyncSender<Result<(), String>>),
}
pub fn install(options: Options) -> Result<(), String> {
    if !options.store.is_absolute() || options.store.exists() {
        return Err("installation requires an absent absolute staging directory".into());
    }
    let archive = options.rootfs.as_ref().ok_or("rootfs archive required")?;
    let mut vfs = open_store(&options)?;
    install_image(&options, archive, &mut vfs)?;
    Ok(())
}
fn default_distro() -> String {
    "alpine".into()
}
fn default_init() -> String {
    "shell".into()
}
fn install_image(
    options: &Options,
    archive: &std::path::Path,
    vfs: &mut Vfs,
) -> Result<(), String> {
    let mut reader = File::open(archive).map_err(|e| e.to_string())?;
    match options.distro.as_str() {
        "alpine" => {
            trames_distro::install_alpine(&mut reader, vfs).map_err(|e| e.to_string())?;
        }
        "debian" => {
            trames_distro::install_debian(&mut reader, vfs).map_err(|e| e.to_string())?;
        }
        "custom" => {
            trames_distro::rootfs::install(archive, vfs, options.rootfs_sha256.as_deref())
                .map_err(|e| e.to_string())?;
        }
        _ => return Err("unsupported distribution".into()),
    }
    trames_distro::rootfs::validate_command(vfs, &["/bin/sh".into()]).map_err(|e| e.to_string())?;
    trames_distro::systemd::renew_identity(vfs).map_err(|e| e.to_string())?;
    if let Some(libc) = &options.musl_libc {
        if options.distro == "alpine" {
            trames_distro::install_musl(&mut File::open(libc).map_err(|e| e.to_string())?, vfs)
                .map_err(|e| e.to_string())?;
        }
    }
    Ok(())
}
fn memory_limits(options: &Options) -> Result<trames_core::MemoryLimits, String> {
    let defaults = trames_core::MemoryLimits::default();
    let limits = trames_core::MemoryLimits {
        total: options.memory_budget_bytes.unwrap_or(defaults.total),
        per_process: options
            .process_memory_budget_bytes
            .unwrap_or(defaults.per_process),
    };
    limits.validate().map_err(|e| e.to_string())?;
    Ok(limits)
}
fn default_rows() -> u16 {
    24
}

fn default_columns() -> u16 {
    80
}
fn default_network() -> String {
    "full".into()
}
#[derive(Clone, Serialize)]
pub struct Status {
    pub phase: &'static str,
    pub error: Option<String>,
    pub exit_code: Option<u8>,
    pub budget_bytes: u64,
    pub used_bytes: u64,
    pub available_bytes: u64,
    pub volume_total_bytes: u64,
    pub node_limit: usize,
    pub nodes_used: usize,
}
pub struct Buffers {
    pub(crate) terminals: BTreeMap<u64, crate::terminals::Channel>,
    pub status: Status,
}
pub struct Session {
    pub display: trames_core::VulkanDisplay,
    pub wayland: Mutex<Option<trames_wayland::Server>>,
    pub bridge: trames_bridge::Broker,
    pub buffers: Mutex<Buffers>,
    pub console_revision: [AtomicU64; 2],
    pub console_readiness: Arc<trames_vfs::Readiness>,
    pub cancelled: AtomicBool,
    pub paused: AtomicBool,
    pub multiple_terminals: bool,
    pub next_terminal: AtomicU64,
    commands: mpsc::SyncSender<Command>,
    transfers: Mutex<BTreeMap<u64, TransferTicket>>,
    next_transfer: AtomicU64,
}
impl trames_vfs::ReadinessDriver for Session {
    fn notification_only(&self) -> bool {
        true
    }
    fn attach(&self, _: Arc<trames_vfs::HostEvents>) -> io::Result<()> {
        Ok(())
    }
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct TransferOptions {
    pub guest: String,
    pub host: PathBuf,
    pub export: bool,
    #[serde(default)]
    pub token: u64,
    pub maximum_bytes: Option<u64>,
    #[serde(default)]
    pub replace: bool,
    pub expected: Option<TransferVersion>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct TransferVersion {
    pub inode: u64,
    pub sha256: String,
    pub replacement_sha256: String,
}
struct Transfer {
    options: TransferOptions,
    result: mpsc::SyncSender<Result<u64, String>>,
    cancelled: Arc<AtomicBool>,
}
struct TransferTicket {
    cancelled: Arc<AtomicBool>,
    claimed: bool,
}
struct WorkerPermit;
impl Drop for WorkerPermit {
    fn drop(&mut self) {
        WORKERS.fetch_sub(1, Ordering::AcqRel);
    }
}
impl Session {
    pub fn start(options: Options) -> Result<Arc<Self>, String> {
        if !options.store.is_absolute()
            || options.store.as_os_str().len() > 4096
            || options
                .rootfs
                .as_ref()
                .is_some_and(|p| !p.is_absolute() || p.as_os_str().len() > 4096)
            || options
                .musl_libc
                .as_ref()
                .is_some_and(|p| !p.is_absolute() || p.as_os_str().len() > 4096)
            || options
                .store_owner_lock
                .as_ref()
                .is_some_and(|p| !p.is_absolute() || p.as_os_str().len() > 4096)
            || options.rows == 0
            || options.columns == 0
            || options.rows > 4096
            || options.columns > 4096
            || options.volume_total == 0
            || options.volume_available > options.volume_total
            || !matches!(
                options.network.as_str(),
                "off" | "loopback" | "outbound" | "full"
            )
        {
            return Err("invalid session configuration".into());
        }
        memory_limits(&options)?;
        WORKERS
            .try_update(Ordering::AcqRel, Ordering::Acquire, |n| {
                (n < 8).then_some(n + 1)
            })
            .map_err(|_| "session worker limit reached")?;
        let permit = WorkerPermit;
        let (commands, receiver) = mpsc::sync_channel(8);
        let session = Arc::new(Self {
            console_revision: Default::default(),
            console_readiness: Arc::default(),
            display: trames_core::VulkanDisplay::default(),
            wayland: Mutex::new(None),
            bridge: trames_bridge::Broker::default(),
            buffers: Mutex::new(Buffers {
                terminals: BTreeMap::from([(
                    1,
                    crate::terminals::Channel {
                        size: Some((options.rows, options.columns)),
                        ..Default::default()
                    },
                )]),
                status: Status {
                    phase: "starting",
                    error: None,
                    exit_code: None,
                    budget_bytes: 0,
                    used_bytes: 0,
                    available_bytes: 0,
                    volume_total_bytes: options.volume_total,
                    node_limit: 0,
                    nodes_used: 0,
                },
            }),
            cancelled: AtomicBool::new(false),
            paused: AtomicBool::new(options.initially_paused),
            multiple_terminals: options.multiple_terminals,
            next_terminal: AtomicU64::new(2),
            commands,
            transfers: Mutex::new(BTreeMap::new()),
            next_transfer: AtomicU64::new(1),
        });
        let driver: Arc<dyn trames_vfs::ReadinessDriver> = session.clone();
        session
            .console_readiness
            .set_driver(Arc::downgrade(&driver));
        let worker_session = session.clone();
        std::thread::Builder::new()
            .name("trames-local".into())
            .spawn(move || {
                let _permit = permit;
                let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                    run(options, worker_session.clone(), receiver)
                }));
                let mut buffers = worker_session
                    .buffers
                    .lock()
                    .unwrap_or_else(|p| p.into_inner());
                match result {
                    Ok(Ok(code)) => {
                        buffers.status.phase = "exited";
                        buffers.status.exit_code = Some(code);
                    }
                    _ if worker_session.cancelled.load(Ordering::Acquire) => {
                        buffers.status.phase = "cancelled"
                    }
                    Ok(Err(error)) => {
                        buffers.status.phase = "failed";
                        buffers.status.error = Some(error.chars().take(4096).collect());
                    }
                    Err(_) => {
                        buffers.status.phase = "failed";
                        buffers.status.error = Some("local runtime panicked".into());
                    }
                }
            })
            .map_err(|e| e.to_string())?;
        Ok(session)
    }
    pub fn begin_transfer(&self) -> Result<u64, String> {
        let mut transfers = self.transfers.lock().map_err(|_| "transfer lock failed")?;
        if transfers.len() >= 8 {
            return Err("transfer limit reached".into());
        }
        let token = self
            .next_transfer
            .try_update(Ordering::AcqRel, Ordering::Acquire, |n| n.checked_add(1))
            .map_err(|_| "transfer tokens exhausted")?;
        transfers.insert(
            token,
            TransferTicket {
                cancelled: Arc::new(AtomicBool::new(false)),
                claimed: false,
            },
        );
        Ok(token)
    }
    pub fn synchronize_timezone(&self, timezone: HostTimezone) -> Result<(), String> {
        let (response, receiver) = mpsc::sync_channel(1);
        self.commands
            .try_send(Command::Timezone(timezone, response))
            .map_err(|_| "local session is busy or closed")?;
        loop {
            if self.cancelled.load(Ordering::Acquire) {
                return Err("session cancelled".into());
            }
            match receiver.recv_timeout(Duration::from_millis(50)) {
                Ok(result) => return result,
                Err(mpsc::RecvTimeoutError::Timeout) => {}
                Err(_) => return Err("local session closed".into()),
            }
        }
    }
    pub fn cancel_transfer(&self, token: u64) -> Result<(), String> {
        let transfers = self.transfers.lock().map_err(|_| "transfer lock failed")?;
        let flag = transfers.get(&token).ok_or("invalid transfer token")?;
        flag.cancelled.store(true, Ordering::Release);
        Ok(())
    }
    pub fn transfer(&self, options: TransferOptions) -> Result<u64, String> {
        let token = options.token;
        let flag = if token == 0 {
            Arc::new(AtomicBool::new(false))
        } else {
            let mut transfers = self.transfers.lock().map_err(|_| "transfer lock failed")?;
            let ticket = transfers.get_mut(&token).ok_or("invalid transfer token")?;
            if ticket.claimed {
                return Err("transfer token already used".into());
            }
            ticket.claimed = true;
            ticket.cancelled.clone()
        };
        let result = self.transfer_inner(options, flag);
        if token != 0 {
            self.transfers
                .lock()
                .map_err(|_| "transfer lock failed")?
                .remove(&token);
        }
        result
    }
    fn transfer_inner(
        &self,
        options: TransferOptions,
        cancelled: Arc<AtomicBool>,
    ) -> Result<u64, String> {
        if cancelled.load(Ordering::Acquire) {
            return Err("transfer cancelled".into());
        }
        if !options.guest.starts_with('/')
            || options.guest.len() > 4096
            || options.guest.contains('\0')
            || !options.host.is_absolute()
            || options.host.as_os_str().len() > 4096
        {
            return Err("invalid transfer path".into());
        }
        let (result, response) = mpsc::sync_channel(1);
        self.commands
            .try_send(Command::Transfer(Transfer {
                options,
                result,
                cancelled: cancelled.clone(),
            }))
            .map_err(|_| "local session is busy or closed")?;
        loop {
            if self.cancelled.load(Ordering::Acquire) || cancelled.load(Ordering::Acquire) {
                return Err("session cancelled".into());
            }
            match response.recv_timeout(Duration::from_millis(50)) {
                Ok(result) => return result,
                Err(mpsc::RecvTimeoutError::Timeout) => {}
                Err(_) => return Err("local session closed".into()),
            }
        }
    }
}
fn prepare_runtime_mounts(vfs: &mut Vfs) -> Result<(), String> {
    let prepare = |vfs: &mut Vfs| -> Result<(), trames_vfs::Errno> {
        match vfs.mkdir_at(trames_vfs::AT_FDCWD, b"/dev", 0o755) {
            Ok(()) | Err(trames_vfs::Errno(17)) => {}
            Err(error) => return Err(error),
        }
        for (path, parent, mode, options) in [
            (
                b"/run".as_slice(),
                b"/".as_slice(),
                0o755,
                b"mode=755".as_slice(),
            ),
            (
                b"/dev/shm".as_slice(),
                b"/dev".as_slice(),
                0o1777,
                b"size=64m,mode=1777".as_slice(),
            ),
        ] {
            match vfs.mkdir_at(trames_vfs::AT_FDCWD, path, mode) {
                Ok(()) | Err(trames_vfs::Errno(17)) => {}
                Err(error) => return Err(error),
            }
            if vfs.mount_id_at(trames_vfs::AT_FDCWD, path, false)?
                == vfs.mount_id_at(trames_vfs::AT_FDCWD, parent, false)?
            {
                vfs.mount(Some(b"tmpfs"), path, Some(b"tmpfs"), 0, Some(options))?;
            }
        }
        Ok(())
    };
    prepare(vfs).map_err(|error| format!("prepare runtime tmpfs: {error}"))
}
fn prepare_guest_defaults(vfs: &mut Vfs, network: &str, distro: &str) -> Result<(), String> {
    let resolver = if network == "loopback" {
        b"nameserver 127.0.0.53\n".as_slice()
    } else {
        b"nameserver 169.254.0.53\n".as_slice()
    };
    for (path, bytes) in [
        (b"/root/.trames".as_slice(), b"Trames\n".as_slice()),
        (b"/etc/resolv.conf".as_slice(), resolver),
    ] {
        let old_managed_dns = path == b"/etc/resolv.conf"
            && vfs
                .open_at(trames_vfs::AT_FDCWD, path, 0, 0)
                .is_ok_and(|fd| {
                    let data = vfs.read(fd, 256).unwrap_or_default();
                    let _ = vfs.close(fd);
                    data != bytes
                        && (matches!(
                            data.as_slice(),
                            b"nameserver 10.0.0.1\nnameserver fd00::1\n"
                                | b"nameserver 10.0.0.1\n"
                                | b"nameserver 127.0.0.53\n"
                                | b"nameserver 169.254.0.53\n"
                        ) || (distro == "debian"
                            && data == b"# https://1.1.1.1 (privacy-focused, highly-available DNS service)\nnameserver 1.1.1.1\nnameserver 1.0.0.1\n"))
                });
        if old_managed_dns {
            let fd = vfs
                .open_at(trames_vfs::AT_FDCWD, path, 1, 0)
                .map_err(|e| e.to_string())?;
            let result = vfs.write(fd, bytes).and_then(|written| {
                if written != bytes.len() {
                    return Err(trames_vfs::Errno(5));
                }
                vfs.truncate_fd(fd, bytes.len() as u64)
            });
            let close = vfs.close(fd);
            result.and(close).map_err(|e| e.to_string())?;
        } else if vfs.stat_at(trames_vfs::AT_FDCWD, path).is_err() {
            vfs.import_file(path, bytes.to_vec())
                .map_err(|e| e.to_string())?;
        }
    }
    Ok(())
}
fn open_store(options: &Options) -> Result<Vfs, String> {
    let disk = DiskOptions {
        volume_space: Some(VolumeSpace {
            total_bytes: options.volume_total,
            available_bytes: options.volume_available,
        }),
        ..DiskOptions::default()
    };
    match &options.store_owner_lock {
        Some(path) if path.is_absolute() && path.as_os_str().len() <= 4096 => {
            Vfs::open_disk_with_owner_lock(&options.store, disk, path)
        }
        Some(_) => return Err("invalid native store owner lock".into()),
        None => Vfs::open_disk(&options.store, disk),
    }
    .map_err(|e| e.to_string())
}
fn run(
    options: Options,
    session: Arc<Session>,
    commands: mpsc::Receiver<Command>,
) -> Result<u8, String> {
    let guest_memory_limits = memory_limits(&options)?;
    if !matches!(options.init.as_str(), "shell" | "systemd") {
        return Err("init must be shell or systemd".into());
    }
    if options.init == "systemd" && (options.command.is_some() || options.debian_setup.is_some()) {
        return Err("use shell mode for maintenance commands".into());
    }
    if (options.command.is_some() || options.debian_setup.is_some()) && options.multiple_terminals {
        return Err("maintenance commands require a single terminal".into());
    }
    let mut vfs = open_store(&options)?;
    if let Some(budget) = options.budget_bytes {
        let mut limits = trames_vfs::StorageLimits::disk();
        limits.file_bytes = budget;
        vfs.set_storage_limits(limits).map_err(|e| e.to_string())?;
    }
    vfs.attach_bridge(session.bridge.clone());
    if vfs
        .stat_at(trames_vfs::AT_FDCWD, b"/usr/local/bin/trames")
        .is_err()
    {
        vfs.import_executable(
            b"/usr/local/bin/trames",
            include_bytes!("../../../resources/guest/trames").to_vec(),
        )
        .map_err(|e| e.to_string())?;
    }
    // First installation uses a new store chosen by the native adapter. A partial
    // failed installation is never promoted to the active environment directory.
    if vfs.executable(b"/bin/sh").is_err() {
        let archive = options
            .rootfs
            .as_ref()
            .ok_or("rootfs archive is required for first installation")?;
        install_image(&options, archive, &mut vfs)?;
    }
    if session.cancelled.load(Ordering::Acquire) {
        return Err("session cancelled".into());
    }
    if options.distro == "alpine" {
        if let Some(libc) = &options.musl_libc {
            trames_distro::install_musl(
                &mut File::open(libc).map_err(|e| e.to_string())?,
                &mut vfs,
            )
            .map_err(|e| e.to_string())?;
        }
    }
    let init_monitor = if options.init == "systemd" {
        Some(trames_distro::systemd::prepare(&mut vfs).map_err(|e| e.to_string())?)
    } else {
        None
    };
    let native_terminals = if options.multiple_terminals && init_monitor.is_some() {
        Some(crate::terminals::NativeTerminals::for_systemd(&mut vfs)?)
    } else {
        None
    };
    // Before Wayland publishes its socket or any terminal/service can run.
    // Mounts belong to this kernel session, shared by all its local terminals.
    prepare_runtime_mounts(&mut vfs)?;
    prepare_guest_defaults(&mut vfs, &options.network, &options.distro)?;
    vfs.chdir(b"/root").map_err(|e| e.to_string())?;
    // Bundled Mesa/Xwayland adapters target musl. Keep glibc rootfs loaders
    // and libraries intact; their own Wayland clients still use this server.
    let bundled_musl = vfs.executable(trames_distro::MUSL_LOADER).is_ok();
    let mut zink_egl = false;
    if options.vulkan_accelerator && bundled_musl {
        if let Some(path) = &options.vulkan_icd {
            trames_distro::install_vulkan_icd(
                &mut File::open(path).map_err(|e| e.to_string())?,
                &mut vfs,
            )
            .map_err(|e| format!("install Vulkan ICD: {e}"))?;
            match File::open(path.with_file_name("libEGL.so.1")) {
                Ok(mut reader) => {
                    trames_distro::install_zink_egl(&mut reader, &mut vfs)
                        .map_err(|e| format!("install Mesa EGL: {e}"))?;
                    zink_egl = true;
                }
                Err(e) if e.kind() == io::ErrorKind::NotFound => {}
                Err(e) => return Err(format!("read bundled Mesa EGL: {e}")),
            }
        }
    }
    if let Some(timezone) = &options.timezone {
        trames_distro::synchronize_timezone(&mut vfs, &timezone.identifier, &timezone.data)
            .map_err(|e| e.to_string())?;
    }
    vfs.configure_network(match options.network.as_str() {
        "off" => NetworkMode::Off,
        "loopback" => NetworkMode::Loopback,
        "outbound" => NetworkMode::Outbound,
        "full" => NetworkMode::Full,
        _ => return Err("invalid network mode".into()),
    })
    .map_err(|e| e.to_string())?;
    let mut environment: Vec<String> = vec![
        "HOME=/root".into(),
        "USER=root".into(),
        "LOGNAME=root".into(),
        "PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin".into(),
        "TERM=xterm-256color".into(),
        "LANG=C.UTF-8".into(),
    ];
    if zink_egl {
        environment.extend([
            "GALLIUM_DRIVER=zink".into(),
            "MESA_LOADER_DRIVER_OVERRIDE=zink".into(),
            "LIBGL_KOPPER_WAYLAND_NO_DRM=true".into(),
            "mesa_glthread=false".into(),
            "VK_DRIVER_FILES=/usr/share/vulkan/icd.d/trames.json".into(),
        ]);
    }
    if options.wayland {
        if let Some(ref path) = options.xwayland_launcher.as_ref().filter(|_| bundled_musl) {
            trames_distro::install_xwayland_launcher(
                &mut File::open(path).map_err(|e| e.to_string())?,
                &mut vfs,
            )
            .map_err(|e| e.to_string())?;
            // Only expose the native transport with this session's authorized
            // Vulkan + matching EGL adapter. No dependence on user apk paths.
            if zink_egl {
                match File::open(path.with_file_name("Xwayland-native")) {
                    Ok(mut reader) => {
                        trames_distro::install_xwayland_native(&mut reader, &mut vfs)
                            .map_err(|e| format!("install native Xwayland: {e}"))?;
                    }
                    Err(e) if e.kind() == io::ErrorKind::NotFound => {}
                    Err(e) => return Err(format!("read bundled Xwayland: {e}")),
                }
            }
        }
        let server = vfs
            .enable_wayland(b"/run/user/0")
            .map_err(|e| e.to_string())?;
        server
            .attach_clipboard(Arc::new(crate::clipboard::Clipboard::new(&session)))
            .map_err(|e| e.to_string())?;
        *session
            .wayland
            .lock()
            .map_err(|_| "Wayland lock unavailable")? = Some(server);
        environment.extend([
            "XDG_RUNTIME_DIR=/run/user/0".into(),
            "WAYLAND_DISPLAY=wayland-0".into(),
            "DISPLAY=:0".into(),
            "XMODIFIERS=@im=trames".into(),
        ]);
    }
    let argv = if init_monitor.is_some() {
        environment.extend([
            "container=trames".into(),
            "NOTIFY_SOCKET=/run/host/notify".into(),
        ]);
        if options.wayland {
            trames_distro::systemd::file(&mut vfs, b"/run/systemd/system/console-getty.service.d/70-wayland.conf", b"[Service]\nEnvironment=XDG_RUNTIME_DIR=/run/user/0 WAYLAND_DISPLAY=wayland-0 DISPLAY=:0\n").map_err(|e|e.to_string())?;
        }
        vec![
            "/usr/lib/systemd/systemd".into(),
            "--system".into(),
            "--unit=multi-user.target".into(),
        ]
    } else if let Some(setup) = options.debian_setup {
        if options.distro != "debian" {
            return Err("Debian setup requires a Debian environment".into());
        }
        vec![
            "/bin/sh".into(),
            "-ec".into(),
            trames_distro::packages::debian_script(&setup.packages, setup.systemd)?,
        ]
    } else if let Some(command) = options.command {
        command
    } else if options.multiple_terminals {
        vec!["/bin/sh".into(), "-c".into(), crate::terminals::INIT.into()]
    } else {
        vec!["/bin/sh".into(), "-il".into()]
    };
    trames_distro::rootfs::validate_command(&mut vfs, &argv).map_err(|e| e.to_string())?;
    let bytes = vfs
        .executable(argv.first().ok_or("empty Linux command")?.as_bytes())
        .map_err(|e| e.to_string())?;
    let mut runtime = Runtime::load_with_options(
        &bytes,
        &argv,
        &environment,
        vfs,
        trames_core::RuntimeOptions {
            memory_limits: guest_memory_limits,
            pure_accelerator: options.pure_accelerator,
            native_bsearch: options.native_bsearch,
            native_allocator: options.native_allocator,
            vulkan_accelerator: options.vulkan_accelerator,
            video_accelerator: options.vulkan_accelerator,
            ..Default::default()
        },
    )
    .map_err(|e| e.to_string())?;
    let mut terminal = Terminal::default();
    runtime.vulkan_display = session.display.clone();
    terminal.resize(options.rows, options.columns);
    runtime
        .enable_terminal(terminal)
        .map_err(|e| e.to_string())?;
    let mut io = Adapter {
        session: session.clone(),
        commands,
        snapshot_at: Instant::now(),
        terminals: native_terminals,
        init_monitor,
        pending: None,
        host_files: None,
        host_active: None,
    };
    io.snapshot(&runtime.linux.vfs);
    session
        .buffers
        .lock()
        .map_err(|_| "session lock failed")?
        .status
        .phase = "running";
    let result = runtime.run(&mut io, u64::MAX);
    runtime.linux.vfs.finish_network();
    runtime.linux.vfs.close_all();
    io.snapshot(&runtime.linux.vfs);
    result.map(|e| e.code).map_err(|e| match e {
        RuntimeError::Cancelled => "session cancelled".into(),
        _ => e.to_string(),
    })
}
struct Adapter {
    session: Arc<Session>,
    commands: mpsc::Receiver<Command>,
    snapshot_at: Instant,
    terminals: Option<crate::terminals::NativeTerminals>,
    pending: Option<Command>,
    host_files: Option<Arc<trames_linux::FileLayouts>>,
    host_active: Option<Arc<AtomicBool>>,
    init_monitor: Option<trames_distro::systemd::Monitor>,
}
impl Adapter {
    // Legacy capability-less callers retain their explicit stopped boundary.
    fn progress_commands(&mut self, vfs: &Vfs) -> io::Result<bool> {
        if self
            .host_active
            .as_ref()
            .is_some_and(|done| !done.load(Ordering::Acquire))
        {
            return Ok(false);
        }
        self.host_active = None;
        if self.pending.is_none() {
            self.pending = self.commands.try_recv().ok();
        }
        let Some(command) = self.pending.take() else {
            return Ok(false);
        };
        if self.host_files.is_none() {
            self.pending = Some(command);
            return Ok(true);
        }
        let context = vfs.host_files_context();
        let export = match &command {
            Command::Transfer(t) if t.options.export => {
                match context.host_export(t.options.guest.as_bytes()) {
                    Ok(export) => Some(export),
                    Err(error) => {
                        fail_command(command, error.to_string());
                        return Ok(false);
                    }
                }
            }
            _ => None,
        };
        let edit = match &command {
            Command::Transfer(t) if !t.options.export && t.options.expected.is_some() => {
                match context.host_edit(t.options.guest.as_bytes()) {
                    Ok(edit) => Some(edit),
                    Err(error) => {
                        fail_command(
                            command,
                            match error.0 {
                                18 => "edited path is covered by another filesystem; copy retained"
                                    .into(),
                                21 => {
                                    "edited Linux path is no longer a regular file; copy retained"
                                        .into()
                                }
                                _ => error.to_string(),
                            },
                        );
                        return Ok(false);
                    }
                }
            }
            _ => None,
        };
        let done = Arc::new(AtomicBool::new(false));
        self.host_active = Some(done.clone());
        self.host_files
            .as_ref()
            .unwrap()
            .submit_host(Box::new(HostCommand {
                vfs: context,
                command,
                access: HostAccess {
                    export,
                    edit,
                    admission: None,
                },
                completion: HostCompletion {
                    session: self.session.clone(),
                    done,
                    wake: std::thread::current(),
                },
            }));
        Ok(false)
    }
    fn command(&mut self, vfs: &mut Vfs, command: Command) {
        Self::execute(&self.session, vfs, command, HostAccess::default());
    }
    fn execute(session: &Session, vfs: &mut Vfs, command: Command, access: HostAccess) {
        match command {
            Command::Transfer(command) => {
                let result =
                    Self::transfer(session, vfs, command.options, &command.cancelled, access);
                Self::snapshot_session(session, vfs);
                let _ = command.result.send(result);
            }
            Command::Timezone(timezone, response) => {
                let result =
                    trames_distro::synchronize_timezone(vfs, &timezone.identifier, &timezone.data)
                        .map_err(|e| e.to_string());
                Self::snapshot_session(session, vfs);
                let _ = response.send(result);
            }
        }
    }
    fn snapshot(&self, vfs: &Vfs) {
        Self::snapshot_session(&self.session, vfs);
    }
    fn snapshot_session(session: &Session, vfs: &Vfs) {
        let capacity = vfs.capacity();
        let mut b = session.buffers.lock().unwrap_or_else(|p| p.into_inner());
        b.status.budget_bytes = capacity.budget_bytes;
        b.status.used_bytes = capacity.used_bytes;
        b.status.available_bytes = capacity.available_bytes;
        b.status.node_limit = capacity.node_limit;
        b.status.nodes_used = capacity.nodes_used;
    }
    fn transfer(
        session: &Session,
        vfs: &mut Vfs,
        o: TransferOptions,
        cancelled: &AtomicBool,
        access: HostAccess,
    ) -> Result<u64, String> {
        if cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire) {
            return Err("transfer cancelled".into());
        }
        if o.export {
            let export = access
                .export
                .map_or_else(|| vfs.host_export(o.guest.as_bytes()), Ok)
                .map_err(|e| e.to_string())?;
            if o.maximum_bytes
                .is_some_and(|maximum| export.len() > maximum)
            {
                return Err("guest file exceeds transfer size limit".into());
            }
            let mut file = OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(&o.host)
                .map_err(|e| e.to_string())?;
            let result = export
                .write_to(&mut CancelWriter {
                    file: &mut file,
                    session: &session.cancelled,
                    cancelled,
                    remaining: o.maximum_bytes.unwrap_or(u64::MAX),
                })
                .map_err(|e| e.to_string());
            if result.is_err() {
                let _ = fs::remove_file(o.host);
            }
            result.map(|_| 0)
        } else {
            let mut file = File::open(o.host).map_err(|e| e.to_string())?;
            let metadata = file.metadata().map_err(|e| e.to_string())?;
            if !metadata.is_file()
                || o.maximum_bytes
                    .is_some_and(|maximum| metadata.len() > maximum)
            {
                return Err("source is not a regular file or exceeds transfer size limit".into());
            }
            if let Some(expected) = &o.expected {
                if !o.replace {
                    return Err("conditional transfer requires replacement".into());
                }
                if let Some(edit) = access.edit {
                    verify_saved_edit(&mut file, expected, || {
                        cancelled.load(Ordering::Acquire)
                            || session.cancelled.load(Ordering::Acquire)
                    })?;
                    let mut reader = CancelReader {
                        file,
                        session: &session.cancelled,
                        cancelled,
                        remaining: o.maximum_bytes.unwrap_or(u64::MAX),
                    };
                    let admission = access
                        .admission
                        .as_ref()
                        .ok_or("missing original edit admission")?;
                    return edit
                        .replace(vfs, admission, &mut reader, |inode, bytes| {
                            let mut digest = Sha256::new();
                            let mut buffer = [0; 65536];
                            loop {
                                if cancelled.load(Ordering::Acquire)
                                    || session.cancelled.load(Ordering::Acquire)
                                {
                                    return Err(trames_vfs::Errno(4));
                                }
                                let count = bytes.read(&mut buffer).map_err(|e| {
                                    trames_vfs::Errno(e.raw_os_error().unwrap_or(5) as u32)
                                })?;
                                if count == 0 {
                                    break;
                                }
                                digest.update(&buffer[..count]);
                            }
                            let current = format!("{:x}", digest.finalize());
                            if current == expected.replacement_sha256 {
                                return Ok(true);
                            }
                            if inode != expected.inode || current != expected.sha256 {
                                return Err(trames_vfs::Errno(116));
                            }
                            Ok(false)
                        })
                        .map_err(|error| {
                            if error.0 == 116 {
                                "Linux file changed since it was opened; edited copy retained"
                                    .into()
                            } else {
                                error.to_string()
                            }
                        });
                }
                if check_edit_version(vfs, o.guest.as_bytes(), &mut file, expected)? {
                    return file_inode(vfs, o.guest.as_bytes()); // Retry after a completed commit.
                }
            }
            let mut reader = CancelReader {
                file,
                session: &session.cancelled,
                cancelled,
                remaining: o.maximum_bytes.unwrap_or(u64::MAX),
            };
            (if o.replace {
                vfs.import_reader_replace(o.guest.as_bytes(), &mut reader)
            } else {
                vfs.import_reader(o.guest.as_bytes(), &mut reader, false)
            })
            .map_err(|e| e.to_string())?;
            file_inode(vfs, o.guest.as_bytes())
        }
    }
}
struct HostCompletion {
    session: Arc<Session>,
    done: Arc<AtomicBool>,
    wake: std::thread::Thread,
}
impl Drop for HostCompletion {
    fn drop(&mut self) {
        self.done.store(true, Ordering::Release);
        self.session.console_readiness.notify_locked();
        self.wake.unpark();
    }
}
#[derive(Default)]
struct HostAccess {
    export: Option<trames_vfs::HostExport>,
    edit: Option<trames_vfs::HostEdit>,
    admission: Option<trames_linux::HostFileAdmission>,
}
struct HostCommand {
    vfs: Vfs,
    command: Command,
    access: HostAccess,
    completion: HostCompletion,
}
impl trames_linux::HostFileTask for HostCommand {
    fn layout(&self) -> Option<Arc<trames_linux::HostFileLayout>> {
        self.access
            .export
            .as_ref()
            .map(|export| export.layout())
            .or_else(|| self.access.edit.as_ref().map(|edit| edit.layout()))
    }
    fn run(self: Box<Self>, exclusive: Option<trames_linux::HostFileAdmission>) {
        let Self {
            mut vfs,
            command,
            mut access,
            completion,
        } = *self;
        access.admission = exclusive;
        Adapter::execute(&completion.session, &mut vfs, command, access);
    }
    fn fail(self: Box<Self>) {
        fail_command(self.command, "host filesystem executor closed".into());
    }
}
fn fail_command(command: Command, error: String) {
    match command {
        Command::Transfer(t) => {
            let _ = t.result.send(Err(error));
        }
        Command::Timezone(_, reply) => {
            let _ = reply.send(Err(error));
        }
    }
}

fn file_inode(vfs: &Vfs, path: &[u8]) -> Result<u64, String> {
    let stat = vfs
        .stat_at_flags(trames_vfs::AT_FDCWD, path, true)
        .map_err(|e| e.to_string())?;
    Ok(u64::from_le_bytes(stat[8..16].try_into().unwrap()))
}
fn verify_saved_edit(
    replacement: &mut File,
    expected: &TransferVersion,
    cancelled: impl Fn() -> bool,
) -> Result<(), String> {
    if [
        expected.sha256.as_str(),
        expected.replacement_sha256.as_str(),
    ]
    .iter()
    .any(|value| value.len() != 64 || !value.bytes().all(|b| b.is_ascii_hexdigit()))
    {
        return Err("invalid edit checksum".into());
    }
    let mut source = Sha256::new();
    let mut buffer = [0; 65536];
    loop {
        if cancelled() {
            return Err("transfer cancelled".into());
        }
        let count = replacement.read(&mut buffer).map_err(|e| e.to_string())?;
        if count == 0 {
            break;
        }
        source.update(&buffer[..count]);
    }
    replacement.rewind().map_err(|e| e.to_string())?;
    if format!("{:x}", source.finalize()) != expected.replacement_sha256 {
        return Err("saved edit checksum mismatch; copy retained".into());
    }
    Ok(())
}
/// Legacy capability-less stopped caller. Production edits use HostEdit
/// original-file admission and a combined compare/replace transaction.
fn check_edit_version(
    vfs: &Vfs,
    path: &[u8],
    replacement: &mut File,
    expected: &TransferVersion,
) -> Result<bool, String> {
    verify_saved_edit(replacement, expected, || false)?;
    let stat = vfs
        .stat_at_flags(trames_vfs::AT_FDCWD, path, true)
        .map_err(|e| e.to_string())?;
    let root = vfs
        .stat_at(trames_vfs::AT_FDCWD, b"/")
        .map_err(|e| e.to_string())?;
    if stat[..8] != root[..8] {
        return Err("edited path is covered by another filesystem; copy retained".into());
    }
    if u32::from_le_bytes(stat[16..20].try_into().unwrap()) & 0o170000 != 0o100000 {
        return Err("edited Linux path is no longer a regular file; copy retained".into());
    }
    struct HashWriter(Sha256);
    impl Write for HashWriter {
        fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
            self.0.update(bytes);
            Ok(bytes.len())
        }
        fn flush(&mut self) -> io::Result<()> {
            Ok(())
        }
    }
    let mut current = HashWriter(Sha256::new());
    vfs.export_writer(path, &mut current)
        .map_err(|e| e.to_string())?;
    let current = format!("{:x}", current.0.finalize());
    if current == expected.replacement_sha256 {
        return Ok(true);
    }
    if u64::from_le_bytes(stat[8..16].try_into().unwrap()) != expected.inode
        || current != expected.sha256
    {
        return Err("Linux file changed since it was opened; edited copy retained".into());
    }
    Ok(false)
}
struct CancelReader<'a> {
    file: File,
    session: &'a AtomicBool,
    cancelled: &'a AtomicBool,
    remaining: u64,
}
impl Read for CancelReader<'_> {
    fn read(&mut self, bytes: &mut [u8]) -> io::Result<usize> {
        if self.cancelled.load(Ordering::Acquire) || self.session.load(Ordering::Acquire) {
            return Err(io::ErrorKind::Interrupted.into());
        }
        if bytes.is_empty() {
            return Ok(0);
        }
        if self.remaining == 0 {
            let mut extra = [0];
            return match self.file.read(&mut extra)? {
                0 => Ok(0),
                _ => Err(io::Error::new(
                    io::ErrorKind::FileTooLarge,
                    "source exceeds transfer size limit",
                )),
            };
        }
        let length = bytes
            .len()
            .min(self.remaining.min(usize::MAX as u64) as usize);
        let count = self.file.read(&mut bytes[..length])?;
        self.remaining -= count as u64;
        Ok(count)
    }
}
struct CancelWriter<'a> {
    file: &'a mut File,
    session: &'a AtomicBool,
    cancelled: &'a AtomicBool,
    remaining: u64,
}
impl Write for CancelWriter<'_> {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        if self.cancelled.load(Ordering::Acquire) || self.session.load(Ordering::Acquire) {
            return Err(io::ErrorKind::Interrupted.into());
        }
        if !bytes.is_empty() && self.remaining == 0 {
            return Err(io::Error::new(
                io::ErrorKind::FileTooLarge,
                "guest file exceeds transfer size limit",
            ));
        }
        let count = bytes
            .len()
            .min(self.remaining.min(usize::MAX as u64) as usize);
        let count = self.file.write(&bytes[..count])?;
        self.remaining -= count as u64;
        Ok(count)
    }
    fn flush(&mut self) -> io::Result<()> {
        self.file.flush()
    }
}
impl GuestOutput for Adapter {
    fn bind_host_files(
        &mut self,
        _: Arc<dyn trames_linux::FileExecutor>,
        layouts: Arc<trames_linux::FileLayouts>,
    ) {
        self.host_files = Some(layouts);
    }
    fn paused(&self) -> bool {
        self.session.paused.load(Ordering::Acquire)
    }
    fn flush_output(&mut self) -> io::Result<bool> {
        if self.pending.is_none() {
            self.pending = self.commands.try_recv().ok();
        }
        Ok(self.pending.is_none()
            && self
                .host_active
                .as_ref()
                .is_none_or(|done| done.load(Ordering::Acquire)))
    }

    fn source_console(&mut self) -> Arc<dyn trames_vfs::ConsoleIo> {
        let input = self.session.clone();
        let output = self.session.clone();
        let poll = self.session.clone();
        Arc::new(
            trames_vfs::ConsoleCallbacks::new(
                move |count| {
                    if input.multiple_terminals {
                        return Err(trames_vfs::Errno(11));
                    }
                    let mut buffers = input.buffers.lock().unwrap();
                    let bytes = buffers.terminals.get_mut(&1).ok_or(trames_vfs::Errno(5))?;
                    if count == 0 {
                        return Ok(Vec::new());
                    }
                    if bytes.input.is_empty() {
                        return Err(trames_vfs::Errno(11));
                    }
                    let count = count.min(bytes.input.len());
                    let data = bytes.input.drain(..count).collect();
                    input.console_revision[0].fetch_add(1, Ordering::Relaxed);
                    Ok(data)
                },
                move |_, bytes| {
                    let mut buffers = output.buffers.lock().unwrap();
                    let Some(buffer) = buffers.terminals.get_mut(&1) else {
                        return Ok(bytes.len());
                    };
                    let count = bytes.len().min(QUEUE_LIMIT - buffer.output.len());
                    if count == 0 && !bytes.is_empty() {
                        return Err(trames_vfs::Errno(11));
                    }
                    buffer.output.extend(&bytes[..count]);
                    if count != 0 {
                        output.console_revision[1].fetch_add(1, Ordering::Relaxed);
                    }
                    Ok(count)
                },
                move |stream, events| {
                    let buffers = poll.buffers.lock().unwrap();
                    let Some(buffer) = buffers.terminals.get(&1) else {
                        return (16, [0; 2]);
                    };
                    let ready = if stream == trames_vfs::ConsoleStream::Input {
                        if !poll.multiple_terminals && !buffer.input.is_empty() {
                            1
                        } else {
                            0
                        }
                    } else if buffer.output.len() < QUEUE_LIMIT {
                        4
                    } else {
                        0
                    };
                    (
                        events & ready,
                        [
                            poll.console_revision[0].load(Ordering::Relaxed),
                            poll.console_revision[1].load(Ordering::Relaxed),
                        ],
                    )
                },
            )
            .readiness(self.session.console_readiness.clone()),
        )
    }
    fn poll_control(&mut self, vfs: &Vfs) -> io::Result<bool> {
        if self.session.cancelled.load(Ordering::Acquire)
            || self.session.paused.load(Ordering::Acquire)
        {
            return Ok(true);
        }
        if let Some(init) = &mut self.init_monitor {
            init.control(false)
                .map_err(|e| io::Error::other(e.to_string()))?;
        }
        if self.progress_commands(vfs)? {
            return Ok(true);
        }
        {
            let mut buffers = self
                .session
                .buffers
                .lock()
                .map_err(|_| io::Error::other("session lock failed"))?;
            if self.session.multiple_terminals {
                let Some(terminals) = self.terminals.as_mut() else {
                    return Ok(true);
                };
                // Open/close, input, resize and hangup use original PTY resources.
                terminals.pump(&mut buffers.terminals);
            } else if buffers.terminals.get(&1).is_some_and(|b| b.size.is_some()) {
                return Ok(true);
            }
        }
        if self.snapshot_at.elapsed() >= Duration::from_secs(1) {
            self.snapshot(vfs);
            self.snapshot_at = Instant::now();
        }
        Ok(false)
    }
    fn control(&mut self, vfs: &mut Vfs) -> io::Result<bool> {
        if self.session.cancelled.load(Ordering::Acquire) {
            return Ok(false);
        }
        if let Some(init) = &mut self.init_monitor {
            init.control(false)
                .map_err(|e| io::Error::other(e.to_string()))?;
        }
        if self.session.multiple_terminals {
            if self.terminals.is_none() {
                self.terminals =
                    Some(crate::terminals::NativeTerminals::new(vfs).map_err(io::Error::other)?);
            }
            let mut buffers = self
                .session
                .buffers
                .lock()
                .map_err(|_| io::Error::other("session lock failed"))?;
            self.terminals
                .as_mut()
                .expect("initialized")
                .pump(&mut buffers.terminals);
        }
        if self.progress_commands(vfs)? {
            let command = self.pending.take().expect("legacy stopped host command");
            self.command(vfs, command);
        }
        if self.snapshot_at.elapsed() >= Duration::from_secs(1) {
            self.snapshot(vfs);
            self.snapshot_at = Instant::now();
        }
        Ok(true)
    }
    fn write(&mut self, _: OutputFd, bytes: &[u8]) -> io::Result<usize> {
        let mut b = self
            .session
            .buffers
            .lock()
            .map_err(|_| io::Error::other("session lock failed"))?;
        let Some(b) = b.terminals.get_mut(&1) else {
            return Ok(bytes.len());
        };
        let count = bytes.len().min(QUEUE_LIMIT - b.output.len());
        if count == 0 && !bytes.is_empty() {
            return Err(io::ErrorKind::WouldBlock.into());
        }
        b.output.extend(&bytes[..count]);
        Ok(count)
    }
    fn poll_stdin(&mut self, bytes: &mut [u8]) -> io::Result<usize> {
        if self.session.multiple_terminals {
            return Err(io::ErrorKind::WouldBlock.into());
        }
        let mut b = self
            .session
            .buffers
            .lock()
            .map_err(|_| io::Error::other("session lock failed"))?;
        let b = b
            .terminals
            .get_mut(&1)
            .ok_or_else(|| io::Error::other("primary terminal closed"))?;
        if b.input.is_empty() {
            return Err(io::ErrorKind::WouldBlock.into());
        }
        let count = bytes.len().min(b.input.len());
        for byte in &mut bytes[..count] {
            *byte = b.input.pop_front().expect("bounded input");
        }
        Ok(count)
    }
    fn stdin_ready(&mut self) -> io::Result<bool> {
        if self.session.multiple_terminals {
            return Ok(false);
        }
        Ok(self
            .session
            .buffers
            .lock()
            .map_err(|_| io::Error::other("session lock failed"))?
            .terminals
            .get(&1)
            .is_some_and(|b| !b.input.is_empty()))
    }
    fn terminal_event(&mut self) -> Option<TerminalEvent> {
        if self.session.multiple_terminals {
            return None;
        }
        self.session
            .buffers
            .lock()
            .ok()?
            .terminals
            .get_mut(&1)?
            .size
            .take()
            .map(|(rows, columns)| TerminalEvent::Resize { rows, columns })
    }
}

#[cfg(test)]
mod memory_tests {
    use super::*;
    fn test_adapter(init_monitor: Option<trames_distro::systemd::Monitor>) -> Adapter {
        let (commands, receiver) = mpsc::sync_channel(8);
        let session = Arc::new(Session {
            display: Default::default(),
            wayland: Mutex::new(None),
            bridge: Default::default(),
            buffers: Mutex::new(Buffers {
                terminals: BTreeMap::from([(1, Default::default())]),
                status: Status {
                    phase: "running",
                    error: None,
                    exit_code: None,
                    budget_bytes: 0,
                    used_bytes: 0,
                    available_bytes: 0,
                    volume_total_bytes: 0,
                    node_limit: 0,
                    nodes_used: 0,
                },
            }),
            console_revision: Default::default(),
            console_readiness: Arc::default(),
            cancelled: AtomicBool::new(false),
            paused: AtomicBool::new(false),
            multiple_terminals: false,
            next_terminal: AtomicU64::new(2),
            commands,
            transfers: Mutex::new(BTreeMap::new()),
            next_transfer: AtomicU64::new(1),
        });
        Adapter {
            session: session.clone(),
            commands: receiver,
            snapshot_at: Instant::now(),
            terminals: None,
            pending: None,
            host_files: None,
            host_active: None,
            init_monitor,
        }
    }
    type HostJob = (
        Box<dyn trames_linux::HostFileTask>,
        Option<trames_linux::HostFileAdmission>,
    );
    struct TestDisk(mpsc::Sender<HostJob>);
    impl trames_linux::FileExecutor for TestDisk {
        fn submit_file(&self, _: trames_vfs::FileWork, _: Arc<trames_linux::FileResult>) {
            panic!("host fixture submits no guest disk work")
        }
        fn submit_host(
            &self,
            job: Box<dyn trames_linux::HostFileTask>,
            admission: Option<trames_linux::HostFileAdmission>,
        ) {
            self.0.send((job, admission)).unwrap();
        }
    }
    fn disk(adapter: &mut Adapter) -> mpsc::Receiver<HostJob> {
        let (send, receive) = mpsc::channel();
        let executor = Arc::new(TestDisk(send));
        adapter.bind_host_files(executor.clone(), trames_linux::FileLayouts::new(executor));
        receive
    }
    fn run_host(adapter: &Adapter, disk: &mpsc::Receiver<HostJob>) {
        let layouts = adapter.host_files.as_ref().unwrap();
        for request in layouts.take_owner() {
            assert!(layouts.try_submit(request).is_ok());
        }
        let (job, admission) = disk.recv_timeout(Duration::from_secs(3)).unwrap();
        job.run(admission);
    }
    #[test]
    fn filesystem_commands_keep_source_execution_and_paused_completion() {
        let mut vfs = Vfs::default();
        let mut adapter = test_adapter(None);
        let disk = disk(&mut adapter);
        let path = std::env::temp_dir().join(format!("trames-host-command-{}", std::process::id()));
        fs::write(&path, b"original").unwrap();
        let (reply, result) = mpsc::sync_channel(1);
        adapter
            .session
            .commands
            .send(Command::Transfer(Transfer {
                options: TransferOptions {
                    guest: "/file".into(),
                    host: path.clone(),
                    export: false,
                    token: 0,
                    maximum_bytes: None,
                    replace: false,
                    expected: None,
                },
                result: reply,
                cancelled: Arc::new(AtomicBool::new(false)),
            }))
            .unwrap();
        assert!(!adapter.poll_control(&vfs).unwrap());
        assert!(!adapter.flush_output().unwrap());
        run_host(&adapter, &disk);
        let inode = result.recv().unwrap().unwrap();
        fs::write(&path, b"edited").unwrap();
        let (reply, result) = mpsc::sync_channel(1);
        adapter
            .session
            .commands
            .send(Command::Transfer(Transfer {
                options: TransferOptions {
                    guest: "/file".into(),
                    host: path.clone(),
                    export: false,
                    token: 0,
                    maximum_bytes: None,
                    replace: true,
                    expected: Some(TransferVersion {
                        inode,
                        sha256: format!("{:x}", Sha256::digest(b"original")),
                        replacement_sha256: format!("{:x}", Sha256::digest(b"edited")),
                    }),
                },
                result: reply,
                cancelled: Arc::new(AtomicBool::new(false)),
            }))
            .unwrap();
        adapter.session.paused.store(true, Ordering::Relaxed);
        assert!(adapter.control(&mut vfs).unwrap());
        assert!(adapter.paused());
        run_host(&adapter, &disk);
        assert_ne!(result.recv().unwrap().unwrap(), inode);
        let mut bytes = Vec::new();
        vfs.export_writer(b"/file", &mut bytes).unwrap();
        assert_eq!(bytes, b"edited");
        adapter.session.paused.store(false, Ordering::Relaxed);
        let (reply, result) = mpsc::sync_channel(1);
        adapter
            .session
            .commands
            .send(Command::Timezone(
                HostTimezone {
                    identifier: "UTC".into(),
                    data: {
                        let mut tz = vec![0; 44];
                        tz[..4].copy_from_slice(b"TZif");
                        tz
                    },
                },
                reply,
            ))
            .unwrap();
        assert!(!adapter.poll_control(&vfs).unwrap());
        run_host(&adapter, &disk);
        result.recv().unwrap().unwrap();
        let output = path.with_extension("export");
        let _ = fs::remove_file(&output);
        let (reply, result) = mpsc::sync_channel(1);
        adapter
            .session
            .commands
            .send(Command::Transfer(Transfer {
                options: TransferOptions {
                    guest: "/file".into(),
                    host: output.clone(),
                    export: true,
                    token: 0,
                    maximum_bytes: None,
                    replace: false,
                    expected: None,
                },
                result: reply,
                cancelled: Arc::new(AtomicBool::new(false)),
            }))
            .unwrap();
        assert!(!adapter.poll_control(&vfs).unwrap());
        run_host(&adapter, &disk);
        result.recv().unwrap().unwrap();
        assert_eq!(fs::read(&output).unwrap(), b"edited");
        assert!(adapter.flush_output().unwrap());
        fs::remove_file(path).unwrap();
        fs::remove_file(output).unwrap();
    }
    #[test]
    fn init_notifications_progress_without_requesting_a_stopped_callback() {
        let mut vfs = Vfs::default();
        vfs.import_reader(
            b"/usr/lib/systemd/systemd",
            &mut b"init endpoint fixture".as_slice(),
            true,
        )
        .unwrap();
        for path in [b"/run".as_slice(), b"/sys", b"/dev"] {
            if vfs.stat_at(trames_vfs::AT_FDCWD, path).is_err() {
                vfs.mkdir_at(trames_vfs::AT_FDCWD, path, 0o755).unwrap();
            }
        }
        let monitor = trames_distro::systemd::prepare(&mut vfs).unwrap();
        let mut adapter = test_adapter(Some(monitor));
        let session = adapter.session.clone();
        let sender = vfs.local_create(2, 0).unwrap();
        let sender = vfs.local_socket(sender).unwrap().unwrap();
        for round in 0..2 {
            vfs.local_send_credentials(
                &sender,
                b"READY=1\nX_SYSTEMD_SIGNALS_LEVEL=2",
                0,
                Some(b"\x01\x00/run/host/notify\x00"),
                Default::default(),
                [1, 0, 0],
            )
            .unwrap();
            assert!(!adapter.poll_control(&vfs).unwrap());
            assert_eq!(
                adapter
                    .init_monitor
                    .as_mut()
                    .unwrap()
                    .control(true)
                    .unwrap(),
                if round == 0 { Some(37) } else { None }
            );
        }
        session.paused.store(true, Ordering::Relaxed);
        assert!(adapter.poll_control(&vfs).unwrap());
    }

    #[test]
    fn runtime_tmpfs_is_fresh_after_cold_disk_reopen() {
        let path = std::env::temp_dir().join(format!(
            "trames-runtime-mounts-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        {
            let mut vfs = Vfs::open_disk(&path, DiskOptions::default()).unwrap();
            prepare_runtime_mounts(&mut vfs).unwrap();
            for file in [b"/run/stale.pid".as_slice(), b"/dev/shm/stale".as_slice()] {
                let fd = vfs
                    .open_at(trames_vfs::AT_FDCWD, file, 0x41, 0o600)
                    .unwrap();
                vfs.write(fd, b"old session").unwrap();
                vfs.close(fd).unwrap();
            }
            vfs.sync().unwrap();
        }
        {
            let mut vfs = Vfs::open_disk(&path, DiskOptions::default()).unwrap();
            prepare_runtime_mounts(&mut vfs).unwrap();
            assert!(vfs
                .stat_at(trames_vfs::AT_FDCWD, b"/run/stale.pid")
                .is_err());
            assert!(vfs
                .stat_at(trames_vfs::AT_FDCWD, b"/dev/shm/stale")
                .is_err());
            assert_eq!(
                &vfs.statfs_at(b"/dev/shm").unwrap()[..8],
                &0x01021994u64.to_le_bytes()
            );
        }
        std::fs::remove_dir_all(path).unwrap();
    }
    #[test]
    fn runtime_tmpfs_precedes_wayland_and_repeated_preparation_preserves_live_files() {
        let mut vfs = Vfs::default();
        prepare_runtime_mounts(&mut vfs).unwrap();
        for (path, mode) in [
            (b"/run".as_slice(), 0o755),
            (b"/dev/shm".as_slice(), 0o1777),
        ] {
            assert_eq!(
                &vfs.statfs_at(path).unwrap()[..8],
                &0x01021994u64.to_le_bytes()
            );
            assert_eq!(
                u32::from_le_bytes(
                    vfs.stat_at(trames_vfs::AT_FDCWD, path).unwrap()[16..20]
                        .try_into()
                        .unwrap()
                ) & 0o7777,
                mode
            );
        }
        let shm = vfs.statfs_at(b"/dev/shm").unwrap();
        let block = u64::from_le_bytes(shm[8..16].try_into().unwrap());
        let blocks = u64::from_le_bytes(shm[16..24].try_into().unwrap());
        assert_eq!(block * blocks, 64 << 20);
        assert_eq!(
            &shm[16..24],
            &shm[24..32],
            "empty shm is lazy, not preallocated"
        );
        let run = vfs
            .mount_id_at(trames_vfs::AT_FDCWD, b"/run", false)
            .unwrap();
        let shm = vfs
            .mount_id_at(trames_vfs::AT_FDCWD, b"/dev/shm", false)
            .unwrap();
        let _wayland = vfs.enable_wayland(b"/run/user/0").unwrap();
        let socket = vfs
            .stat_at(trames_vfs::AT_FDCWD, b"/run/user/0/wayland-0")
            .unwrap();
        let fd = vfs
            .open_at(trames_vfs::AT_FDCWD, b"/run/live.pid", 0x41, 0o600)
            .unwrap();
        vfs.write(fd, b"123\n").unwrap();
        vfs.close(fd).unwrap();
        prepare_runtime_mounts(&mut vfs).unwrap();
        assert_eq!(
            vfs.mount_id_at(trames_vfs::AT_FDCWD, b"/run", false)
                .unwrap(),
            run
        );
        assert_eq!(
            vfs.mount_id_at(trames_vfs::AT_FDCWD, b"/dev/shm", false)
                .unwrap(),
            shm
        );
        assert_eq!(
            vfs.stat_at(trames_vfs::AT_FDCWD, b"/run/user/0/wayland-0")
                .unwrap()[8..16],
            socket[8..16]
        );
        assert_eq!(
            u64::from_le_bytes(
                vfs.stat_at(trames_vfs::AT_FDCWD, b"/run/live.pid").unwrap()[48..56]
                    .try_into()
                    .unwrap()
            ),
            4
        );
        let peer = vfs.clone();
        assert_eq!(
            peer.mount_id_at(trames_vfs::AT_FDCWD, b"/run", false)
                .unwrap(),
            run
        );
    }
    #[test]
    fn managed_dns_is_routable_for_containers_and_preserves_custom_configs() {
        let mut vfs = Vfs::default();
        for dir in [b"/etc".as_slice(), b"/root".as_slice()] {
            if vfs.stat_at(trames_vfs::AT_FDCWD, dir).is_err() {
                vfs.mkdir_at(trames_vfs::AT_FDCWD, dir, 0o755).unwrap();
            }
        }
        let read = |vfs: &mut Vfs| {
            let fd = vfs
                .open_at(trames_vfs::AT_FDCWD, b"/etc/resolv.conf", 0, 0)
                .unwrap();
            let bytes = vfs.read(fd, 1024).unwrap();
            vfs.close(fd).unwrap();
            bytes
        };
        prepare_guest_defaults(&mut vfs, "full", "alpine").unwrap();
        assert_eq!(read(&mut vfs), b"nameserver 169.254.0.53\n");
        prepare_guest_defaults(&mut vfs, "loopback", "alpine").unwrap();
        assert_eq!(read(&mut vfs), b"nameserver 127.0.0.53\n");
        prepare_guest_defaults(&mut vfs, "full", "alpine").unwrap();
        assert_eq!(read(&mut vfs), b"nameserver 169.254.0.53\n");
        let fd = vfs
            .open_at(trames_vfs::AT_FDCWD, b"/etc/resolv.conf", 1 | 0x200, 0)
            .unwrap();
        vfs.write(fd, b"nameserver 1.1.1.1\n# user override\n")
            .unwrap();
        vfs.close(fd).unwrap();
        prepare_guest_defaults(&mut vfs, "full", "debian").unwrap();
        assert_eq!(read(&mut vfs), b"nameserver 1.1.1.1\n# user override\n");
        let shipped=b"# https://1.1.1.1 (privacy-focused, highly-available DNS service)\nnameserver 1.1.1.1\nnameserver 1.0.0.1\n";
        let fd = vfs
            .open_at(trames_vfs::AT_FDCWD, b"/etc/resolv.conf", 1 | 0x200, 0)
            .unwrap();
        vfs.write(fd, shipped).unwrap();
        vfs.close(fd).unwrap();
        prepare_guest_defaults(&mut vfs, "full", "custom").unwrap();
        assert_eq!(read(&mut vfs), shipped);
        prepare_guest_defaults(&mut vfs, "full", "debian").unwrap();
        assert_eq!(read(&mut vfs), b"nameserver 169.254.0.53\n");
    }
    #[test]
    fn optional_json_budgets_preserve_defaults_and_validate_before_start() {
        let base = serde_json::json!({"store":"/tmp/trames-memory-options", "volume_total": 1 << 30, "volume_available": 1 << 30});
        let options: Options = serde_json::from_value(base.clone()).unwrap();
        assert_eq!(
            memory_limits(&options).unwrap(),
            trames_core::MemoryLimits::default()
        );
        let mut custom = base;
        custom["memory_budget_bytes"] = serde_json::json!(4usize << 30);
        custom["process_memory_budget_bytes"] = serde_json::json!(2usize << 30);
        let options: Options = serde_json::from_value(custom.clone()).unwrap();
        assert_eq!(memory_limits(&options).unwrap().per_process, 2 << 30);
        custom["process_memory_budget_bytes"] = serde_json::json!(8usize << 30);
        let options: Options = serde_json::from_value(custom).unwrap();
        assert!(Session::start(options).is_err());
    }
    #[test]
    fn fileprovider_commit_detects_same_inode_edits_and_retries_completed_saves() {
        use std::io::Cursor;
        let mut vfs = Vfs::default();
        vfs.import_reader(b"/edit.txt", &mut Cursor::new(b"old"), false)
            .unwrap();
        let stat = vfs.stat_at(trames_vfs::AT_FDCWD, b"/edit.txt").unwrap();
        let path = std::env::temp_dir().join(format!(
            "trames-edit-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        fs::write(&path, b"new").unwrap();
        let mut file = File::open(&path).unwrap();
        let expected = TransferVersion {
            inode: u64::from_le_bytes(stat[8..16].try_into().unwrap()),
            sha256: format!("{:x}", Sha256::digest(b"old")),
            replacement_sha256: format!("{:x}", Sha256::digest(b"new")),
        };
        assert!(!check_edit_version(&vfs, b"/edit.txt", &mut file, &expected).unwrap());
        let fd = vfs
            .open_at(trames_vfs::AT_FDCWD, b"/edit.txt", 1 | 0x200, 0)
            .unwrap();
        vfs.write(fd, b"bad").unwrap();
        vfs.close(fd).unwrap();
        assert!(check_edit_version(&vfs, b"/edit.txt", &mut file, &expected)
            .unwrap_err()
            .contains("changed"));
        vfs.import_reader_replace(b"/edit.txt", &mut Cursor::new(b"new"))
            .unwrap();
        assert!(check_edit_version(&vfs, b"/edit.txt", &mut file, &expected).unwrap());
        fs::write(&path, b"tampered").unwrap();
        assert!(check_edit_version(&vfs, b"/edit.txt", &mut file, &expected)
            .unwrap_err()
            .contains("checksum"));
        fs::remove_file(path).unwrap();
    }
}
