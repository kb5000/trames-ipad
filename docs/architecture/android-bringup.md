# Android container bring-up

Status, 2026-11-13: the original ARM64 Waydroid Android 13 framework,
SurfaceFlinger and Launcher3 run in a native Mac window through the shared
userspace kernel and native Apple GPU. Native mouse input opens the original
Calendar APK, and keyboard text reaches its search field. This is an experimental
desktop milestone, not general Android application or hardware conformance. Binder implementation:
[`fe2f98e`](https://github.com/kb5000/trames-runtime/commit/fe2f98ea447cf23a13083fd962c34a7b65bd1075).
Init/resource foundation:
[`e6c9546`](https://github.com/kb5000/trames-runtime/commit/e6c954634e5e5a01cda034aea6c78b8898f0d17f).
Original runtime audit baseline:
[`c082959`](https://github.com/kb5000/trames-runtime/commit/c0829591f8652e55c3edb074f593f680efbb76b9).

## Optional product target and APK installation

Linux remains the default. Android is a separate experimental environment with
its own persistent store and original Android init, rather than a Linux init
mode or a replacement framework. The manager can import a Trames Android ARM64
package. The iPad catalog and embedding API carry the same `target` selection;
physical iPad Android boot remains unverified.

The Mac CLI uses the normal environment lifecycle:

```sh
trames env create android --target android --rootfs Trames-Android-13-arm64-adapters.tar.gz
trames env start android
trames -e android exec -- /system/bin/getprop sys.boot_completed
trames -e android apk install /path/to/original.apk
trames -e android apk list
trames -e android apk launch com.example.application
trames -e android shell
trames env stop android
```

Wait for the original framework to finish its first boot before launching apps.
APK install/list use Android's own blocking PackageManager service lookup. APK
bytes stream through the existing bounded exec pipes while output is drained;
there is no new guest HTTP/ADB daemon or coordinator service. Android uses its
own init and Vulkan HALs; selecting Linux systemd or disabling required Vulkan
for that environment is rejected. Older configurations without `target` retain
Linux behavior. Host launch agents raise only their own file descriptor soft
limit, since the original Android framework exceeds launchd's 256-FD default.
Stopped launchd jobs are re-registered so changed executable paths and limits
take effect. APK launch resolves the real component through PackageManager,
then calls the original `am start -W -n`; Android `am` errors produce a failing
host exit status even when the original command itself returns zero. An explicit
`PACKAGE/ACTIVITY` is also accepted.

### Small adapter package and upstream composition

The optional package built by `scripts/package-android.py` now contains only
four ARM64 adapter binaries, graphics-selection properties, a manifest, the
GPL text and source-location metadata: eight regular files, under 1 MiB
compressed. Android system/vendor, APKs, user data and source tarballs
are not in this adapter archive. Matching iPad/runtime source exports are a
separate `-sources.tar.gz` artifact; publish it with the adapters and checksum
sidecars. Linux remains the default and no Android image is added to either
product application download.

The authoritative upstream recipe is `runtime/scripts/android-images.json`.
It pins the official Waydroid ARM64 VANILLA system and MAINLINE vendor ZIP URLs,
compressed sizes/SHA-256 and exact raw ext4 image names/sizes. The adapter
manifest must match the host's supported image pair and graphics contract.
Updates to these pins require compatibility validation, rather than selecting
a moving upstream `latest` image.

Mac environment creation and iPad Android import read that manifest, download
missing ZIPs directly from upstream using their existing platform downloader,
verify size/hash and retain a reusable cache. The common `trames-distro`
installer verifies the inputs, expands one image at a time to an immediately
unlinked temporary file, then copies filesystem entries straight into the
unpublished private VFS. It preserves numeric owners, modes, timestamps,
symlinks and hardlink identity. No root privileges, filesystem mount, external
extraction command, Linux VM or host directory tree is required. The readonly
ext4 parser is upstream `ext4-view` 1.0.0, with one documented accessor exposing
inode numbers for hardlink preservation; ZIP decoding uses pinned `zip` 9.5.1.
The parser and download path are installation-only; normal guest filesystem I/O
uses the existing private store without a new wrapper or coordinator.

Each environment gets independent writable data. The adapter payload excludes
mutable paths; the installer also omits mutable system-image descendants.
Android's guest network default remains `off`: downloading installation inputs
does not enable outbound guest traffic. Old full-rootfs packages remain
importable through the original path.

```sh
python3 scripts/package-android.py \
  --output target/android-packages/Trames-Android-13-arm64-adapters.tar.gz
trames env create android --target android \
  --rootfs target/android-packages/Trames-Android-13-arm64-adapters.tar.gz
```

Mac caches ZIPs under `TRAMES_HOME/cache/android`; an explicit
`TRAMES_ANDROID_IMAGE_CACHE` directory can reuse independently obtained, pinned
ZIPs for offline creation. iPad caches inputs in its private application support
area. Users still need approximately 1.06 GB of upstream downloads and roughly
2.43 GB of installed file content, plus first-boot/application data and one
image's temporary installation space. The distribution size reduction does not
remove the Android system's installed storage requirement.

Validation: a fresh Mac environment was created from the 106,601-byte test
adapter and original cached ZIPs, completed Android boot and displayed the
original Calendar application through native graphics. A native ext4 fixture
checks UID/GID, executable permissions, hardlinks, relative/absolute symlinks
and mutable-data exclusion. Import tests reject unsupported image recipes,
duplicate/unexpected payloads and wrong hashes before opening upstream images;
the current packaging check verifies the eight-file payload and separate source bundle.
iPad FFI cross-compilation and an unsigned Dev application build pass. Physical
iPad Android/download acceptance remains pending.

Shared importer revision:
[`81c9c15`](https://github.com/kb5000/trames-runtime/commit/81c9c1564b13b0db542321db19947c1ec089803f).
Full local runtime workflows pass on Mac and native ARM64 Linux (7/7 each);
the Mac product workflow passes 12/12, and all 13 iPad FFI regressions pass.
Stable application installations and environments were not replaced or restarted.

### Legacy full-preview inventory

The inspected local archive is
`Trames-Android-13-arm64-preview-240bc18.tar.gz`: 1,040,917,377 compressed bytes
(1.04 GB), with approximately 2.43 GB of regular-file payload before import.
This is an optional complete Android userspace image, not just a driver archive.
The system/vendor pair is Waydroid LineageOS 20 / Android 13, API 33, ARM64,
VANILLA system and MAINLINE vendor, dated 2026-09-27.

| Included layer | Contents and role |
| --- | --- |
| Original Android userspace | Init, Bionic/linker, ART, APEX modules, framework, Binder service managers, PackageManager, netd and other system services |
| Original desktop and apps | SurfaceFlinger, Waydroid HWC, launcher/SystemUI/Settings, DocumentsUI, Jelly browser, WebView, calendar, calculator, clock, contacts and other original system APKs |
| Original graphics libraries | ANGLE/EGL/OpenGL ES and Waydroid display components; ANGLE uses the Trames Vulkan frontend |
| `vulkan.trames.so` | 224,464-byte ARM64 frontend to the shared native Vulkan forwarding path |
| `gralloc.trames.so` | 8,328-byte ARM64 allocator/native-buffer adapter |
| `trames-hal-service` | 8,584-byte ARM64 launcher for the image's original audio HIDL implementations; host audio is not accepted |
| Configuration and provenance | Vulkan/ANGLE/Gralloc properties, `/.trames-android.json`, image/component hashes and graphics contract version; adjacent JSON and SHA-256 sidecars describe the archive |
| Source and license material | Original image material plus Trames GPL text and separate iPad/runtime source exports under `/.trames-android/sources/` |

The archive excludes mutable `/data`, device, proc/sys, runtime, storage and
cache contents. Installed Geekbench/other added APKs, their data, benchmark
results and diagnostic fixtures are not included. The VANILLA image has no
Google Play/GMS. Presence of an original system app or service does not establish
host camera, microphone, telephony, external-storage or other hardware support.

Binder, Linux syscall compatibility, networking, guest scheduling and memory,
the native GPU backend and host window/input bridges live in the Trames host
application/shared runtime, rather than being another kernel in this archive.
Each imported environment receives its own persistent data; transient mounts,
devices and boot overlays are prepared by the host at startup.

The archive records source `240bc18` and runtime `37ea838` as **build provenance**;
its bundled source exports are not the executing runtime. Its three adapter
hashes still match the locally built adapters. Network acceptance and the
WebView clock fix require the newer tested host, currently runtime `4333558`;
they do not require rebuilding the unchanged Android APKs/HALs. The archive's
legacy `network/audio not accepted` limitation predates network acceptance:
IPv4 outbound is now tested on Mac when explicitly enabled, while host audio
remains unaccepted. The new adapter package records current provenance and
separates these limitations; this old full package is retained for compatibility. This remains a local preview; physical iPad Android
acceptance and general app/hardware conformance are pending.

The official Geekbench 6.7.1 Android APK (464,329,585 bytes, SHA-256
`e3b1a98c5edae1ef03106b8a0c25f13953b6550ddf5db6ae968179f3b4ef7249`)
installed through unchanged PackageManager in 5.198 seconds and cold-launched
its real HomeActivity in 5.885 seconds. Its EULA/privacy acceptance was explicitly
authorized. The profile now selects original `dex2oat64` on this ARM64-only target;
an original `cmd package compile -m speed -f` completed successfully in 3.766
seconds instead of attempting the unsupported 32-bit compiler.

Android app networking now has end-to-end acceptance in the isolated Mac
product target: the original EthernetTracker/IpClient/netd configure `eth0`,
publish network 100, and ConnectivityService reports `VALIDATED` after real
HTTP/HTTPS probes. A Bionic probe with a non-root application identity and the
original Android `inet` supplementary group resolves through all four ordinary /
network-bound, `AI_ADDRCONFIG` on/off entry points. The original Geekbench CPU
and Vulkan runs have completed all workloads; final scores and the result-page
compatibility fix are recorded below. No APK networking or license check
is patched or bypassed.

Networking stays explicitly selectable: the preview default remains `off`.
For an isolated Mac environment use `trames -e android config set network outbound`
and restart that environment. Existing explicit settings are retained.
FUSE/emulated external storage remains an independent compatibility limitation.

`tests/integration/android-apk.py` retains the direct PackageManager test;
`tests/integration/android-target.py` exercises normal target creation, CLI APK
streaming, malformed-APK failure and app/data persistence across a clean restart.

Validation for this target/APK batch: shared runtime macOS 7/7 and native ARM64
Linux 7/7; Mac product CI 12/12 including both channel builds/package checks;
normal Linux CLI configuration/rootfs/PATH/streaming regression; 13 iPad FFI
tests, iOS cross-check, final static library and unsigned iPad app build. The
normal Android target test installs/lists the official APK, resolves and cold
launches its HomeActivity (`Status: ok`, 9.130 seconds in that run), rejects an
invalid APK without breaking later commands, and retains the application plus
a private data marker across stop/start. These are local checks before source
integration; no stable app/environment was installed, restarted or replaced.

The initial target is an ARM64 Android desktop and APK execution using an
existing Waydroid system/vendor pair. Reuse upstream container adaptations,
ART, framework, SurfaceFlinger and HWC. Trames supplies the Linux interfaces and
host device bridges. This does not introduce a full Linux-kernel VM or a second
Android-specific kernel implementation. Common resources belong in the shared
runtime; Mac HVF and the iPad interpreter use those same resources.

## Image and boot contract

- Start with one matching ARM64 Waydroid system/vendor pair. The inspected source
  uses `lineage-20` / Android 13. Record the actual image digests, ABI, build
  fingerprints, vendor API and upstream provenance before accepting binaries.
  Source patches listed below do not prove that a particular downloaded image
  contains them. Do not combine an unrelated redroid system with a Waydroid
  vendor image.
- Import read-only system/vendor trees and a private writable `/data` into the
  existing VFS. Preserve modes, owners, links and necessary extended attributes.
  Image extraction is an installation operation, not a block-device emulator in
  the normal file path. Do not download/build a complete AOSP source tree for
  this first acceptance.
- Add an opt-in Android boot profile alongside shell/systemd. It prepares the
  mounts, devices, boot metadata and Waydroid properties expected from the
  container host, then executes the image's existing init. Inspect the selected
  image to select the correct first/second-stage entry; do not invent an argv
  contract from the patch names.
- Configure display and memory transport explicitly. The current virtual DRM
  render node implements discovery only; its existence must not select a GBM
  backend that expects a real Linux GPU driver.
- Initial rendering may use the image's existing software/shared-memory route
  for boot verification. Vendor API 33's upstream fallback uses ANGLE with the
  Pastel software Vulkan driver. Native GPU acceptance is a separate milestone.

## Foundation inventory

"Existing" means code is present, not that Android compatibility has passed.
"Conditional" work is promoted when a selected-image probe demonstrates a real
dependency; it is not silently treated as unnecessary for boot.

| ID | Component | Current audit | First implementation / acceptance boundary |
| --- | --- | --- | --- |
| A0 | Image import and Android boot profile | Matching binary system/vendor rootfs imported with original ownership. Shared Android preparation and experimental `--android-init` run original init with fresh device/property state and retained `/data`. | Original full init, system_server and Launcher3 boot on Mac. Keep this opt-in; no Android catalog entry or iPad desktop acceptance yet. |
| A1 | Binder IPC | Shared 64-bit Binder foundation implemented; original Waydroid service-manager/client smoke test passes. | Provide separate binder/hwbinder/vndbinder contexts, the 64-bit device UAPI, transaction mappings, object/FD transfers, nested synchronous calls, ordered one-way delivery, death notifications, poll/wait and original process/thread/FD cleanup. See the complete subsystem contract below. |
| A2 | Android shared memory | memfd, FD transfer and `F_SEAL_FUTURE_WRITE` are implemented, including existing VMA write rights. | Validate new write/mapping denial and retained old shared mappings through the canonical memory model. Verify the selected libcutils memfd detection. Add ashmem-on-memfd ioctls or a legacy ashmem device only if the selected image actually requires them. |
| A3 | SELinux boot compatibility | Disabled: no SELinux LSM or selinuxfs; security xattrs report absence on reads and reject writes. `/proc/cmdline` includes `selinux=0`. | Use the existing Waydroid no-SELinux adaptations. Do not expose an `enforce=0` pseudo-filesystem or claim permissive/enforcing policy support. |
| A4 | Boot metadata, logging and basic ABI | Original bootstrap Bionic, init, flattened APEX activation and generated linker configuration work. Per-boot command line supplies container hardware identity and disabled SELinux. | Continue with ART-used synchronization and actual framework/runtime callers. Host device enumeration is not exposed; boot preparation records completion of its own supplied device set. |
| A5 | Unix IPC and credentials | Original init property socket, non-root logd and system-UID servicemanager work; real getprop/setprop and log/logcat round trips pass. | Continue with remaining services, SO_PEERGROUPS/security-context callers and full app credentials. Peer identity remains the guest's original credentials. |
| A6 | Task profiles and memory-pressure interfaces | cgroup v2 membership, pids, freezer/kill, mutable original cgroup UID/GID/mode and oom_score_adj exist. CPU/memory/cpuset controllers, PSI and pidfd remain incomplete. Original LMKD control/reply handling runs with upstream pressure monitors explicitly disabled. | Resolve the selected task_profiles/cgroups configuration and provide a real LMKD pressure path. Unsupported v1 controller mounts still return errors; metadata support does not claim CPU/memory controller implementation or unrestricted cgroup delegation. |
| A7 | Android sync fences | Real Linux sync_file/sw_sync create, increment, merge, status, timestamp and final-OFD cancellation are implemented and tested with original libsync. | Original libsync passes timeline/merge/status/close/fork/wrap/copyout cases. Native-image ownership remains retained until real producer/consumer completion. |
| A8 | Waydroid HWC, Wayland and input | Original Waydroid HWC presents retained native images through android_wlegl. Its EventHub input FIFOs receive native mouse/keyboard events. Native-image cursor subsurfaces are selected by default; iPad touch is not accepted. | Launcher and Calendar display through the native-image route; Mac mouse/keyboard input is accepted. iPad touch, broader window lifecycle and the software-cursor fallback remain unaccepted. |
| A9 | Native Android graphics buffers | Bionic Vulkan/Gralloc HALs, native-buffer and selected AHardwareBuffer import reuse the command bridge and canonical native images. Original ANGLE renders the desktop through Vulkan on the Apple GPU. | Selected RGBA/BGRA native-buffer, AHardwareBuffer import and SurfaceFlinger presentation work. General export, protected/YUV and full Android graphics conformance remain incomplete. |
| A10 | netd and networking | Shared TCP/UDP/DNS, routing/netlink and netfilter resources exist; full Android eBPF integration does not. | Reuse Waydroid's no-BPF configuration/patches where present, while returning honest unsupported results from absent kernel APIs. Verify IP configuration, app DNS and actual app traffic. Do not infer netd compatibility from Docker networking. |
| A11 | APEX, vold and external storage | Original apexd bootstrap activates the selected image's flattened modules; original linkerconfig generates their namespaces. VFS/bind mounts and private persistence are reusable. | POSIX default ACL inheritance lets original vold unlock private credential-encrypted user storage. /dev/fuse and emulated external storage remain missing; media/gallery services still show ANRs. Full AVB, OTA, disk encryption and phone block-device setup are outside this milestone. |

The original shared-memory audit found that `F_SEAL_FUTURE_WRITE` was rejected.
It is now implemented: writes/pwrites and newly writable shared mappings fail,
while existing mappings retain their original maximum permissions. Size changes
remain possible unless GROW/SHRINK seals are also present. The same-source native
Linux/Trames fixture tests this behavior. Current upstream
[libcutils probing](https://android.googlesource.com/platform/system/core/+/refs/heads/main/libcutils/ashmem-dev.cpp)
also checks ashmem compatibility ioctls on memfds. The chosen Android 13 graphics
and shared-memory callers still need their own acceptance; do not assign every
newer-source probe to older binaries or add a legacy ashmem device speculatively.

## Binder as one complete subsystem

Implement and validate A1 with its memory, descriptor, wait and exit dependencies
as one integration batch. A successful version ioctl or ping transaction is not
the acceptance criterion.

The resource model needs original binder contexts, per-open process state,
thread/looper state, nodes, references, transactions and receive-buffer ownership.
FD aliases/transfers, fork/exec, unmap and process exit must follow the original
Linux driver contract; a reused numeric PID or FD cannot retarget an accepted
operation. Do not substitute a Unix socket RPC protocol for Binder's device ABI.

The initial command set includes BINDER_VERSION, BINDER_WRITE_READ, context
manager registration, maximum-thread controls and thread exit; transaction,
reply, buffer-free, looper, reference-count and death-notification BC/BR commands
must work with real libbinder/libhidl clients. Treat other ioctl commands and
features according to the pinned image's actual probe behavior. Do not advertise
freeze, security-context delivery or spam detection before their semantics exist.

Required tests include:

- Independent binder/hwbinder/vndbinder contexts and concurrent Android instances.
- An original service manager registering a service and a separate process
  resolving and calling it through the unchanged guest Binder library.
- Synchronous replies and nested A-to-B-to-A calls with one and multiple CPU slots.
- Ordered one-way calls to the same node while unrelated nodes continue running.
- Strong/weak reference transfers, death registration/clear/acknowledgement and
  service/client death while transactions and buffers are retained.
- FD transfer preserving the same OFD, capabilities and close-on-exec contract;
  close/reuse, fork/exec, bad offsets, partial commands and failed imports must
  leave correct original ownership and completion state.
- mmap/unmap, buffer reuse, signal interruption, cancellation, peer exit and final
  resource reclamation. A reserved virtual receive region is not resident RAM.

Use the pinned Linux Binder UAPI and native Linux differential tests where a
Binder-capable reference is available. Linux reference tests are test tooling,
not a replacement for Trames' userspace kernel.

## Implemented Binder foundation

The common runtime owns the implementation in
[`trames-vfs/src/binder`](https://github.com/kb5000/trames-runtime/tree/main/crates/trames-vfs/src/binder),
with the retained ioctl read adapter in
[`trames-linux/src/direct/binder.rs`](https://github.com/kb5000/trames-runtime/blob/main/crates/trames-linux/src/direct/binder.rs).
Both products use these resources; the interpreter and HVF remain CPU backends.

- `/dev/binder`, `/dev/hwbinder` and `/dev/vndbinder` have independent contexts,
  shared by the original environment and all its worker path views.
- The 64-bit protocol includes synchronous/nested replies, per-node ordered
  one-way calls, strong/weak object translation, flat FDs, SG parent fixups/FD
  arrays, loopers, thread exit, death/clear/acknowledgement and node queries.
- Binder receive mappings use canonical lazy shared pages, including when the
  guest requests `MAP_PRIVATE`. They cannot gain write permission or be inherited
  by a process fork. Unmapping releases logical VMA lifetime independently of
  retained physical CPU snapshots; process/thread exit also retires Binder state.
- Ready requests finish on their original caller without allocating a parked
  request. Blocking reads subscribe, recheck and surrender the existing execution
  slot. Already consumed write commands are retained rather than replayed.
- Epoll admission/readiness follows the original Binder resource. A context
  manager's pinned null context object never receives null-pointer reference
  notifications. FDA cleanup tracks the actual destination slot, including reuse
  of a number for the same OFD, and applies original process-lock close semantics.
- Freeze, transaction security-context delivery and one-way spam detection are
  not advertised or faked. Binderfs control and legacy ashmem compatibility remain separate from this
  selected-image desktop acceptance.

The first inspected binary image is the official ARM64 VANILLA build
`lineage-20.0-20260927-VANILLA-waydroid_arm64-system.zip` (912,267,974 bytes), SHA-256
`5fb8179ed4455f45919c5a2bfbfdd1591619295994ad321217f03bb1dcfc53a7`.
Its system fingerprint is
`waydroid/lineage_waydroid_arm64/waydroid_arm64:13/TQ3A.230901.001/aleasto09270452:userdebug/test-keys`.
The initial Binder smoke test imported only the required executables/libraries.
The later desktop acceptance uses the complete matching system/vendor pair below.
Neither stage uses a complete AOSP checkout or rebuild of the framework.

The same ARM64 static
[`binder-foundation.c`](https://github.com/kb5000/trames-runtime/blob/main/tests/fixtures/binder-foundation.c)
passes against the Linux Binder driver and Trames. It covers nested callbacks,
OFD transfer/close/reuse, FD_CLOEXEC, death, VM_DONTCOPY and future-write sealing.
The separate
[`android-binder-supervisor.c`](https://github.com/kb5000/trames-runtime/blob/main/tests/fixtures/android-binder-supervisor.c)
runs the image's unchanged Bionic linker, libraries, `servicemanager` and `service`
clients in one guest environment, and requires both a registered `manager` in
`service list` and `Service manager: found` from a separate `service check manager`
process. A successful process exit alone is insufficient.
The same supervisor also requires the image's original `toybox getenforce` to
report `Disabled`.

Resource tests also cover independent contexts, one-way progress across nodes,
SG/FDA rollback, malformed offsets, signal cancellation, concurrent references,
worker-open context identity and retained-snapshot unmap lifetime. The interpreter
executor test runs Binder round trips with 1, 4 and 12 execution slots and requires
zero coordinator service requests and zero per-mm checkpoints for those calls.
The full static fixture additionally uses ordinary fork/mmap/munmap/exit, whose
existing structural memory paths are outside that no-checkpoint IPC assertion.

### Validation at Binder integration

| Layer | Result |
| --- | --- |
| Binder/memfd resources | 13 Binder tests and the future-write seal test pass, including concurrent reference updates, FD-slot reuse and logical unmap lifetime. |
| Session kernel conformance | 489 tests pass; three existing tests remain ignored. The former rejection of the newly supported seal was updated and its affected test rerun. |
| Native Linux comparison | The same ARM64 static Binder fixture passes on the Linux Binder driver and the interpreter. |
| Mac hardware execution | The static Binder fixture and original Android service supervisor both pass with 1, 4 and 12 CPU slots. Their execution metrics contain zero coordinator service requests and no cohort-return syscalls. |
| Interpreter execution | The original Android service supervisor passes with 1, 4 and 12 CPU slots. The focused IPC executor test also requires zero per-mm checkpoints. |
| Complete local CI | Common runtime macOS 7/7 and Linux 7/7; Mac product 12/12, including both channel builds and packaging. |
| iPad integration | `trames-ffi` checks successfully for `aarch64-apple-ios`; these interpreter tests do not constitute an Android boot on a physical iPad. |

The real Bionic binaries exposed one CPU-backend gap: Mac HVF rejected
execute-only mappings. The Mac backend now represents these with ARM64 AP=10
and UXN clear. A signed hardware test verifies instruction execution, denied
guest data loads and RX-to-X permission changes. This change is isolated from
the common Binder implementation and the iPad interpreter.

## Original init and platform dependencies

The matching vendor image is
`lineage-20.0-20260927-MAINLINE-waydroid_arm64-vendor.zip` (147,678,746 bytes),
SHA-256 `20a49c11961a5ac7295320f578764eeab3c982b217f1bfde7473adfc22e747ae`.
It is paired with the system image recorded above. Binary images, not an AOSP
checkout, supply the platform programs, libraries, ownership and symlinks.

[`android::prepare`](https://github.com/kb5000/trames-runtime/blob/main/crates/trames-distro/src/android.rs)
provides fresh `/dev`, property/socket state and per-boot metadata while keeping
`/data`. It overlays generated command-line and Waydroid property files through
ordinary guest bind mounts, retaining the original vendor property inode. It
fills absent container defaults, selects the supported ARM64 ABI/zygote and
records completion of its provisioned device set. SELinux remains disabled.
This cold preparation runs before the original `/system/bin/init second_stage`;
it does not replace init's parser, property service, logging or APEX daemon.

Related Linux support now includes tmpfs `uid`/`gid`, genuine shared/slave mount
propagation, private detachment and inheritance across mount namespaces. Busy
propagated recipients reject an ordinary unmount before any namespace changes;
downstream slaves reparent when their immediate master disappears. The identical
[`android-mounts.c`](https://github.com/kb5000/trames-runtime/blob/main/tests/fixtures/android-mounts.c)
passes on native Linux, HVF and the interpreter. Structural work uses a lease
shared by related namespaces, with original readiness for parked callers.
Private-only unshare creates an independent domain. No coordinator/service
thread, host mount or new path in ordinary file IO is introduced. Moving shared
mounts remains explicitly unsupported; this is not complete Linux mount CTS.

The common resolver installer follows guest directory symlinks such as
`/etc -> /system/etc`, preserving existing configuration. cgroupfs metadata
changes use the actual control inode, permissions and read-only mount rules;
they do not enable ordinary truncation/control writes or persist virtual files
into the rootfs manifest.

The acceptance boot script imports the image's original service definitions and
runs original `apexd --bootstrap`, linkerconfig, logd and servicemanager. Its
static client then invokes the actual image entry points (`getprop`/`setprop`
are toolbox links in this image), validates property and log round trips, checks
Binder discovery and requires `getenforce` to report `Disabled`. The result
requires fresh per-boot property state as well as a newly produced marker.
The fixture never sets `sys.boot_completed` or pretends to start the framework.

Use [`tests/integration/android-foundation.py`](../../tests/integration/android-foundation.py)
with a disposable owner-preserving rootfs/store. The Mac debug runner's optional
`--exec-socket` exposes its existing worker-based process admission independently
of a shell console. It adds no service thread and lets the test query the live
Android PID 1 environment. Explicit syscall tracing also prints retained guest
kernel logs at exit; normal runs do not perform that diagnostic dump.

At foundation integration, the combined acceptance passed with 1, 4 and 12 Mac CPU slots: original
init/APEX/linkerconfig, property/log round trips, Binder discovery, SPHAL module
discovery and native Apple M6 Vulkan compute. Each run records zero coordinator
service requests and an empty cohort-return syscall list. The identical mount
fixture additionally passes with 1, 4 and 12 slots on both HVF and the interpreter,
including busy-peer rejection and downstream-slave reparenting, and on native
ARM64 Linux. Common-runtime local CI passes 7/7 on macOS and 7/7 on Linux; Mac
product CI passes 12/12. Interpreter kernel checks pass 489 tests (3 ignored),
and the iOS embedding checks for `aarch64-apple-ios`. These foundation results alone did not imply a desktop boot. The later Mac
desktop milestone below still does not validate a physical iPad Android desktop.

## SELinux scope: disabled

The owner selected disabled mode, not permissive mode. Trames exposes no
selinuxfs or SELinux filesystem type, rejects attempts to mount it, and reports
missing security labels rather than manufacturing contexts or a policy engine.
The generic boot view records `selinux=0`; no `enforce=0` view is synthesized.

Use Waydroid's existing no-SELinux init/service adaptations, including its omission
of Binder security-context requests. Keep unsupported APIs honest. This milestone
does not load policies, perform enforcing checks or claim Android security-policy
conformance. Android boot metadata and remaining service dependencies will be
added from actual image failures rather than inventing permissive interfaces.

Binderfs is similarly conditional: private device contexts are required, but
mounting binderfs and BINDER_CTL_ADD are only needed if the chosen launcher/image
uses them. A direct Trames bootstrap can provision the three contexts without
requiring a port of the entire LXC host manager.

## Concurrency, memory and performance rules

- New resource work executes on the same existing worker that executes the
  guest. Binder waits, PSI/pidfd waits and fence waits subscribe to the original
  resource and surrender the CPU slot. No new coordinator, resource-service
  thread, polling bus or global guest pause is introduced.
- Synchronize actual multi-field invariants by context/process/node/transaction.
  Keep scalar state atomic where appropriate; do not hold resource locks while
  waiting for a guest reply or callback. Preserve Binder's real ordering and
  transaction requirements without serializing unrelated nodes or processes.
- Reuse canonical guest memory, descriptor transfer and completion resources.
  Binder payload copies required by its protocol are distinct from avoidable
  host staging copies. Do not promise zero-copy Binder transactions.
- F_SEAL_FUTURE_WRITE prevents future writes/new writable shared mappings while
  retaining existing writable-map rights as Linux permits. It must not revoke
  old mappings globally or be accepted without its actual effect.
- Binder pools, ART heap reservations, graphics and memfd views allocate backing
  on demand and release it with real ownership. Do not add arbitrary global
  socket-like quotas or count reserved address space as physical consumption.
- For LMKD, counters and notifications must describe the available guest/host
  memory model. Do not expose a permanently zero PSI file. This experimental
  profile keeps the real control protocol alive with upstream monitors disabled;
  it does not implement Android pressure-based app killing or claim otherwise.
- Initially use Waydroid's EventHub input-FIFO adaptation when applicable. Do not
  implement an entire evdev/uinput stack just because generic Android uses it.
- Do not force raw GPU image readback/copy into every frame. Software/shared-
  memory presentation is a bring-up fallback; native rendering must eventually
  preserve shared image ownership and actual completion/release fences.

## Integration batches and acceptance

1. **Foundation:** pin/inspect the image, probe Bionic and prepare the boot
   profile; integrate complete Binder with FD/memory/wait/exit dependencies,
   Android shared-memory semantics and the demonstrated security/metadata ABI.
   Concentrate tests after the subsystem work rather than one ioctl at a time.
2. **Real init and runtime:** run existing init, property service, logd and
   service managers. Verify getprop/setprop and logcat, real cross-process Binder
   calls, and an ART command-line Java program. Resolve APEX, task-profile,
   pidfd/LMKD and network dependencies from actual blocking failures.
3. **Complete desktop:** use existing Waydroid HWC with the validated fallback
   to start SurfaceFlinger, Zygote, system_server and Launcher. Require stable
   `sys.boot_completed=1`, independent logcat/ADB access and working app input;
   repeated crashes/restarts are failures even if a boot property is set.
4. **Application and native devices:** install and launch a small real APK,
   test DNS/network, activity lifecycle and persistent data after restart. Then
   enable the native allocator/Vulkan/fence path and later audio/video bridges.
   Mac is the first full-system target; common fixtures also exercise the iPad
   interpreter before iPad end-to-end acceptance.

Each implementation batch gets focused resource tests, same-source Linux
comparisons where applicable and actual guest binaries. Run complete local CI
at integration/release boundaries. Reuse unchanged passed results when only
documentation or test-recording details change.

## GPU route: reuse execution, add Android buffer interoperability

The Android Vulkan HAL now reuses the current Vulkan command backend. The
original Android loader executes verified native GPU compute through this
frontend. The desktop now uses selected Android native-buffer/Gralloc contracts and real
completion handling across its producer, SurfaceFlinger and original HWC.

| Responsibility | Current implementation | Android addition |
| --- | --- | --- |
| Draw/compute commands and shaders | Guest Vulkan bridge to native Vulkan/MoltenVK/Metal | Reuse the same backend; package the guest side for Bionic and Android's Vulkan driver-loading ABI. |
| Renderable image allocation and sharing | Retained native-image FDs, selected import-only image-plane support, backend-owned Wayland swapchain images | Extend allocation/export of renderable native images; provide matching Gralloc/Mapper/native-handle metadata and Android native-buffer import. This does not require arbitrary VkDeviceMemory export. |
| Cross-process completion | Native Vulkan fences/timelines and internal image completion leases | Provide acquire/release sync-file FDs that travel with buffers over Binder and remain connected to real native GPU completion. |
| Desktop presentation | Existing Wayland server and native presenter | Reuse Waydroid HWC, add its required buffer protocols and import the same retained native images. |

Android's Vulkan loader has a different device-driver and native-window
integration contract from the current Linux ICD/JSON path. A Bionic recompile
alone is insufficient. Audit the selected platform's HAL entry points and
Android native-buffer hooks, including gralloc usage queries, image acquisition
and release-fence production; public AHardwareBuffer import is related but is
not a substitute for all of that loader/WSI contract. Only advertise actual
implemented extensions and buffer formats.

The intended ownership flow is:

1. Gralloc allocates a native-backed image and returns its guest FD/handle and
   actual format, dimensions, row stride and supported plane metadata.
2. The app renders into that same allocation through the Vulkan bridge, directly
   or through the chosen Android EGL/GLES implementation.
3. BufferQueue/Binder transfers its handle and acquire-fence FD to the consumer.
   Binder moves descriptor references and metadata, not pixel contents.
4. SurfaceFlinger and Waydroid HWC compose/present using the retained allocation;
   the native presenter releases it only after its actual consumer use ends.
5. The release fence controls reuse by the producer. Closing one FD or returning
   from an API call alone must not allow reuse while GPU/UI work is pending.

CPU lock/unlock and CPU/GPU coherency are required when a supported allocation
is actually CPU-accessed. Do not force CPU readback, an ordinary memfd shadow or
whole-frame byte copies into the standard GPU presentation path. Necessary GPU
composition is distinct from an avoidable CPU copy.

### What the DRM node needs

Current `crates/trames-linux/src/render.rs` supports a discovery/version query
only. It is not a GEM/GBM allocation, PRIME sharing or GPU-submission driver.
Adding a device node or a plausible driver name cannot make an existing Mesa
hardware driver work.

Prefer a small Trames allocator backend under the existing Gralloc interface,
retaining Waydroid's HWC and the current native image resource model. That path
does not require routing draw/dispatch commands through DRM. Keep the render
node discovery-only until a chosen userspace backend demonstrates a need for
additional operations; select Android driver/allocator properties explicitly.

If preserving a GBM interface makes a thin DRM buffer adapter useful, its scope
is supported capability queries, original per-open buffer handles/lifetime,
allocation and mapping for supported usages, PRIME-style FD import/export and
the actual synchronization operations used by that backend. Allocation calls
can be backend-specific; render nodes are not KMS dumb-buffer allocators.
Advertise only real capabilities. A thin buffer adapter still does not run
unmodified Intel/AMD/virtio GPU command-submission drivers.

Waydroid HWC presents through Wayland, so this route does not need DRM master,
connectors, CRTCs, planes or KMS atomic modesetting. The existing host presenter
continues to own physical display integration. No new DRM coordinator or GPU
service thread is introduced; buffer and fence ownership remain ordinary shared
runtime resources on existing workers.

References: [Android Vulkan loader/driver architecture](https://source.android.com/docs/core/graphics/implement-vulkan),
[BufferQueue and Gralloc](https://source.android.com/docs/core/graphics/arch-bq-gralloc),
[Linux render-node and PRIME interfaces](https://docs.kernel.org/gpu/drm-uapi.html#render-nodes).

### Independent GPU component builds

The standalone build is implemented in
[`scripts/build-vulkan-android.py`](../../scripts/build-vulkan-android.py), using
[`accelerator/android/vulkan.c`](../../accelerator/android/vulkan.c). The small
HAL wrapper exports Android's HMI/device contract and directly includes the
existing guest wire encoder. Linux's ICD/JSON frontend remains unchanged. The
Gralloc0.3, native-buffer version 5 and selected public AHardwareBuffer imports
are implemented. Advertised capabilities are limited to the supported contracts
and formats described below.

[`scripts/prepare-android.py`](../../scripts/prepare-android.py) verifies the
pinned official NDK r27d archive and Android 13 public/platform C headers.
It extracts about 18 MB of Bionic headers/API 33 link stubs, rather than the NDK
compiler, and uses native host Clang plus the existing Rust ELF linker. No
x86 compiler execution or complete AOSP checkout is involved. The linked guest
uses the image's original libc/libvulkan at runtime; link stubs are not shipped
as implementations. Source pins, compiler versions, recipes and output digests
are recorded in the generated manifest.

The original Android loader selects `vulkan.trames.so` through
`ro.hardware.vulkan=trames` and the SPHAL namespace. Native test executables are
installed in `/system/bin` so that the original generated linker configuration
selects that system namespace. A binary in an arbitrary root directory has a
different namespace and must not be treated as equivalent HAL-discovery proof.
No Android loader or image executable was patched to make this work.

On the native Apple GPU, the existing compute fixture passes through original
Android libvulkan: three 4096-element dispatch rounds, two specialized semaphore
chains, active/concurrent pipeline callers, semaphore reuse and memory
mapping/flush/invalidate checks. That fixture validates headless compute. Separate desktop and original ANGLE
fixtures now cover native windows and SurfaceFlinger; no Vulkan/AHB CTS claim is
made for Android.

For a repeatable local run (using an isolated, owner-preserving rootfs/store):

```sh
# Native ARM64 Linux or a Linux ARM64 cross compiler:
cc -static -O2 -Wall -Wextra -Werror \
  runtime/tests/fixtures/android-init-probe.c -o /path/to/android-init-probe
glslangValidator -V tests/fixtures/vulkan-compute.comp -o /path/to/compute.spv
python3 scripts/build-vulkan-android.py --shader /path/to/compute.spv
python3 tests/integration/android-foundation.py \
  --runner /path/to/signed/trames-mac-run \
  --store /path/to/disposable/android-store \
  --rootfs /path/to/owner-preserving-waydroid-rootfs.tar \
  --probe /path/to/android-init-probe --cpus 1,4,12 \
  --vulkan-driver target/vulkan-android/vulkan.trames.so \
  --vulkan-probe target/vulkan-android/vulkan-compute \
  --hal-probe target/vulkan-android/vulkan-hal-probe \
  --shader /path/to/compute.spv --output target/android-acceptance
```

Continue reusing the selected image's SurfaceFlinger, HWC, interface libraries
and vendor ANGLE. Build only the small native-image allocator/mapper frontend
needed to connect them to the existing host resources.

Waydroid's Android.bp/Android.mk files describe platform builds; the hardware
repository is not a self-contained NDK project. Provide a standalone CMake or
equivalent build using the NDK toolchain, selected platform/HAL headers, generated
interface headers where needed, and matching prebuilt libraries from the pinned
system/vendor image. Avoid private C++ platform dependencies in the small C ABI
bridge where practical.

If HWC later needs changes, its upstream implementation can also be built this
way. The [waydroid-nvidia standalone build](https://github.com/Shiro836/waydroid-nvidia/blob/main/hwcomposer/build.sh)
and [dependency provisioning](https://github.com/Shiro836/waydroid-nvidia/blob/main/hwcomposer/provision.sh)
provide a concrete reference using selected AOSP repositories/headers, HIDL and
Wayland code generation, static guest dependencies and image-derived link inputs,
without the AOSP build system. That reference targets x86_64 on Linux; it is not
an ARM64/macOS validation result. Our target triple, API level, host tools and
private platform/C++ ABI must match our selected ARM64 image.

Pin source revisions, NDK version, generated inputs and image digests. Link-time
stubs must not become shipped runtime implementations. Retain Android linker
namespace/VNDK compatibility and do not replace the image's platform C++ runtime
with an unrelated NDK runtime merely to satisfy the linker. Validate ELF imports,
loader/HAL discovery, native-buffer interoperability and actual pixels after
linking; producing a .so alone is not GPU acceptance.

ANGLE normally comes from the selected vendor image. If it needs rebuilding,
use its own [Android GN/Ninja build](https://chromium.googlesource.com/angle/angle/+/HEAD/doc/DevSetupAndroid.md)
and pinned dependencies, not a full AOSP checkout. The documented Android ANGLE
build currently requires Linux; verify host-tool architecture separately rather
than assuming the current ARM64 Linux CI instance is an upstream-supported host.
The APK-based app opt-in ANGLE route is not a replacement for the image's native
vendor EGL/GLES integration used by SurfaceFlinger and HWC.

## Upstream evidence inspected

Waydroid sources were inspected on `lineage-20`. The pinned binary pair now
passes the scoped foundation/headless tests above; complete desktop and APK
acceptance remain pending.

- [Container init without SELinux](https://github.com/waydroid/android_vendor_waydroid/blob/lineage-20/waydroid-patches/base-patches-33/system/core/0001-waydroid-init-start-inside-LXC-container-without-SEL.patch)
- [Service-manager SELinux adaptation](https://github.com/waydroid/android_vendor_waydroid/blob/lineage-20/waydroid-patches/base-patches-33/frameworks/native/0002-halium-disable-SELinux-checks-in-ServiceManager.patch)
- [Binder security-context adaptation](https://github.com/waydroid/android_vendor_waydroid/blob/lineage-20/waydroid-patches/base-patches-33/frameworks/native/0003-halium-never-set-FLAT_BINDER_FLAG_TXN_SECURITY_CTX-f.patch)
- [Host-provisioned Binder devices](https://github.com/waydroid/android_vendor_waydroid/blob/lineage-20/waydroid-patches/base-patches-33/system/core/0016-init-Do-not-try-to-mount-binderfs.patch)
- [Early memfd selection](https://github.com/waydroid/android_vendor_waydroid/blob/lineage-20/waydroid-patches/base-patches-33/system/core/0007-init-Allow-setting-sys.use_memfd-before-boot.patch)
- [Waydroid no-BPF path](https://github.com/waydroid/android_vendor_waydroid/blob/lineage-20/waydroid-patches/base-patches-33/system/bpf/0002-Support-no-bpf-usecase.patch)
- [HWC buffer and shared-memory paths](https://github.com/waydroid/android_hardware_waydroid/blob/lineage-20/hwcomposer/gralloc_handler.cpp)
- [ANGLE/Pastel fallback selection](https://github.com/waydroid/android_vendor_waydroid/blob/lineage-20/waydroid-patches/base-patches-33/system/core/0013-init-Substitute-swiftshader-with-angle-pastel-on-ven.patch)
- [Linux binderfs semantics](https://docs.kernel.org/admin-guide/binderfs.html)
- [Android LMKD dependencies](https://source.android.com/docs/core/perf/lmkd)


## Native desktop integration (2026-11-13)

The desktop uses the original init, APEX modules, Bionic, ART, framework,
SurfaceFlinger, Waydroid HWC, ANGLE and APKs from the recorded binary images.
Only the small Trames Vulkan/Gralloc frontends and a 64-bit audio-service launcher
are built separately. The audio launcher loads the image's original HIDL 4.0
implementations; it does not replace AudioFlinger or claim working host audio.
No framework executable, APK, Linux GPU driver or full Linux kernel is rebuilt.

### Buffer and fence contract

`gralloc.trames.so` implements Gralloc0.3 allocation, registration, validation,
CPU lock/unlock and asynchronous fence ownership. Supported native allocation
formats are RGBA8888, RGBX8888, BGRA8888 and height-one BLOB. Protected and YUV
allocations are rejected. Handles contain one retained guest FD plus typed
metadata; pitch and allocation length come from the canonical native image.
CPU mappings alias its actual shared backing, including across FD transfer.

The Android Vulkan frontend retains original native-window owners, imports
supported dedicated RGBA/BGRA images and exposes the implemented Android hooks.
Original ANGLE renders through the same Vulkan backend as Linux. Its selected
queue-family view contains the single supported family; original Android's
loader owns WSI. This is not arbitrary device-memory export or complete public
AHardwareBuffer interoperability. BLOB/general buffer import, YUV, protected
content and the full Android format-properties query surface remain incomplete.

The vendored protocol retains its upstream copyright/license. The inspected
Waydroid HWC source revision is `a575cfcc12ab79a122844218f2c6df5bd208cca0`;
container/EventHub adaptation was checked against android_vendor_waydroid
`lineage-20` revision `4dced17d9528ba07f6a6eab11b78a8ed38a36067`.

The upstream `android_wlegl` protocol transports the retained image and actual
geometry to the existing scene/presenter. Producer completion and consumer
leases control reuse and `wl_buffer.release`; a normal frame does not require a
CPU pixel copy. Native-image cursor subsurfaces use the same ownership path.
The original HWC's alternative software-cursor EGL readback path crashed in
bring-up and is not accepted; `persist.waydroid.cursor_on_subsurface=true` is
an absent-only default, with explicit caller settings preserved.

`/dev/sw_sync` supplies real pollable Linux sync-file FDs. Merge coalesces the
latest point in each timeline context, reports real status/timestamps and
retains original final-OFD ownership across dup/fork/Binder/SCM_RIGHTS. Destroying
a timeline reports pending-fence cancellation instead of success. The current
Vulkan release adapter waits for actual producer completion on the existing
resource wait, then returns `-1` for an already completed release; it never
manufactures an early signalled fence or adds a GPU service/coordinator thread.

### Full-init dependencies and observed stalls

- Binder process work and synchronous thread replies have distinct readiness.
  Poll/epoll handles bind to the original calling task and cannot consume another
  thread's reply or revive a retired TID. Completion, owner-reference work,
  nested callbacks, thread-pool growth and node-address reuse follow real driver
  ordering. The same C poll/epoll fixture runs against native Linux Binder.
- A real Java stack identified repeated three-second framework stalls in
  `LmkdConnection.waitForConnection`, while ActivityManager and process locks
  were held. Original LMKD had exited because neither PSI nor v1 vmpressure is
  implemented. The profile sets `ro.lmk.use_new_strategy=true` and both PSI stall
  thresholds to zero. Upstream LMKD deliberately skips those monitors and keeps
  its real priority/control/reply channel operational. **Pressure-based Android
  app killing remains disabled**; host memory management does not replace that
  missing policy. No fake PSI source or dummy LMKD is supplied.
- Waydroid EventHub reads original Linux input_event records from four named
  FIFOs owned by system:input. The container provisions these before boot. The
  original Lineage physical bugreport keychord is omitted from a per-boot RC
  overlay: Android init otherwise opens the FIFOs without O_NONBLOCK before HWC
  and InputFlinger can start, deadlocking service startup. Other service options
  and the bugreport service are retained.
- Queued process/thread signals preserve the AArch64 kernel siginfo payload and
  original permissions, including 64-bit sigval and signalfd conversion. Original
  Bionic and debuggerd now produce actual Java stacks rather than ENOSYS.
- POSIX access/default ACLs, inheritance before umask, chmod mask updates and
  persistence permit original vold to unlock private user storage. This does
  not implement FUSE or make emulated external storage usable.
- Same-mm process_vm transfers, pipe sizing, boot/alarm timerfd clocks and
  nonblocking/timeout Unix-socket behavior use the shared resource model. Netlink
  additions supply real selected route/uevent behavior and honest unsupported
  netfilter replies. All common changes remain available to the iPad interpreter.

### Build and run

Use an isolated owner-preserving rootfs/store made from the matching images.
The foundation test replaces its boot-property overlay, so **use separate stores
for the foundation fixture and the full desktop**. The rootfs must contain the
full original system, system_ext, product, vendor and flattened APEX trees.
Offline `--fs put` preserves host ordinary permission bits (subject to guest
umask), including executable bits; it does not import setuid/setgid. Use a
runner containing this change for executable HAL helpers and probes.

```sh
python3 scripts/build-vulkan-android.py --shader /path/to/compute.spv
python3 scripts/build-android-hal-service.py --rootfs /path/to/waydroid-rootfs.tar

# Use a signed Mac runner. debug run --fs is offline; stop this store first.
RUNNER=/path/to/signed/trames-mac-run
STORE=/path/to/isolated/android-desktop-store
"$RUNNER" debug run --storage "$STORE" --rootfs /path/to/waydroid-rootfs.tar --fs ls /
"$RUNNER" debug run --storage "$STORE" --fs put --replace target/vulkan-android/vulkan.trames.so /vendor/lib64/hw/vulkan.trames.so
"$RUNNER" debug run --storage "$STORE" --fs put --replace target/vulkan-android/gralloc.trames.so /vendor/lib64/hw/gralloc.trames.so
"$RUNNER" debug run --storage "$STORE" --fs put --replace target/vulkan-android/trames-hal-service /vendor/bin/hw/trames-hal-service
```

Supply a vendor overlay containing `ro.hardware.vulkan=trames`,
`ro.hardware.egl=angle` and `ro.hardware.gralloc=trames`. Do not set
`ro.boot.init_rc` for a full desktop: that would select the foundation fixture
instead of the image's real boot scripts. Run with windows enabled:

```sh
"$RUNNER" debug run --storage "$STORE" --android-init --vulkan --cpus 12 \
  --no-pty /system/bin/init second_stage

# Automated framework/APK/probe acceptance; native input is separately manual.
python3 tests/integration/android-desktop.py --runner "$RUNNER" --store "$STORE" \
  --output target/android-desktop-acceptance --fixtures target/vulkan-android \
  --install-calendar
```

The automated test requires actual framework boot completion, resumed original
Launcher, Calendar launch, unchanged composer/framework PIDs and no fatal Java
or native crash. Optional APK acceptance copies the image's original Calendar
APK and reinstalls it into private app storage through the real PackageManager.
The fixtures exercise original libsync, Gralloc async fence ownership, original
ANGLE GPU-to-CPU AHardwareBuffer pixels, queued signals, ACLs and netlink. An ANR
list is retained separately; background media failures are not reported as an
all-app pass. Native mouse/keyboard acceptance additionally opened Calendar,
navigated its search UI and visibly entered `trames keyboard test`.

### Remaining acceptance boundary

- Mac desktop is experimental. iPad shares the kernel and graphics resources,
  but an Android desktop on a physical iPad has not been tested.
- FUSE/emulated external storage and media/gallery boot broadcasts remain
  incomplete; those apps can ANR. A loaded boot also produced a KeyguardService
  ANR before Launcher resumed, so startup responsiveness is not accepted as a
  stable product experience. Network/app DNS, audio output, camera, codecs,
  suspend/resume and arbitrary third-party APKs are not accepted in this run.
- Clean-package first boot also produced a recoverable original SystemUI
  KeyguardService exception in `AppOpsControllerImpl.setListening`. This remains
  a separate cold-start/audio-service compatibility issue; successful APK launch
  and persistence tests do not constitute a crash-free Android system pass.
- The runtime executes ARM64. This image's remaining 32-bit OMX/camera launchers
  cannot run; no ARM32 compatibility is claimed.
- No SELinux LSM, pressure-based Android LMK policy, Android CTS/VTS or general
  Vulkan/AHB conformance is claimed. Stable Trames installations are unchanged.


### Validation for the native desktop batch

Shared runtime revision: [`b227f4e`](https://github.com/kb5000/trames-runtime/commit/b227f4ee653233efaa33675464be117cd88f0fa5).

| Acceptance | Result |
| --- | --- |
| Full shared-runtime local CI | macOS 7/7 and native ARM64 Linux 7/7. |
| Mac product local CI | 12/12, including both channel builds and unnotarized packaging checks. No stable installation was updated. |
| Interpreter kernel conformance | 489 pass, 3 ignored. Reused 487 passes; the new queued-signal legacy oracle and a load-sensitive host-pause case passed their focused reruns. |
| iPad composition | Workspace all-target/all-feature Clippy and `trames-ffi` for `aarch64-apple-ios` pass. No physical iPad Android acceptance. |
| Native Linux comparison | Identical ARM64 C Binder poll/epoll/nested-FD/lifecycle, queued-signal/memory and POSIX ACL fixtures pass. |
| Original Android foundation | Original init/APEX/log/property/Binder, libsync, HAL discovery and native GPU compute pass with 1, 4 and 12 Mac CPU slots. Same native Linux Binder binary passes all three HVF configurations. |
| Full Android desktop | Final windowed run: framework boot at 59.824 s; actual Launcher resumed at 74.437 s. Original Calendar APK installation reports Success. Composer/system_server PIDs remain unchanged through app/probe acceptance and a 20 s settling interval. |
| Original graphics/ABI probes | All seven Bionic probes pass: memory/signals, netlink, ACL, libsync, Gralloc, original ANGLE/EGL and Vulkan HAL. The ANGLE renderer identifies Apple M6/Vulkan/MoltenVK; imported AHardwareBuffer pixels are 64,128,191,255 as expected. |
| Runtime logs | Final accepted run contains no ANR, fatal Java exception or native fatal signal. Earlier loaded boots had the failures recorded above; this is a bounded run, not a longevity claim. |
| Concurrency | Final full desktop metrics: zero coordinator service requests and an empty cohort-return syscall list. Native image presentation uses retained storage. |

Only test artifacts and existing build caches were used; the Linux reference
instance was stopped after its checks. Available disk space remained above
100 GiB. The source/image/toolchain recipes and limitations above are part of
the acceptance record; binary images and local stores are not committed.


## Managed Android IP uplink (2026-11-13)

Android needs ownership of its interface configuration and policy routing;
exporting a read-only snapshot of a host interface is insufficient. The cold
Android session selects a guest-owned IP-only `eth0` over the existing dynamic
userspace packet stack and native gateway. This is shared runtime code; Mac
and iPad select it only for the Android target. Ordinary Linux sessions keep
their existing native socket path. No host NIC, route, sysctl, firewall or
DNS setting is modified, and no network coordinator/service thread is added.

The original Android 13 IpConfigStore v3 file seeds `10.0.0.2/24`, gateway and
DNS `10.0.0.1` only when neither original configuration location exists.
EthernetTracker, IpClient, netd and DnsResolver then perform their normal
configuration, service registration, policy and DNS work. User configuration
is not overwritten. DNS uses the existing gateway resolver and real reply
routes, including original flow ownership rather than an interface-number
assumption for routed namespaces.

The integrated kernel interfaces include IPv4 address/netmask ioctls,
guest-only mutable link/address state, Android policy rules and route tables,
SO_MARK with its NET_ADMIN/NET_RAW authorization, socket creator UID and real
sockfs `fchown` UID/GID changes. The latter updates the original socket's
routing UID and shared descriptor metadata. It stays on the current executing
worker and original socket control gate, even with a disk-backed rootfs.
Unknown marks and prohibited UIDs reject actual socket I/O; these are not
readback-only controls. Legacy non-strict netlink dumps accept unused request
padding from the image's original iproute2. The original zero IPv4 address-add
request succeeds without inserting an address, matching Linux.

UDP connect to destination port zero is a real source-route probe on Linux.
The shared stack now permits it, selects the source address and retains Linux's
ENOTCONN result for getpeername with a zero peer port. This is required by both
Android Java and Bionic address-family detection. Socket ownership transfer is
required by Bionic's per-application source probe: returning EINVAL for fchown
made all non-root getaddrinfo paths report no address even after asynchronous
DNS was working.

The guest IPv6 interface controls retain `accept_ra`, `use_tempaddr`,
`disable_ipv6`, `addr_gen_mode` and stable-secret state. Disabling a device
retires its actual IPv6 addresses and main/custom routes and emits deletions;
new IPv6 address insertion is denied while disabled. Unset stable secrets and
writes to `all/stable_secret` return EIO. This IP-only link has no Ethernet RA
source; these controls do not claim SLAAC, IPv6 Internet, Android VPN/tethering,
eBPF accounting or DNS-over-TLS support. Unsupported facilities continue to
return errors. The accepted external network path is IPv4 outbound.

Validation includes original netd request replay against a native ARM64 Linux
private network namespace; marked, unmarked, unknown-mark and prohibited-UID
route probes; real host TCP/UDP payload and reply delivery; live UID/mark
blocking; DNS gateway replies; IPv6 address/route retirement; and sysctl/socket
metadata authorization and descriptor/namespace lifetimes. The native reference
fixture refuses to run in PID 1's network namespace. Reference request/ACK bytes
are retained in `runtime/tests/fixtures/managed-network-linux-reference.txt`;
`managed-network-reference.py` regenerates them. Bionic validation source is
`tests/fixtures/android-network.c`.


The first full 12-slot CPU attempt reached 48% and then aborted in multi-core
File Compression. Bionic logged `pthread_create: clone failed: Try again`
immediately before Zstd's error assertion. The complete desktop had 83 live
processes with 985 tasks, against the Mac launcher's legacy 1,024-task ceiling;
no zombie buildup was present. Independent original-library Zstd tests pass
single-threaded, internal 12-worker and 12 parallel-context compression. The
Android cold profile now uses 8,192 task admission slots on all product entry
points. This changes only the existing atomic task admission bound; it does not
preallocate guest memory, start more host CPU workers or add a coordinator.
The subsequent full CPU run passes the former failure and all remaining workloads.


### Official Android Geekbench acceptance

The original, unmodified Geekbench 6.7.1 Android AArch64 APK completes CPU and
Vulkan compute in the isolated Mac Android 13 target. The session uses 12 CPU
slots, host-managed memory and the native Apple M6 GPU forwarding path. No
benchmark-specific library interposition, sandbox bypass or APK modification
is used. The installed APK is not distributed with the Android image.

| Test | Saved score |
| --- | ---: |
| CPU single-core | 3483 |
| CPU multi-core | 8952 |
| GPU Vulkan | 43738 |

Scores are preserved by the application's original history database and visible
in its history list. Exported evidence contains only score, API and timestamp
fields, not a copy of the app's private database. No version-matched native-host
run was made for this batch, so these numbers do not establish an overhead ratio.
Image-format queries returning `VK_ERROR_FORMAT_NOT_SUPPORTED` are capability
probes, not failed compute submissions.

The Android default network setting remains `off`; this acceptance explicitly
selects `outbound` for the isolated target. Original ConnectivityService selects
a validated default Ethernet network after HTTP and HTTPS connectivity probes.
The non-root Bionic probe passes ordinary and network-bound getaddrinfo with and
without AI_ADDRCONFIG, including original socket fchown and port-zero source
selection. Static IP seed data does not enable host connectivity by itself.

Shared runtime revision: [`3edbde9`](https://github.com/kb5000/trames-runtime/commit/3edbde917b2d0bbde165743c3c9bd3297ce68955).

The shared-runtime full local workflow passes on macOS and native ARM64 Linux
(7/7 each); the Mac product workflow passes 12/12, including both channel builds
and unnotarized package checks. iPad/CLI workspace all-feature Clippy and the
ARM64 iOS FFI cross-check pass. Physical iPad Android acceptance is still pending.

### WebView thread CPU clock compatibility

The untraced result-page crash was a missing Linux clock ABI. A bounded syscall
history captured `clock_gettime(CLOCK_THREAD_CPUTIME_ID)` returning EINVAL
immediately before Chromium deliberately raised SIGTRAP. The original system
HTMLViewer reproduces the same failure independently of Geekbench. Successful
rendering under full tracing was timing-dependent and did not fix the cause.
The missing crash-handler trampoline is a secondary failure in crash reporting.

Shared runtime [`4333558`](https://github.com/kb5000/trames-runtime/commit/4333558c95574f61c76e3e38eb1b75f13726f28d)
implements clock ID 3 and its resolution query on the calling executor, using
the original guest task's existing execution accounting. The value survives
execution-slot migration and excludes parked time and other guest threads.
There is no new coordinator, lock or accounting sample on normal syscalls or
wall-clock reads. As with the existing `/proc` accounting, this measures charged
execution-assignment time, including host preemption within an assignment; it
is not a strict host kernel CPU-time measurement. Process-wide and encoded
per-thread CPU clocks remain unsupported pending live and retired task accounting.

The original WebView/APKs and sandbox settings are unchanged. Six consecutive
HTMLViewer cold launches pass without syscall tracing. The Bionic
`android-memory` fixture verifies clock resolution, low-32-bit clock IDs,
EFAULT, time growth, parked sleep and isolation from another running thread.
Geekbench's saved CPU (3483/8952) and Vulkan (43738) detail pages both render
fully in this normal build, including their individual workload charts.
Shared-runtime local CI passes on macOS and native ARM64 Linux (7/7 each);
the Mac product workflow passes 12/12. iPad/CLI all-feature Clippy, iOS ARM64
FFI cross-check and all 13 FFI tests pass.
Stable product installations and environments are unchanged.


## Desktop repair: alpha, Gallery and network selection (2026-11-15)

Android native-image buffers retain their producer's alpha contract. RGBA and
BGRA planes use premultiplied blending; RGBX planes stay opaque. Both the Mac
presenter and iPad frame ABI consume the shared capability metadata. No pixel
readback, extra guest copy or new scheduler/coordinator is introduced. The real
Metal shaders for both products pass transparent, half-transparent and opaque
pixel checks (`tests/fixtures/wayland-alpha-metal.m`).

Gallery's boot ANR was in the original camera-count query, not its drawing code.
The profile now defaults to Android's own `config.disable_cameraservice=true`
because no camera is forwarded; explicit image properties still win. The image
also ships 32-bit OMX/mediaserver entrypoints. A small ARM64 media adapter
registers its existing `libstagefright_omx` objects and compiles the unmodified
Android 13 resource-manager/observer sources missing from the 64-bit image.
Original Codec2 software codecs remain enabled. Without both services,
MediaProvider blocks in `MediaCodec.native_setup` during its HDR capability
probe and is killed for failing to publish its providers.

The adapter runs under the original Android init as user `media`. Init stops
the unusable original media/OMX launchers and disabled camera services; no host
service or polling loop substitutes for them. `media.player` is not fabricated:
the framework retains its normal local-codec-list fallback. Pinned source and
VNDK header inputs are recorded in `scripts/android-upstream.json`; upstream
source material goes in the separate source export, not the adapter payload.
The macOS build uses Google's pinned x86_64 host AIDL executable through
Rosetta at build time only; other hosts can pass an Android 13 compiler with
`--aidl`. The runtime and guest adapters remain ARM64.

The Mac and iPad Android entrypoints now select the existing guest-managed IP
uplink instead of a read-only native interface snapshot. netd can preserve the
IP link's intrinsic `IFF_NOARP` while changing its up/down state. Android still
performs its original Ethernet, IpClient, routing and resolver setup. Ordinary
Linux native networking is unchanged; the guest cannot configure a host NIC.

Native mouse events stay unmodified. A real Mac right click was observed as
Android InputReader `ButtonState=0x2`, followed by `0x0` on release. Android 13
maps this to Secondary; Back/Side buttons are separate and synthesize the
platform Back key. Trames does not remap right clicks to Back. Window resizing
already follows xdg configure → original Waydroid HWC hotplug → Android display
configuration: a live check changed the physical display from 3840×1856 to
2048×1536 with matching InputReader coordinate bounds.

Gallery is accepted through its real Timeline UI (empty library). This is not
an emulated-storage/FUSE, camera, general video-playback or Android CTS pass.
The lack of emulated external storage remains a separate limitation.


Final cold-boot acceptance: init reports `trames-media=running`, original OMX,
resource-manager and observer services are registered, and the unusable media,
OMX and camera launchers are stopped. Gallery cold-launch reports 1,212 ms.
ConnectivityService selects validated Ethernet network 100, with actual gateway
DNS, and the owner confirms the original Jelly browser displays Example Domain
over HTTPS. No private photos or browser session data are part of the evidence.
Shared-runtime local CI passes on both macOS and native ARM64 Linux (7/7 each),
Mac product CI passes 12/12, and the iPad/CLI workspace tests, all-feature Clippy
and ARM64 iOS FFI check pass. See the sanitized
[validation record](../validation/android-desktop-20261115.json).
