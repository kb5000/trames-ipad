# Linux ARM64 syscall implementation roadmap

Source snapshot, 2026-11-22. The shared runtime at `d645a8f` has usable paths
for 261 of the 318 names in its Linux v6.12 AArch64 inventory, including partial
implementations. Two more entries validate requests without completing the
operation. The remaining 55 have no implementation path. This is a source audit.
Per-operation ABI compatibility and product acceptance have separate evidence.
The audit uses the runtime's `trames-vfs/src/syscall_names.rs` and includes
specialized session dispatch outside the main syscall tables.

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

- Complete `ptrace` as a guest task relationship. Share target lookup and
  credential checks with the existing admission code. Implement `TRACEME`,
  `ATTACH`, `SEIZE`, `INTERRUPT` and `DETACH` with the corresponding stopped
  states, wait reporting, signal delivery and cleanup when either task exits.
- Add the ARM64 register set and guest memory access needed by GDB, together
  with breakpoint traps, single step and continue on both execution backends.
  Cover `SETOPTIONS`, event messages, syscall stops and the clone, exec and exit
  events used by GDB and `strace -f`. Keep debugger stops distinct from ordinary
  job-control stops. Audit `/proc/PID/mem`, `waitpid` with `__WALL` and any
  cross-address-space memory access the actual debugger selects. Existing
  `process_vm_readv` and `process_vm_writev` support only selected local cases.
- Run the original gVisor tracing-dependent wait and exec cases against native
  ARM64 Linux. With stock GDB and `gdbserver`, launch a small multithreaded
  program, attach to a running instance, hit a breakpoint, read and edit
  registers and memory, step and detach. Use stock `strace -f` to check syscall
  stops across fork and exec. Cover denied access and debugger exit while the
  target is stopped. [GDB Linux backend](https://github.com/RTEMS/sourceware-mirror-binutils-gdb/blob/master/gdb/linux-nat.c)

## 2. Modern mount administration

- Implement `mount_setattr`, `fspick`, `statmount` and `listmount` around the
  existing VFS mount objects and mount namespaces. Reuse the descriptor mount
  path for selection, reconfiguration, detached trees and attachment.
  `statmount` and `listmount` must agree with `statx` and procfs where the
  interfaces overlap. Preserve the distinction between unique mount IDs and
  legacy IDs used by procfs.
- Cover recursive read-only and related per-mount flags, propagation and
  `MOUNT_ATTR_IDMAP` on the supported guest filesystems. Apply the ID mapping
  consistently to ownership queries, permission checks and file creation.
  Validate the whole requested update before publishing it. Reject unsupported
  filesystem and flag combinations with the Linux ABI error.
- Run original mount and namespace cases, then test a Debian systemd environment
  with stock OCI tools. Check recursive bind trees, mapped and unmapped IDs,
  denied mutations, namespace teardown and existing mount fallback. Exercise
  isolated private storage and existing shared directory behavior.
  [OCI mount configuration](https://github.com/opencontainers/runtime-spec/blob/main/config.md)

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
