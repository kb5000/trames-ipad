# Android container bring-up: foundation and roadmap

Status: shared Binder foundation implemented, 2026-11-13. Unchanged Waydroid
service-manager/client binaries pass on the interpreter and Mac HVF with 1, 4
and 12 CPU slots. A full Android system has not yet booted. Implementation:
[`fe2f98e`](https://github.com/kb5000/trames-runtime/commit/fe2f98ea447cf23a13083fd962c34a7b65bd1075).
Original runtime audit baseline:
[`c082959`](https://github.com/kb5000/trames-runtime/commit/c0829591f8652e55c3edb074f593f680efbb76b9).

The first target is a complete ARM64 Android desktop and APK execution using an
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
| A0 | Image import and Android boot profile | Rootfs import, PID 1, mount namespaces, tmpfs/proc/sysfs/devpts and systemd boot preparation exist; Android policy is missing. | Install a matching image pair, prepare private device/mount trees and boot properties, retain `/data` across restart, and run the existing init with observable logs. |
| A1 | Binder IPC | Shared 64-bit Binder foundation implemented; original Waydroid service-manager/client smoke test passes. | Provide separate binder/hwbinder/vndbinder contexts, the 64-bit device UAPI, transaction mappings, object/FD transfers, nested synchronous calls, ordered one-way delivery, death notifications, poll/wait and original process/thread/FD cleanup. See the complete subsystem contract below. |
| A2 | Android shared memory | memfd, FD transfer and `F_SEAL_FUTURE_WRITE` are implemented, including existing VMA write rights. | Validate new write/mapping denial and retained old shared mappings through the canonical memory model. Verify the selected libcutils memfd detection. Add ashmem-on-memfd ioctls or a legacy ashmem device only if the selected image actually requires them. |
| A3 | SELinux boot compatibility | Disabled: no SELinux LSM or selinuxfs; security xattrs report absence on reads and reject writes. `/proc/cmdline` includes `selinux=0`. | Use the existing Waydroid no-SELinux adaptations. Do not expose an `enforce=0` pseudo-filesystem or claim permissive/enforcing policy support. |
| A4 | Boot metadata, logging and basic ABI | kmsg/syslog, proc task/memory/time views, random devices, PTYs, ELF/PT_INTERP, fork/exec, signals, timers, epoll, futexes and membarrier have implementations. | Reuse them. Probe Bionic/linker64, TLS, allocator reservations, tagged-address controls and ART-used synchronization. The basic `/proc/cmdline` and empty `/proc/bootconfig` now exist; supply Android-specific boot arguments and selected sysfs metadata from the guest boot profile, without leaking host configuration or advertising absent hardware. |
| A5 | Unix IPC and credentials | Unix streams/datagrams, peer credentials, SCM_CREDENTIALS and SCM_RIGHTS exist. | Test init property sockets and logd against their real credential/group queries. Audit SO_PEERGROUPS and any remaining security-context queries. Peer identity must be the guest's original credentials, never the host UID/groups. |
| A6 | Task profiles and memory-pressure interfaces | cgroup v2 membership, pids, freezer/kill and oom_score_adj exist. CPU/memory/cpuset controllers and PSI are not established; pidfd names exist without a corresponding implementation found in this audit. | Resolve the selected task_profiles/cgroups configuration, implement required pidfd open/signal/readiness/exit lifetime, and provide a real LMKD pressure path. Existing pids/freezer support does not mean all Android cgroup controllers exist. |
| A7 | Android sync fences | Vulkan fences/timelines and existing resource waits exist; Linux sync_file/sw_sync device ABI was not found. | Reuse completion resources for pollable sync-file FDs, fence merge/status and the sw_sync operations used by the selected HWC. Retain native-image ownership until its real producer/consumer fences complete. |
| A8 | Waydroid HWC, Wayland and input | Trames Wayland supports shared-memory buffers, windows, keyboard/pointer and native image presentation. Generic linux-dmabuf protocol handling was not found; wl_touch currently has no actual event delivery. | Reuse Waydroid HWC and its existing shared-memory route first. Test its required globals, subsurfaces, configure/frame/release ordering and input FIFO integration. Add actual touch delivery for iPad. GPU DMA-BUF/import support comes with A9. |
| A9 | Native Android graphics buffers | Native images and selected DMA-BUF-shaped image imports exist; the guest Vulkan library is built against musl. | Build the Android/Bionic guest driver bridge and adapt the smallest allocator/GBM backend needed to retain original native images. Implement the matching Wayland buffer import and fence contracts. Reuse HWC rather than building another compositor or a complete DRM GPU. |
| A10 | netd and networking | Shared TCP/UDP/DNS, routing/netlink and netfilter resources exist; full Android eBPF integration does not. | Reuse Waydroid's no-BPF configuration/patches where present, while returning honest unsupported results from absent kernel APIs. Verify IP configuration, app DNS and actual app traffic. Do not infer netd compatibility from Docker networking. |
| A11 | APEX, vold and external storage | Existing VFS/bind mounts and private persistence are reusable; APEX image activation and FUSE are not established. | Inspect how the selected container image activates its packaged modules. Run the existing apexd/vold path where compatible. FUSE and external-storage operations remain conditional boot/app dependencies, not assumed successes. Full AVB, OTA, disk encryption and phone block-device setup are outside this milestone. |

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
  not advertised or faked. Binderfs control, ashmem compatibility and full Android
  boot are separate remaining milestones.

The first inspected binary image is the official ARM64 VANILLA build
`lineage-20.0-20260927-VANILLA-waydroid_arm64-system.zip` (912,267,974 bytes), SHA-256
`5fb8179ed4455f45919c5a2bfbfdd1591619295994ad321217f03bb1dcfc53a7`.
Its system fingerprint is
`waydroid/lineage_waydroid_arm64/waydroid_arm64:13/TQ3A.230901.001/aleasto09270452:userdebug/test-keys`.
Only the required original executables/libraries were imported for the smoke
test. No complete AOSP checkout or Android source rebuild was used. This does
not validate a vendor image or the complete system/vendor boot pair.

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

### Validation at integration

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
  memory model. Merely exposing a permanently zero PSI file is not monitoring;
  disabling PSI is insufficient unless the image has a working alternative.
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

The recommended Android GPU path keeps the current Vulkan command backend.
It adds Android loader/driver entry points and the external-buffer/fence
contracts used by Android's producer, compositor and HWC processes. This is a
design decision, not completed Android GPU support.

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

Do not check out or build full AOSP to add the initial GPU bridge. Reuse the
selected image's SurfaceFlinger, HWC, interface libraries and vendor ANGLE first.
Build only the Android/Bionic Trames Vulkan driver module and the small native-
image allocator/mapper backend that connects to the existing host resources.
These outputs use Android's driver/HAL contract, not the Linux ICD JSON package.

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

Waydroid sources were inspected on the `lineage-20` branch; actual image
selection and binary acceptance remain pending.

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
