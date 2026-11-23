# Linux ARM64 syscall implementation roadmap

Source snapshot, 2026-11-22. The shared runtime at `d645a8f` has usable paths
for 261 of the 318 names in its Linux v6.12 AArch64 inventory, including partial
implementations. Two more entries validate requests without completing the
operation. The remaining 55 have no implementation path. This is a source audit.
Per-operation ABI compatibility and product acceptance have separate evidence.
The audit uses the runtime's `trames-vfs/src/syscall_names.rs` and includes
specialized session dispatch outside the main syscall tables.

## gVisor compatibility queue, 2026-11-23

The latest rolling inventory has **126 gaps among 13,864 cases that passed on
native ARM64 Linux**: 13,738 have a passing Trames result (99.09%). The gaps
comprise ten failures, one timeout, 77 conditional skips and 38 missing result
records. These are carried-forward per-case results from multiple revisions,
not a full rerun on the latest commit. The [case ledger](gvisor-gaps-20261123.json)
records the exact selection and status. The 593 native skips and 73 native
failures/timeouts/missing results are outside this strict comparison; their
Trames outcomes are not additional confirmed compatibility failures.

| Remaining group | Cases | Combined implementation and acceptance work |
| --- | ---: | --- |
| Landlock v1–v6 | 38 | Join the application-sandboxing batch below. Implement enforced rules and inheritance; feature-detection skips remain open until the assertions run and pass. |
| iptables / ip6tables result coverage | 38 | Reconcile or rerun the 23 IPv4 and 15 IPv6 missing records against the unchanged original binaries. Do not count these as confirmed implementation failures or subtract them based on revision-query coverage alone. Repair any demonstrated socket-option or filtering gaps in the network batch. |
| cgroup v2 and cgroup BPF | 17 | Join container resource management and Android memory lifecycle: CPU/cpuset controllers, memory charging/limits, OOM, controller ownership and BPF queries. These extend beyond the missing syscall-entry list. |
| PIDFD_THREAD | 6 | Extend the process-lifecycle batch with individual-thread identity, exit notification, tracing and exec revival. |
| Remaining VFS, network, VM and identity behavior | 27 | Fix object semantics in batches: ACL/overlay, link/unlink, proc sysctls, mount-FD overlay use, netlink, FUSE and mapping behavior. Pair with modern mounts, mapping management and affected existing syscalls. |
| Total | 126 | Keep missing results, conditional skips, failures and timeouts distinct. |

Use one work queue for the syscall roadmap and these original cases. A new
syscall can close multiple cases; a failing case can instead expose a boundary
condition in an already implemented syscall. Completing the syscall name list
therefore does not establish gVisor compatibility, and clearing this fixed
selection does not establish the newer interfaces absent from the selection.

First reconcile the 38 missing network records and close small existing-ABI
gaps. Then deliver Landlock, cgroup/PIDFD lifecycle and modern mount support as
whole dependency batches, keeping the Android image-specific memory and
backend-dependent synchronization work tied to their actual workloads. Each
batch updates this ledger, verifies both executors and ordinary workload
performance, and pushes the validated main commits with paired runtime pins.
Reserve a broad same-revision rerun for the integration milestone; reuse
unchanged evidence while implementing individual batches.

## Delivery order

| Order | Batch | Completion evidence |
| --- | --- | --- |
| 1 | Guest tracing and debugging | GDB can launch and attach, stop at a breakpoint, inspect state, step and detach. `strace` can follow a child and report syscall results. |
| 2 | Modern mount administration | A supported OCI container can boot with recursive read-only and ID-mapped bind mounts. Mount queries describe the same visible tree as `/proc/self/mountinfo`. |
| 3 | Android memory lifecycle | Verify which paths the shipped ART and `lmkd` use. Implement and measure the paths the image can exercise under memory pressure. |
| 4 | Thread synchronization | The futex2 family shares existing wait queues and preserves legacy futex performance. Enable `rseq` after both CPU backends support its restart semantics and a target workload benefits. |
| 5 | Guest performance tooling | Stock Linux `perf stat` and `perf record` work with a documented set of guest software events. |

