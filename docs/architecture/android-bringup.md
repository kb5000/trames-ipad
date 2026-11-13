# Android container bring-up: foundation implementation plan

Status: source audit and implementation plan, 2026-11-13. Android has not yet
been booted or tested in Trames. Runtime audit baseline:
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
| A1 | Binder IPC | No Android Binder implementation found. | Provide separate binder/hwbinder/vndbinder contexts, the 64-bit device UAPI, transaction mappings, object/FD transfers, nested synchronous calls, ordered one-way delivery, death notifications, poll/wait and original process/thread/FD cleanup. See the complete subsystem contract below. |
| A2 | Android shared memory | memfd, shared mappings, FD transfer and four original seals exist. `memfd.rs` rejects seal bits outside `0x0f`. | Implement real `F_SEAL_FUTURE_WRITE` semantics across file I/O, shared mapping admission and native views. Verify the selected libcutils memfd detection. Add ashmem-on-memfd ioctls or a legacy ashmem device only if the selected image actually requires them. |
| A3 | SELinux boot compatibility | No SELinux policy engine or selinuxfs; security xattrs currently report absence on reads and reject writes. | Prefer the upstream Waydroid no-SELinux route. Add only demonstrated permissive metadata/query dependencies, scoped to the Android profile. Enforcing policy is outside this boot milestone. |
| A4 | Boot metadata, logging and basic ABI | kmsg/syslog, proc task/memory/time views, random devices, PTYs, ELF/PT_INTERP, fork/exec, signals, timers, epoll, futexes and membarrier have implementations. | Reuse them. Probe Bionic/linker64, TLS, allocator reservations, tagged-address controls and ART-used synchronization. Supply missing `/proc/cmdline`, `/proc/bootconfig` and selected sysfs metadata from the guest boot profile, without leaking host configuration or advertising absent hardware. |
| A5 | Unix IPC and credentials | Unix streams/datagrams, peer credentials, SCM_CREDENTIALS and SCM_RIGHTS exist. | Test init property sockets and logd against their real credential/group queries. Audit SO_PEERGROUPS and any remaining security-context queries. Peer identity must be the guest's original credentials, never the host UID/groups. |
| A6 | Task profiles and memory-pressure interfaces | cgroup v2 membership, pids, freezer/kill and oom_score_adj exist. CPU/memory/cpuset controllers and PSI are not established; pidfd names exist without a corresponding implementation found in this audit. | Resolve the selected task_profiles/cgroups configuration, implement required pidfd open/signal/readiness/exit lifetime, and provide a real LMKD pressure path. Existing pids/freezer support does not mean all Android cgroup controllers exist. |
| A7 | Android sync fences | Vulkan fences/timelines and existing resource waits exist; Linux sync_file/sw_sync device ABI was not found. | Reuse completion resources for pollable sync-file FDs, fence merge/status and the sw_sync operations used by the selected HWC. Retain native-image ownership until its real producer/consumer fences complete. |
| A8 | Waydroid HWC, Wayland and input | Trames Wayland supports shared-memory buffers, windows, keyboard/pointer and native image presentation. Generic linux-dmabuf protocol handling was not found; wl_touch currently has no actual event delivery. | Reuse Waydroid HWC and its existing shared-memory route first. Test its required globals, subsurfaces, configure/frame/release ordering and input FIFO integration. Add actual touch delivery for iPad. GPU DMA-BUF/import support comes with A9. |
| A9 | Native Android graphics buffers | Native images and selected DMA-BUF-shaped image imports exist; the guest Vulkan library is built against musl. | Build the Android/Bionic guest driver bridge and adapt the smallest allocator/GBM backend needed to retain original native images. Implement the matching Wayland buffer import and fence contracts. Reuse HWC rather than building another compositor or a complete DRM GPU. |
| A10 | netd and networking | Shared TCP/UDP/DNS, routing/netlink and netfilter resources exist; full Android eBPF integration does not. | Reuse Waydroid's no-BPF configuration/patches where present, while returning honest unsupported results from absent kernel APIs. Verify IP configuration, app DNS and actual app traffic. Do not infer netd compatibility from Docker networking. |
| A11 | APEX, vold and external storage | Existing VFS/bind mounts and private persistence are reusable; APEX image activation and FUSE are not established. | Inspect how the selected container image activates its packaged modules. Run the existing apexd/vold path where compatible. FUSE and external-storage operations remain conditional boot/app dependencies, not assumed successes. Full AVB, OTA, disk encryption and phone block-device setup are outside this milestone. |

The shared-memory gap is concrete: current
[memfd seal admission](https://github.com/kb5000/trames-runtime/blob/c0829591f8652e55c3edb074f593f680efbb76b9/crates/trames-vfs/src/memfd.rs#L310)
rejects `F_SEAL_FUTURE_WRITE`. Current upstream
[libcutils probing](https://android.googlesource.com/platform/system/core/+/refs/heads/main/libcutils/ashmem-dev.cpp)
checks this seal and ashmem compatibility ioctls on memfds. The exact probe in
the chosen Android 13 binaries still needs confirmation; newer-source checks
must not be silently assigned to every older image.

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

## SELinux scope: boot without policy enforcement

The owner's initial requirement is boot compatibility, including permissive-only
support if needed. Waydroid already provides a smaller initial path:

- Its init patch skips policy loading and enters second stage directly.
- Its service-manager patch bypasses SELinux access checks.
- Its Binder patch omits transaction security-context requests.
- Its host/container layer supplies Binder nodes; its init patch removes the
  binderfs mount/device setup.

Use these existing upstream adaptations after checking the selected image's
provenance. Do not implement a policy parser or fabricate a selinuxfs merely to
turn an already supported no-SELinux path into a policy-loading path.

If real callers still require a permissive subset, specify their actual files,
xattrs or socket calls and implement metadata consistently on original guest
objects. A supplied `enforce` view remains zero; attempts to enable enforcement
are unsupported. Do not return success for loading/enforcing a policy that is
not implemented. Generic Linux environments retain their existing behavior.
This initial profile does not claim Android security-policy conformance.

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
at integration/release boundaries. This document-only change does not require
rebuilding CPU, graphics, Linux or existing installed applications.

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
