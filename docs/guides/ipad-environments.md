# Linux environments on iPad

Open **+ → Linux 环境 → 新建 Linux 环境**. Existing Alpine data keeps its original
store and Files identity. New environments have independent software and files;
one local environment runs at a time. Switching stops the current Linux session.
Returning from the background resumes the selected session.

Choose Alpine, Debian 13, or a custom ARM64 Linux rootfs. The built-in distributions
use immutable source URLs and checked SHA-256 digests. Debian defaults to daily
tools: certificates, Git/SSH clients, editors, download/network/process utilities
and archive tools. The development profile also installs compilers, Python,
CMake and Ninja; the base profile leaves those optional packages out. Package
setup runs in the guest and streams its output into the terminal.

Debian can start with a shell or original systemd as PID 1. Selecting systemd
installs systemd and D-Bus when needed. In the environment's **更多** menu, change
the boot mode for its next start. **打开维护终端** temporarily starts a shell and
skips pending package setup, so a stopped or interrupted installation can be
repaired. Reopening normally retries the configured preparation.

Custom rootfs import uses a Files-selected `tar` or `tar.gz` copy, with an optional
SHA-256. The archive root must contain executable `/bin/sh`, `setsid` on the normal
Linux PATH, and their ARM64 Linux loaders/libraries. A systemd image must also
include `/usr/lib/systemd/systemd` and D-Bus; Trames does not assume a package
manager for custom images.
OCI image bundles, sparse tar, x86 ELF and Mach-O entrypoints are not supported.
Archive contents are imported into the guest VFS, never extracted on the host.
The selected archive copy is retained for resetting that custom environment.

The normal **新建终端** action opens another PTY in the current environment;
terminals share its processes, files and network. Systemd uses the same PTY
transport through its console service. **设置 → 重置 Linux 环境** resets only the
selected environment; it does not delete other environments or Files grants.

Files lists installed environments under **Trames → environment name** when the
app and extension have matching App Group signing. A pending rootfs import does
not hide existing environments. Existing-file edits are staged and committed when that
particular environment is active again. Switching to a different environment
leaves its pending edits intact. See [Files interoperability](rootfs-files.md).

The bundled iPad Mesa/Xwayland/Vulkan guest adapters currently target musl.
Debian and glibc-based custom images retain their own graphical libraries and
loaders; Trames does not insert musl adapters into them. The shared Wayland server
is available, but this change does not provide a Debian graphics-driver bundle.