Each batch includes the related resource operations, caller fallbacks and
regressions needed to deliver the capability. The guest ABI stays in the shared
runtime. Use the existing worker and resource owners, object-local locks or
atomics, and existing wake targets. A blocking operation returns its execution
slot. Avoid coordinator threads, polling loops and extra work on unaffected
syscall or instruction paths. Keep the guest namespace and permission model
independent of host credentials and resources. [Execution backends](../architecture/execution-backends.md)

## 1. Guest tracing and debugging

Status on 2026-11-23: the ptrace task relationship, ARM64 state mutation,
software breakpoints, single stepping and proc-memory work are implemented.
The [ptrace validation](../../runtime/docs/validation/guest-ptrace-mutation-20261123.md)
closes the twelve remaining process-debugging comparison gaps. The
[proc-memory follow-up](../../runtime/docs/validation/guest-proc-mem-20261123.md)
passes all selected proc-memory cases. The sole upstream ptrace skip is its
Native-platform Yama guard, covered separately in the disposable guest.
Real GDB launch, software breakpoint, step, register mutation and continued
exit pass on both executors. The broader stock-tool acceptance checklist below
still needs explicit evidence for live attach, gdbserver, multithreaded sessions
and strace fork/exec following; do not mark it complete based only on those
GDB smoke runs. The six PIDFD_THREAD cases remain in the queue above.

- Complete `ptrace` as a guest task relationship. Share target lookup and
  credential checks with the existing admission code. Implement `TRACEME`,
  `ATTACH`, `SEIZE`, `INTERRUPT` and `DETACH` with the corresponding stopped
  states, wait reporting, signal delivery and cleanup when either task exits.
- Add the ARM64 register set and guest memory access needed by GDB, together
  with breakpoint traps, single step and continue on both execution backends.
  Cover `SETOPTIONS`, event messages, syscall stops and the clone, exec and exit
  events used by GDB and `strace -f`. Keep debugger stops distinct from ordinary
  job-control stops. Audit `/proc/PID/mem`, `waitpid` with `__WALL` and any
  cross-address-space memory access the actual debugger selects, including
  `process_vm_readv` and `process_vm_writev`.
