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
owner-created environments retain basic clipboard integration; other command
capabilities require explicit grants. Clipboard controls use
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