Implementation and container policy live in the shared runtime's
[environment boot design](https://github.com/kb5000/trames-runtime/blob/main/docs/architecture/environment-boot.md).

See the [validation record](../validation/ipad-environments-20261102.json) for
physical iPad checks, production FFI coverage and the systemd idle-CPU comparison.

## Experimental Android environments

In **+ → Linux 环境 → 新建环境**, select **Android** and choose the ARM64
Trames adapter archive from Files. The adapter archive is small; initial setup
separately downloads approximately 1 GB of pinned official Waydroid images and
checks their sizes and SHA-256 digests before importing them. Keep the app in the
foreground during preparation. Physical iPad desktop compatibility remains
experimental and is not implied by successful installation of the app.

Bootstrap downloads allow one hour for the entire transfer, with a 60-second
inactivity timeout and at most three attempts for transient network errors. A
slow but active image download no longer expires after one minute. Retries
restart the current download; byte-range resume is not implemented.

Preparation prints timestamped progress for download, checksum verification,
ZIP decompression, ext4 file import, storage synchronization, validation and
publication. Byte percentages appear only when a total is known; file import
shows actual bytes, entry count and the most recent file instead of a fabricated
overall percentage. Normal updates are limited to twice per second, with
immediate stage boundaries. A stage label alone is not proof of forward progress;
check whether the timestamp and counters are advancing.

Use **Settings → Diagnostics → Share environment preparation log** to export
the current JSON Lines journal, including failures and the last native installer
checkpoint. It is kept at `Documents/Diagnostics/environment-preparation.jsonl`;
the preceding preparation has a `.previous.jsonl` sibling. Each file is bounded
to 2 MiB. Native progress callbacks execute synchronously on the existing import
worker and are never retained. They add no coordinator, polling timer or normal
guest execution overhead. Closing the preparing terminal propagates cancellation
to the installer at progress checkpoints.

The journal's `ready` event means that host preparation has completed and the
guest session can execute. It does **not** mean that Android has finished its
first boot or that its desktop has drawn a frame. Subsequent Android service
startup is separate from image download/import.

The application requests Apple's increased-memory-limit entitlement on supported
devices. This changes the host application's possible headroom, not the guest
memory budget, and does not reserve physical RAM. The Files extension keeps its
separate App Group entitlement without this application-only request. Preparation
events record advisory `host_available_bytes` and `host_physical_bytes`; remaining
headroom can vary and must not be interpreted as allocated memory or a guarantee.

With an Android environment selected, **Settings → Diagnostics → Collect Android
diagnostics** preserves a non-consuming snapshot of the existing bounded Trames
kernel log, queries original Android properties and input/display state, captures
one-shot warning/crash `logcat -d`, and reads recent ANR, tombstone and DropBox files through the existing Files
snapshot reader. Logcat runs on existing guest execution workers; completion
wakes the calling background queue without polling or a persistent helper.
Each service dump has a five-second timeout; the fixed script contains no
interpolated command or guest input. Capture is limited to one request per session
and 45 seconds. If the guest cannot
complete logcat, the export still includes previously saved crash files and the
capture error. This does not restart Android, open another filesystem writer, or
pause its execution. The JSON export contains at most 64 files, 2 MiB per file
and 16 MiB of source bytes, with truncation markers. Repeated `_wtf` warnings have
lower priority than crash/ANR records. **Share Android diagnostics** exports the
result from `Documents/Diagnostics/android-<environment UUID>.json`; it is not
uploaded automatically. The source files may change during a live capture, so
this is diagnostic evidence rather than a transactional filesystem backup.

The iPad interpreter boot profile defaults `ro.hw_timeout_multiplier` to `5`
when the image has no explicit value. Android 13's original foreground service
deadline becomes 100 seconds rather than 20 seconds; guest clocks and execution
speed are unchanged. Physical traces showed runnable SystemUI initialization
still in its service constructors at the original deadline. This is Android's
existing slow-hardware policy, not a claim that every ANR is harmless or that a
rendering crash has been repaired. The Mac HVF product keeps its original policy.
See AOSP's [Build property](https://github.com/aosp-mirror/platform_frameworks_base/blob/android-13.0.0_r1/core/java/android/os/Build.java)
and [service deadlines](https://github.com/aosp-mirror/platform_frameworks_base/blob/android-13.0.0_r1/services/core/java/com/android/server/am/ActiveServices.java).

Android uses its own init target and timezone service. The iPad adapter keeps
the separate Linux init selector at `shell` and skips Linux TZif file injection
for Android, including live host timezone updates. Android timezone property
synchronization is not implemented; Linux timezone synchronization is unchanged.
The physical-device diagnostics and current acceptance limits are recorded in
[the preparation validation](../validation/ipad-preparation-progress-20261118.json).

## Application language and guest locale

The app supports English and Simplified Chinese, including environment setup,
settings, file transfer, native access consent, accessibility labels, Files
pending-edit folders and the local-network permission explanation. It follows
iPadOS's language preference for Trames; choose the app's language in system
Settings when an override is needed. Other languages fall back to English.
Environment names, guest terminal output and Linux window titles remain their
original content. Low-level runtime/package diagnostics retain their source text.

Application language does not configure Linux locale. The existing `C.UTF-8`
default remains available for UTF-8 terminal text; no locale packages or
`locale-gen` setup is needed for the app's interface. Configure a locale inside
a guest only when that Linux application's own behavior requires it.

## Display density and Picture in Picture

Wayland clients receive the screen's integer density. Scale-aware applications
render sharper buffers while keeping window sizes and pointer coordinates in
logical units. Fractional scaling and physical monitor metadata are not exposed.

Select a terminal or graphical window and tap **Picture in Picture / 画中画** in
the toolbar or floating window title bar. The preview follows that selected
window; tapping its button again closes it. A playing PiP keeps its environment
running in the background. Closing or pausing PiP restores normal background
pausing. iPadOS still controls resource termination and suspension.

Terminal and software-rendered Wayland content can update in the background.
Native GPU windows show their last foreground frame until Trames returns, since
iPadOS restricts background Metal execution. Preview capture runs only while
requested, at up to 5 fps and 960×720. Staged Files edits continue to commit on a
foreground return. See the shared [display lifecycle design](https://github.com/kb5000/trames-runtime/blob/main/docs/architecture/display-lifecycle.md)
and [validation record](../validation/display-lifecycle-20261104.json).

## TOML configuration

New environments have a host profile at `profiles/<installation-id>.toml` in
Trames' shared data root. The app uses the same Rust manifest model as Mac; its
[configuration and authorization specification](https://github.com/kb5000/trames-runtime/blob/main/docs/architecture/environment-configuration.md)
is shared. Without an explicit host profile the runtime reads guest
`/etc/trames/env.toml`. There is no configuration migration reader.

`roots.json` is an installation and File Provider index. It identifies private
stores and pending package preparation; boot settings are loaded from TOML.
Host-owned `grants-<installation-id>.toml` records are stored separately in the
app's authorization directory and are bound to the manifest UUID. New
owner-created environments allow the platform-supported clipboard, Files and
host UI controls by default; existing explicit denials remain intact. Guest
permission commands are read-only, and OS privacy/folder authorizations still
apply. The old per-request Trames consent sheet is removed. Clipboard controls use
`/dev/trames/clipboard`, independently of host UI, CLI, and execution.

The iPad embedding supports Linux shell/systemd and Android manifests and
resolves CPU/component selections from them. Its `auto` memory selection uses
the app's platform budget. Arbitrary host execution and Mac private-network or
host-mount administration are unavailable on iPad; Files permissions remain
security-scoped platform authorizations. Unsupported settings fail rather than
silently selecting a different network or mounting an arbitrary host path.

## Task capacity and planned live configuration

The shared default is now 65,536 admitted tasks for both Linux and Android. The
former Linux-only default of 256 could reject ordinary Firefox renderer thread
creation with `EAGAIN`. This ceiling is atomic admission metadata: it does not
preallocate tasks, guest memory or host execution threads. Existing lower
embedding-supplied limits remain effective.

Owner decision, 2026-11-14: add Linux-compatible live sysctl administration. That
configuration interface remains planned; increasing the default does not make
`threads-max` writable yet.

The intended interface is `/proc/sys/kernel/threads-max`, with a proposed
`[kernel.sysctl]` manifest table providing boot-time values. Guest `sysctl -w`
and manifest initialization must update the same environment-owned capacity.
`pid_max`, per-UID `RLIMIT_NPROC`, and cgroup `pids.max` remain distinct controls.
Increasing capacity must not preallocate tasks, memory, or host threads. The
schema will be settled in the implementation task; do not add
a second independent `resources.tasks` quota or a coordinator/service thread.

## Alpine Firefox graphics payload

Firefox uses the native Wayland session (`GDK_BACKEND=wayland`). Its Vulkan ICD
and the paired Mesa 25.1.9 EGL/Gallium libraries are app resources, installed
before guest processes start. Both Mesa libraries go under `/usr/local/lib`;
Alpine package files under `/usr/lib` remain intact. A frontend linked to a
stock Gallium build is not an interchangeable pair: Firefox may initialize EGL
and then crash when creating a drawing pipeline.

`scripts/build-ios.sh` synchronizes the complete graphics payload. The Xcode app
target also validates the actual resource directory against current ICD sources,
wire revision, Mesa patches and binary hashes. Direct Xcode builds therefore
reject stale resources. Build the guest ICD with `scripts/build-vulkan-guest.sh`
and paired Mesa with `scripts/build-zink-egl.py` before packaging; already
validated, unchanged Mesa binaries can be reused with their paired manifest.

Linux startup seeds `/etc/machine-id` only when missing. Mac and iPad call the
same shared runtime helper; existing identities and linked `/etc` directories
are preserved. This does not install D-Bus: an accessibility-bus warning about a
missing `dbus-launch` still requires the guest's own D-Bus packages/services.
For Alpine, install `dbus-x11 at-spi2-core font-noto-cjk` and launch Firefox
inside `dbus-run-session -- firefox` when no session bus is already running.
These are guest packages, separate from the app's injected graphics payload.

The iOS native Rust archive also carries a source-checked MoltenVK build record.
`scripts/stage-ios-rust.py` builds and stages it with the correct Apple SDK;
Xcode rejects a stale Rust archive or missing native image-memory corrections.

`GuestVideo` contains the VA-API driver and the paired FFmpeg 7.1 decoder
libraries. Stage it with `scripts/prepare-ios-video.py` from the current musl
driver output and FFmpeg installation. `components.vaapi = true` enables its
startup injection and VideoToolbox capability; disabling it does not expose the
native video device. The bundle targets musl, with H.264 and original-bitstream
HEVC/AV1 support; Debian/glibc requires its own compatible adapters. Decoder
libraries remain separate from signed distribution files. This does not install
a guest player or override Firefox's media preferences.

Physical acceptance and the artifact hashes are recorded in
[the 2026-11-14 graphics/video validation](../validation/ipad-graphics-video-20261114.json).

## Injected guest CLI

Linux environments receive the shared static ARM64 command at
`/usr/local/bin/trames` and `/usr/bin/trames`; Android uses `/system/bin/trames`.
It has no guest package dependency. `trames --help`, `status`, `doctor`,
`permissions`, `clipboard get|set`, `files ls|cp` and `host open|gui` use the
same command model as Mac. Use `--json` for scripts, or `TRAMES_LANGUAGE=zh`
for Chinese help without changing the environment locale.

Files transfers use `share:PROVIDER_ID/relative/path`, with IDs listed by
`trames files ls`. The existing Files adapter supports individual files up to
1 GiB; recursive/no-clobber provider copies report unsupported. Host command
execution, delegated Mac CLI, dynamic host mounts and port administration
remain unsupported on iPad. Newly started sessions attach the control metadata;
running sessions retain their original runtime until restarted.