- Run the original gVisor tracing-dependent wait and exec cases against native
  ARM64 Linux. With stock GDB and `gdbserver`, launch a small multithreaded
  program, attach to a running instance, hit a breakpoint, read and edit
  registers and memory, step and detach. Use stock `strace -f` to check syscall
  stops across fork and exec. Cover denied access and debugger exit while the
  target is stopped. [GDB Linux backend](https://github.com/RTEMS/sourceware-mirror-binutils-gdb/blob/master/gdb/linux-nat.c)

## 2. Modern mount administration

| Caller and operation | Syscalls and acceptance |
| --- | --- |
| util-linux 2.41.5 `mount -o rbind,ro=recursive,noexec=recursive` | **Verified.** The original tool calls `open_tree`, `mount_setattr(AT_EMPTY_PATH\|AT_RECURSIVE)` and `move_mount`. Both target levels reject writes and report the new VFS flags. The source stays writable. [Validation](../../runtime/docs/validation/guest-mount-setattr-20261123.md) |
| runc recursive mount options | The runc 1.3.0 source uses `mount_setattr(AT_RECURSIVE)` through an opened target's proc FD. The shared fixture exercises that path. Acceptance for the full OCI startup with these options is pending. [runc source](https://github.com/opencontainers/runc/blob/v1.3.0/libcontainer/rootfs_linux.go#L1231) |
| OCI ID-mapped bind mounts | `mount_setattr(MOUNT_ATTR_IDMAP)` needs mount-local UID/GID translation through permission checks, ownership queries and creation. Verify mapped and unmapped IDs, namespace authority and private/shared storage. [OCI mount configuration](https://github.com/opencontainers/runtime-spec/blob/main/config.md) |
| util-linux filesystem reconfiguration | `fspick` selects the original superblock for `fsconfig(FSCONFIG_CMD_RECONFIGURE)`. Verify that changes reach bind aliases and that an individual mount's attributes remain independent. [libmount source](https://github.com/util-linux/util-linux/blob/v2.41.5/libmount/src/hook_mount.c) |
| util-linux 2.41.5 `findmnt --kernel=listmount` | **Verified.** `listmount` provides forward and reverse pagination and `statmount` describes each mount. `statx(STATX_MNT_ID_UNIQUE)` returns the same unique ID. The original tool agrees with `mountinfo` on the tested mount tree and selects mounts by unique ID. [Validation](../../runtime/docs/validation/guest-mount-query-20261123.md) |

`mount_setattr` supports read-only, nosuid, nodev, noexec and atime flags on
attached and detached trees. ID mapping, propagation changes through this
syscall and nosymfollow remain pending. The original mount and mount-FD
suites are carried into validation for this batch. The two overlay mount-FD
skips remain in the gVisor queue.

## 3. Android memory lifecycle

- Identify the shipped ART collector and `lmkd` reaping path from guest logs
  and controlled traces. Measure GC pause times and the time for memory use to
  drop after killing an app. Use the same image and workload for before and
  after comparisons.
- Implement `process_mrelease` through the guest pidfd, exit lifecycle and
  address-space owner. Reclaim eligible pages while preserving shared mappings
  and valid references held by other tasks. Check shared address spaces,
  concurrent exit, repeated calls and documented failure results. Use an
  isolated Android pressure test to confirm the reaper actually selects it.
  [AOSP reaper](https://android.googlesource.com/platform/system/memory/lmkd/%2B/refs/tags/android-13.0.0_r65/reaper.cpp)
- Implement the `userfaultfd` ABI required by the selected ART path. Start with
  the guest user-mode fault model and the exact registration, event and
  resolution operations used by that image. Verify `mremap` behavior if the
  collector depends on it. Connect missing-page delivery to each CPU backend's
  existing fault and wake paths. Cover multiple waiters, unmap, remap, close
  and exit, including termination while a fault is pending. Advertise only
  verified feature bits. Check that the image runs the intended collector and
  compare application pause tails on a physical iPad as well as on Mac.
  [AOSP ART collector](https://android.googlesource.com/platform/art/%2B/b4172cda352aee6cd207a354ab9a568988037e09/runtime/gc/collector/mark_compact.cc)

## 4. Thread synchronization

- Implement `futex_waitv`, `futex_wait`, `futex_wake` and `futex_requeue` on
  the existing private and shared futex keys. A vector wait publishes one
  logical waiter, wakes once with the winning index and removes all of its
  registrations on completion. Verify mismatched values, wake races, requeue,
  timeout, signal interruption and shared aliases. Run the Linux futex
  selftests supported by the guest environment and existing Trames shared
  futex tests. Compare uncontended legacy futex costs and multicore throughput.
  [Linux futex2 ABI](https://docs.kernel.org/userspace-api/futex2.html)
- Treat `rseq` as an execution and lifecycle change. Validate thread
  registration, CPU identity, preemption, migration and signal abort semantics
  on the Mac hardware executor and iPad interpreter. Restore the original
  gVisor no-libc fixture before treating that suite as coverage. Run a stock
  glibc workload with the same CPU setup and measure the selected fast path.
  [Linux rseq ABI](https://docs.kernel.org/userspace-api/rseq.html)
  Debian's util-linux mount invokes `rseq` during glibc startup and completes
  the tested workload through the existing fallback. Record this use as a
  performance dependency when selecting the first workload for `rseq`.

## 5. Guest performance tooling

Implement `perf_event_open` for guest task software events such as task CPU
time and scheduler context switches. Expose the event FD, read and control
operations and the sample ring required by supported `perf record` modes.
Use existing accounting, enable collection only while events are active and
apply guest task visibility rules. Validate event counts and sample records
with stock tools. Report hardware PMU events as unsupported until the backend
can attribute them to the guest and measurements confirm the result.
[Linux perf ring buffer ABI](https://docs.kernel.org/userspace-api/perf_ring_buffer.html)

## Follow-on batches

| Capability | Calls | Implementation and acceptance condition |
| --- | --- | --- |
| Filesystem identity and events | `fanotify_init`, `fanotify_mark`, `name_to_handle_at`, `open_by_handle_at` | Build on guest inode and mount identity. Add the fanotify event and file-handle modes used by the target tool. Verify mount namespace visibility, permission checks, rename and stale handles, including supported shared-directory semantics. |
| File and pipe I/O | `cachestat`, `vmsplice`, `io_cancel` | Use real guest cache residency for `cachestat`. Reuse pipe backpressure and account for page lifetime with `vmsplice`. AIO cancellation must coordinate with the accepted request and publish one completion across cancel, I/O completion and teardown. |
| Process and queue utilities | `kcmp`, `mq_notify`, `vhangup` | Compare retained guest resources for `kcmp`, deliver one-shot queue notification for `mq_notify`, and use guest PTY lifetime and job control for `vhangup`. Test with the actual diagnostic, libc or login caller. |
| Application sandboxing and keys | `landlock_create_ruleset`, `landlock_add_rule`, `landlock_restrict_self`, `add_key`, `request_key`, `lsm_get_self_attr`, `lsm_set_self_attr`, `lsm_list_modules` | Enforce advertised Landlock access rules across guest lookups, exec and inheritance. Add creation and lookup to existing guest keyrings. Report attributes for security modules that the runtime actually provides. Verify positive and negative access cases. [Linux Landlock ABI](https://docs.kernel.org/userspace-api/landlock.html) |
| Mapping management and seals | `process_madvise`, `mseal` | Reuse pidfd permission checks and existing VM advice and mapping mutation paths. For `mseal`, verify every supported map mutation respects the seal, plus range, inheritance and failure behavior. Prioritize when a target workload exercises either interface. |
| Quotas and time administration | `quotactl`, `quotactl_fd`, `clock_settime`, `settimeofday`, `adjtimex`, `clock_adjtime` | Tie quota operations to a guest filesystem with enforceable quotas. Implement time setting or discipline only against a defined guest clock model, with consistent realtime timer behavior. |
| Backend-dependent memory features | `remap_file_pages`, `migrate_pages`, `move_pages`, `pkey_mprotect`, `pkey_alloc`, `pkey_free`, `memfd_secret`, `map_shadow_stack` | Defer until a guest workload needs the interface and the backend can provide its observable semantics. Page placement depends on available guest NUMA nodes. Protection keys, secret memory and shadow stacks require their corresponding access and execution guarantees. |
| Kernel administration | `kexec_load`, `kexec_file_load`, `init_module`, `finit_module`, `delete_module`, `swapon`, `swapoff`, `acct` | Defer guest kernel loading, kernel modules and swap in the userspace-kernel design. Process accounting requires a separate guest policy and a real consumer. |
| Legacy numbers | `lookup_dcookie`, `nfsservctl` | Preserve `ENOSYS`, matching Linux v6.12. [Linux v6.12 syscall table](https://github.com/torvalds/linux/blob/v6.12/scripts/syscall.tbl) |

## Validation before integration

1. Run affected shared-runtime tests and original gVisor cases on the same
   candidate revision. Use native ARM64 Linux for expected ABI behavior and
   record pass, skip, failure, timeout and fixture errors separately. Add
   targeted Linux selftests or stock application tests where gVisor lacks a
   relevant assertion. [Linux kselftest guide](https://docs.kernel.org/dev-tools/kselftest.html)
2. Run the Mac hardware path and the iPad interpreter for shared ABI changes.
   Check the iOS build and use a physical iPad for platform-dependent
   lifecycle, pause and memory-pressure acceptance. Keep the runtime revision
   paired with each product in the test record.
3. Compare fixed-work old and new runs on the affected main paths. Record
   relevant median and tail latency, CPU time or memory, with repeated samples
   on the same configuration. Investigate reproducible regressions before
   integration. Use the existing local CI workflow and reuse evidence for
   unchanged code. Keep the full gVisor rerun and end-to-end release checks
   for the integration or release candidate.

Record each delivered syscall group with its supported subset, Linux reference,
workload results, unresolved behavior and exact runtime revision. A feature
with an unavailable prerequisite retains its Linux-compatible error until its
whole path is ready.
