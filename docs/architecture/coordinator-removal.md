# Coordinator dependency audit

## Current status — 2026-11-09

Stage 80 moves supported **guest halt/poweroff** onto original lifecycle
resources. Source/solo share authority, magic, command and namespace validation.
The initial root publication retains its original group control across later
admission/rekey/retirement. A source power request exits that original root and
the caller's group using existing execution leases, waits and cleanup. No PID
lookup, host power API, new service thread, ordinary memory check/fence or
per-instruction check is added.

This removes the power syscall handoff; final session/backend teardown still
stops and drains actual CPU actors. Missing real shutdown/exit capabilities,
other final dispatch and host/legacy boundaries remain. The overall goal is
active. See the [validation record](../validation/guest-power-20261109.json).
Only Mac Dev deployment is included; stable 1.5.4, business environments and
physical iPad installs retain their versions.

### Stage 79 original system statistics (historical)

Stage 79 moves the supported **system statistics subsystem** onto original
shared resources. `sysinfo` and proc observations use the same original task
directory, unique-mm residency reports and host-memory callback. Both source and
legacy calls share the AArch64 ABI and destination preflight. Active task/CPU time
is visible before an assignment returns; load and idle no longer wait for the
owner drive loop to copy a snapshot. No statistics thread, message bus, extra
ordinary guest-memory check/fence or per-instruction/native-fast-call clock is
introduced.

Load remains the existing five-second exponential approximation, sampled by
actual aggregate observations. CPU time retains wall-assignment accounting.
The missing-provider, final dispatch, actual cancel/deadline/shutdown and
initial/legacy/solo boundaries remain. The overall coordinator-removal goal
stays active. See the [validation record](../validation/system-statistics-20261109.json).
Only Mac Dev deployment is included; stable 1.5.4, business environments and
physical iPad installs keep their versions.

### Stage 78 live host pause/resume (historical)

Stage 78 removes the **live host pause/resume join**. An explicit pause keeps
original jobs, CPU state, execution Views, translations and accepted waits in the
same cohort. Existing execution leases stop active CPUs and close the pre-entry
race. The queue suspends guest dispatch while existing workers can still finish
accepted backing/lifecycle work. Resume reuses those jobs and starts only needed
cold affinity slots. No new service thread, guest memory check, fence or
per-instruction pause check is introduced.

Host pause is separate from guest STOP/CONT, cgroup freezing and actual mapping
quiescence. Cancel, host deadlines/shutdown, final teardown, missing/excluded
resources and generic/legacy dispatch still retain their recorded boundaries.
The overall coordinator-removal goal remains active. See the
[validation record](../validation/host-pause-20261109.json).
Only Mac Dev is updated; stable 1.5.4, business environments and physical iPad
installs retain their existing versions.

### Stage 77 native-device closure (historical)

Stage 77 moves native special-device pathname/OFD operations and
`/dev/trames-bridge` bytes/readiness onto original source resources. Device
opens, aliases and metadata use the original shared pseudo-node cache. Accepted
Bridge I/O retains its original connection across close/FD reuse; passive
snapshots and subscriptions cannot delay last close. The existing broker mutex
still protects its protocol. Notifications run after that mutex is released, and
queue readiness observation is pure. No new coordinator/resource thread, polling
bus, ordinary guest-memory check or fence is added.

This closes the supported native-device/Bridge resource group. io_uring's two
actual maintenance-failure setters remain: absent wait capability and an original
body that cannot run on the source. The four remaining broad boundaries below
have not yet reached zero. See the [validation record](../validation/native-devices-20261109.json).
Only Mac Dev is updated; stable 1.5.4, business environments and physical iPad
installs keep their existing versions.

### Stage 76 host filesystem closure (historical)

Stage 76 moves **host filesystem commands** off joined execution: streaming
imports/exports, conditional FileProvider saves and timezone updates use the
existing disk pool and original shared resources. Accepted mapped byte consumers
use original backing admission, keeping unrelated execution in the same cohort.
Imports stage unpublished objects. Conditional compare/replace retains conflict,
metadata, hardlink, old-OFD, retry and failure cleanup behavior. Explicit pause
still stops guest execution; it now lets the original driver complete accepted
I/O and backing work. No new service thread or ordinary guest-memory check is
added. A cold editor hash can still block its original filesystem observers, and
actual mapping dependants still quiesce.

Both local runtime workflows, 407 process tests on each host, 9 FFI tests on each
host, 52 signed HVF plus 2 product hardware tests, the 8 applicable Mac product
checks and both iPad SDK compiles pass. The mapped export fixture completes in one
cohort on 4/12 CPUs, including explicit pause. These are conformance results, not
a browser or throughput speedup claim. See the [validation record](../validation/host-files-20261109.json).
Only Mac Dev is updated; stable 1.5.4, business environments and physical iPad
installs retain their existing versions.

### Stage 75 memory-pressure closure (historical)

Stage 75 closes **Mac host memory-pressure reclamation**. The existing host
adapter advances one original-VM request while vCPUs continue. The pressure-only
cohort join and stopped-only reclamation body are deleted. There is no new thread,
resource service, polling bus or ordinary guest-memory check.

The pressure request scans at most 64 original mapping records per step and
processes an 8 MiB batch rounded to physical extents. It consumes an amount once;
refaults do not create a moving RAM ceiling. Original host byte leases keep their
gate and release readiness, with no repeated scan while waiting. Stable host VA,
dirty data, shared-file aliases and native address ownership survive eviction.
The immediate API skips borrowed resources without leaving subscriptions.

| Signed real userspace-Linux pressure acceptance | 4 CPUs | 12 CPUs |
| --- | ---: | ---: |
| Peak parallel execution CPUs | 4 | 12 |
| Cohorts through reclaim and refault | 1 | 1 |
| Returned SVCs / owner service requests | 0 / 0 | 0 / 0 |

All 52 signed HVF tests, both signed product pressure tests and applicable local
Mac product checks pass. Coverage includes concurrent dirty-byte execution,
native growth/retirement, bounded scans/bytes, borrowed writes, cancellation and
original address ownership. These are backend/concurrency checks, not a host RSS
or browser/Geekbench performance claim.

The shared Linux/iPad Rust source is unchanged. Matching full runtime workflows
and both iPad compile results from stage 74 are reused. The [validation record](../validation/memory-pressure-20261109.json)
records source/executable hashes, original joined behavior, reuse proof and final
installed acceptance. Only Mac Dev is updated; stable 1.5.4, business environments
and physical iPad installs remain unchanged.

### Remaining boundaries after stage 80

**The coordinator still exists.** io_uring's remaining `owner_required` setters
are the two actual source-maintenance failures: unavailable wait capability or an
unpumpable original body. Missing layout/signal capability is no longer a trigger.
Missing typed providers, excluded resources/pins and actual host administration
remain explicit. Earlier stages retain their completed removals; this audit does
not rename those remaining cases or claim the whole goal is finished.

The remaining triggers group into four boundaries, rather than one category per
row in the coverage inventory below:

1. Actual host cancellation/deadline/shutdown and final teardown events.
2. Missing embedding capabilities or excluded resources, including io_uring's two
   actual source-maintenance failures.
3. Last-resort known-resource/subcommand and unhandled HLT/fault dispatch.
4. Initial/legacy admission and solo orchestration; solo is not itself a syscall RPC.

Supported guest halt/poweroff syscall execution is removed by stage 80. Supported sysinfo/system-accounting refresh is removed by stage 79. Live host pause/resume is removed from joined execution by stage 78. Native special-device/Bridge pathname, byte and readiness dependencies are removed by stage 77. Host transfer/timezone/FileProvider commands are removed from joined execution by stage 76. Mac memory-pressure reclamation is removed by stage 75.
Init notification/graceful-stop supervision was removed by stage 74. Diagnostic syscall tracing was removed by stage 73. Stage 72 removed
the independent embedding terminal-ID-0 execution category. Normal subsystem names
below describe coverage and do not imply another coordinator dependency.

| Area | Current execution | Remaining boundary |
| --- | --- | --- |
| Admitted file/path/FD, io_uring, Unix/TCP I/O, SCM_RIGHTS and cross-resource transfer | Calling worker and original shared objects | Excluded paths/descriptors/controls retain explicit escalation; supported layout changes now use original backing admission |
| Futexes, clocks/timers, admitted event/child/signal waits and caught signals | Source execution; blocking returns the CPU slot | Legacy nonmember wake/adoption, unsupported wait resources and capability-less embeddings remain |
| Guest terminals, sessions and job control | Calling worker and original process/TTY/Files/namespace resources; source and solo share the ABI | Embedding terminal ID 0 shares original PTY execution; capability-less embeddings and unsupported tty controls/io_uring remain explicit |
| Scheduling attributes and CPU affinity | Original task atomics, original PID/group/UID authority and task-scoped completion on the caller | Capability-less embeddings retain their boundary; unsupported features keep their errno. Required original CPU slots start directly from the live queue |
| Exit/group/default-fatal termination, STOP/CONT and vfork release | Source resources and original group/family notifications | Returned CPU/statistics and cold cleanup complete on existing workers; the actual shared mm may still need its byte checkpoint. Outside-cohort/solo and capability-less embeddings remain separate |
| fork/clone/vfork and execve/execveat | Existing workers handle pure and mixed mapping scopes, nonleader rekey and original-mm retirement; existing disk pool prepares native images without retaining a CPU slot | Affected actors still pause; actual capability-less embeddings and solo invocation remain distinct |
| Mapping topology, pins and mapped-file layout | Supported brk/mmap/munmap/mremap/mprotect/msync/madvise and buffer-registration checkpoints publish on an existing worker; additive anonymous mmap is direct | Proc/cold-mm checkpoints and supported mapped-file layout transactions now run on existing workers; excluded backing/lifetime/policy and capability-less embeddings retain explicit boundaries |
| Native special-device paths and Bridge bytes/readiness | Calling worker, original pseudo nodes/OFDs/connections and original readiness | No supported-device/Bridge-only join remains; unavailable providers and unsupported driver APIs retain their existing route/result |
| Native libc/GPU/video/audio | Normal bodies, resource waits and admitted mapping/entry checkpoints use calling/shared-resource execution; exceptional access reuses original task/mm custody | Capability-less embeddings and actual platform/host lifecycle remain; AppKit affinity, device/disk pools and local fallback are not extra coordinators |
| Guest halt/poweroff | Calling executor and original root/caller group lifecycle, with shared source/solo validation | No supported power-control-only join. Actual CPU drain/backend teardown and missing original control/exit capabilities retain their scope; restart/kexec/nested behavior stays unsupported |
| System statistics and live CPU accounting | sysinfo and proc use original task/mm/host resources; active assignments expose time directly | No supported-statistics-only join. Missing real statistics/host capabilities retain legacy execution; observer-sampled load and wall-assignment time remain approximations |
| Dynamic proc/sys and proc pathname/FD consumers | Calling worker and original PID/mm/Files/fs/namespace resources | First cross-mm byte export and invalidation activate on an existing worker at the original task/mm boundary; legacy providers retain explicit fallback |
| Mount topology administration | Original namespace/fs resources on the caller; existing disk pool for host-backed paths; contention releases the CPU slot | Genuinely excluded pathname controls retain their explicit boundary |
| Task namespace membership and Files/fs detachment | Supported unshare/setns/close_range run on the caller, refresh future consumers and preserve accepted async operations | Capability-less embeddings remain; clone creation still uses creator-mm custody; unsupported user/time namespaces are not added |
| Credentials and capability administration | Calling worker, shared ABI and original task credentials; future consumers refresh, accepted async work retains submission authority | Capability-less embeddings retain their boundary; unsupported user namespaces and file capabilities are unchanged |
| Memory barriers | Supported membarrier controls run on source workers, using original mm registration and active execution leases; pending callers return their CPU slot | Only requested fences affect relevant execution actors. Capability-less/solo callers keep their existing route; unsupported barrier commands remain unsupported |
| Entropy and random devices | getrandom and supported random/urandom device/UUID operations run on the caller; unsupported entropy controls return on the original OFD | Absent descriptor capabilities retain their boundary; guest entropy credit/reseed APIs and GRND_INSECURE remain unsupported |
| BPF and cgroup device policy | Program administration uses the original registry/Files/hierarchy on the caller; target directory and control-file opens resolve directly | Existing unsupported BPF commands/types remain explicit errors; capability-less embeddings remain. Original device-access consumers retain the same live rules |
| Seccomp policy | Supported installation/TSYNC, decisions and lifecycle effects use original source resources; original policy/NNP is visible to proc, fork and exec | Capability-less embeddings remain; unsupported seccomp APIs are unchanged. Allowed calls still retain their own excluded resource/generic fallback |
| Mac host memory-pressure reclamation | Existing host adapter and original physical mapping gates; borrowed bytes use original release readiness | No pressure-only join remains. Actual refaults are backend work; explicit resident targets and final teardown keep their own semantics |
| Init notification and graceful-stop supervision | Existing host adapter, original retained notification socket and original PID 1 policy/private inbox | No notification/handshake/root-signal join remains; bootstrap and actual teardown keep their own lifetime |
| Diagnostic syscall tracing | Optional original-task metadata follows source/solo execution, parked waits, signals and lifecycle scopes; existing executors emit complete lines | No logging-only execution exclusion remains. Actual stderr writes can block their caller; this does not add guest ptrace support |
| Cold process observation and source ownership | Original body custody/current PID identity; queue membership orders later exit observation | Accepted source Handles retain their original authority. Actual excluded/legacy actors remain explicit |
| Host process and native terminal lifecycle | Host task creation/admission, original group control and UI native terminal open/close/input/resize/hangup progress on original resources and existing execution/disk workers | One-time product terminal initialization and genuinely missing/excluded providers remain explicit; terminal ID 0 transport no longer requests a join |
| Live host pause/resume | Original cohort, execution leases, shared pause flag and existing queue/workers | No pause-only join; initial/solo and legacy stopped-hook invocation retain their separate route |
| Host administration and generic dispatch fallback | Unknown syscall numbers finish locally using generated entry recognition; actual administration retains its own route | Cancel, admin/deadline/shutdown; accepted filesystem commands use original resources and the existing disk pool; known SVC resources/subcommands and HLT/fault exits not completed, parked or scoped on the worker still reach the joined fallback |

A concrete remaining supported call is `get_mempolicy(236)`: its address-policy
query still reads original owner `Memory::regions()`. It is an actual final
known-call boundary, rather than an unimplemented-number diagnostic. Future
memory-placement work must include that mapping-existence dependency.

The last row is an important coverage gap: the remaining owner work is **not
limited to a fixed set of rare controls**. The final worker-dispatch fallback is
still reachable for known resources and actual host/backend events. Initial admission and legacy/control rescheduling also involve
the owner; ordinary epoch-member migration, parking, wakeup and child/replacement
admission already use the live execution queue. Single-CPU/solo execution retains
its coordinated caller path, which does not by itself imply an extra RPC.

The [current consolidated audit](https://github.com/kb5000/trames-runtime/blob/75b9edadad2bf34884d8475765b7e3ab842f5044/docs/architecture/coordination-boundaries.md)
records scopes, actual admission gates and source locations. Earlier sections
below are historical records and do not override this inventory.

### Stage 74 init supervision closure (historical)

Stage 74 closes **systemd host notification and graceful-stop supervision**.
Both adapters drain the original authenticated notification socket while guest
executors continue. The shutdown handshake publishes one signal to the original
PID 1 task's policy/private inbox, without a Process/CPU borrow or a cohort join.
Monitor::pending, Mac admin_pending and the iPad notification-only stopped callback
are removed. Mac retains its existing 100 ms host-control interval; no new thread,
polling bus or ordinary guest check is introduced.

| Four-thread real init protocol fixture | Previous Dev | New Dev |
| --- | ---: | ---: |
| 4 CPUs: cohorts through shutdown | 4 | 1 |
| 12 CPUs: cohorts through shutdown | 4 | 1 |

The fixture rejects a non-init shutdown handshake, verifies CPU progress during
repeated readiness notifications, then receives signal 37 exactly once after PID 1
confirms its signal level. Both versions pass functional checks at 1/4/12 CPUs;
new multi-CPU execution returns no SVC to Kernel and requests no service. Native
macOS/Linux controls validate threads and signal completion, not native systemd
performance. These delayed protocol checks are not a throughput benchmark.

Both complete Mac/Linux runtime workflows and 406 process tests per host pass,
together with strict lint, the complete FFI test suite, both iPad compile targets
and disposable unmodified Debian systemd acceptance on one and four CPUs.
The actual iPad adapter regression verifies notification progress without a stop;
explicit pause still requests its existing callback. The [validation record](../validation/init-supervision-20261109.json)
records hashes, gates, native controls and limits. Only Mac Dev is updated;
stable 1.5.4, business environments and physical iPad installs remain unchanged.

### Stage 73 diagnostic tracing closure (historical)

Stage 73 closes the **diagnostic syscall tracing** category across all admitted
subsystems. Enabling Linux.trace, Mac --trace or interpreter --trace-syscalls no
longer disables descriptor/ring capture, waits, lifecycle/native checkpoints or
continuous source execution. Snapshot::owner_syscalls and the logging-only
admission exclusions are deleted.

Original entry arguments and completion metadata move with their task through
waits, interruption/restart, signals, fork/exec and joined handoff. Queue producers
only record results; existing execution threads emit complete lines outside queue
locks using optional reusable per-task scratch. There is no new logger/resource
thread, polling bus, guest permission check or memory fence. Disabled tracing
allocates no trace state or line buffer. Actual stderr writes can still block.

The completion audit also closes caught seccomp TRAP, sigreturn and immediate or
parked credential/namespace calls. New negative controls fail the intermediate
implementation's stale-entry attribution, then pass after correction. The same
fixture validates original arguments, file/socket bytes, mappings, signals and
fork/exec/wait on HVF and the interpreter at 1/4/12 CPUs, with native macOS/Linux
controls. Traced multi-CPU acceptance uses one cohort and zero returned SVCs.
The full socket-policy matrix retains exactly one BPF decision, traced or not.

| Four-thread marked getpid fixture, tracing on, 1,000 calls/thread | Before | After |
| --- | ---: | ---: |
| 1 CPU | 126.576 ms | 13.375 ms |
| 4 CPUs | 340.589 ms | 20.406 ms |
| 12 CPUs | 334.675 ms | 20.908 ms |

Three alternating pairs use exact signed runners and the final fixture.
Coordination removal and line batching both contribute. Untraced 20,000-round
controls vary from -0.03% to +0.55%; these are diagnostic fixtures, not browser or
Geekbench scores. Both complete runtime workflows pass; final completion/formatting
deltas pass 406 process tests per host (two existing ignored), strict lint, both
iPad compile targets and real backend acceptance. Unchanged CPU/Linux/network/
graphics checks are reused. The [validation record](../validation/syscall-tracing-20261109.json)
records hashes, exact samples, failed intermediate oracles and limits.
Only Mac Dev is deployed; stable 1.5.4, business environments and physical iPad
installs remain unchanged.

### Stage 72 embedding terminal closure (historical)

Stage 72 closes the **legacy embedding terminal ID 0** subsystem. Embedding
stdio, aliases, /dev/tty and /dev/pts/0 now use the same original PTY endpoints,
line discipline, direct ABI and task/job-control authority as guest terminals.
The independent Kernel terminal state/syscall body, signal scan, readiness copy,
byte pump and terminal-triggered cohort stop are deleted.

Host input, resize and signals update original resources while workers continue.
Potentially blocking native output uses the existing bounded file/image I/O pool;
there is no new pool, resource thread or coordinator. Partial output survives guest
flush and Runtime handoff, EOF only closes input, and resumed sessions rebind
signal authority without putting a route lock on ordinary bytes.

Both complete Mac/Linux runtime workflows, 405 process tests per host, strict core
lint and both iPad compile targets pass. Real HVF and interpreter acceptance covers
1/4/12 CPUs. With stdout blocked for three seconds, input sent at 0.2 s arrives
before output is drained, and all 8 MiB are verified. The old HVF runner misses the
two-second input deadline at every CPU count. Native macOS/Linux controls pass.
The [validation record](../validation/embedding-terminal-20261109.json) records
source/runner hashes, paired samples, initial failures, native controls and limits.
Only Mac Dev is deployed; stable 1.5.4, business environments and physical iPad
installs remain unchanged.

| Four-thread terminal query fixture, 1,000 iterations/thread | Before | After |
| --- | ---: | ---: |
| 1 CPU | 65.594 ms | 57.011 ms |
| 4 CPUs | 934.122 ms | 16.146 ms |
| 12 CPUs | 931.624 ms | 16.588 ms |

Three alternating exact-runner pairs include termios, PGID, SID and window queries.
At 4/12 CPUs, terminal ioctl cohort returns fall from 16,001 to zero, and the new
flow remains in one cohort through shutdown. Native controls are 18.759 ms on
Darwin and 1.303 ms on ARM64 Linux; their tty implementations and scheduling differ.
This does not imply a whole-browser or Geekbench speedup. Accepted blocking native
OS writes still finish during final pool teardown; arbitrary OS calls are not
made cancellable by this change.

### Stage 65 closed category and measured cost

The Unix ancillary owner-control category is deleted, including its mixed-set
lifetime, reaping-triggered join and accepted-message owner handoff. The existing
cycle graph remains object synchronization, rather than an execution service.

| Raw-policy workload, 200 iterations/thread | 4 CPUs before → after | 12 CPUs before → after | Native ARM64 Linux, 12 threads |
| --- | ---: | ---: | ---: |
| Ordinary SCM_RIGHTS, FD close/reuse and import | 8.825 → 8.547 ms | 32.060 → 30.733 ms | 3.391 ms |
| Filesystem destination through proc-FD path | 17.765 → 17.825 ms | 67.443 → 66.698 ms | 3.608 ms |
| Stream batch EPIPE with SIGPIPE | 34.953 → 2.172 ms | 211.687 → 7.202 ms | 1.523 ms |

Three alternating pairs use exact signed runners and the same native-validated
binary. The initial per-call private pathname projection caused a substantial
single-CPU regression and was removed before integration. At 20,000 rounds,
ordinary SCM_RIGHTS changes by +0.20%/+0.74% (raw/ALLOW); partial batches with
MSG_NOSIGNAL change by +1.60%/+2.07%, and pathname controls by -0.87%/+1.66%.
Very cheap stream/seqpacket error controls still cost +3.19% to +5.64%; this is not
a zero-overhead claim. The short raw ordinary-rights +46% sample is not hidden;
the longer paired control above distinguishes it from the corrected projection
regression. Native Linux runs on this Mac in the six-vCPU local CI environment,
with local stdout. No Firefox/Geekbench speedup is inferred.

Both complete runtime workflows pass (Mac VFS 563, Linux VFS 562; Linux ABI 253),
with 383 process tests per host, both iPad compile targets, 96 real backend cases
and 144 native Linux cases. Linux process acceptance uses a serial outer test
runner after unrelated seccomp tests exhibited one-epoch scheduling assumptions
under heavy parallel test load; their assertions and production scheduling are
unchanged. The old binary fails the corrected datagram/seqpacket signal oracle,
so those modes are compatibility fixes rather than paired timing comparisons.

### Stage 64 closed entry and performance

The retired entry included epoll creation, FD seek/stat, eligible stream bytes,
and network compatibility controls. This does not claim that every FD, terminal,
SCM_RIGHTS body, mixed poll or native/memory checkpoint is now source-complete.
Those independent boundaries remain in the table above. Public service counters
are retained as zero-valued diagnostic compatibility fields; the production
producer and handler are absent.

| Raw-policy workload, 200 iterations/thread | 4 CPUs before → after | 12 CPUs before → after | Native ARM64 Linux, 12 threads |
| --- | ---: | ---: | ---: |
| FD observations / epoll | 62.146 → 3.690 ms | 350.819 → 12.147 ms | 1.467 ms |
| Same plus scalar/vector console output | 73.318 → 19.643 ms | 410.691 → 69.180 ms | 3.212 ms |
| Same plus socket controls | 70.300 → 9.713 ms | 380.888 → 27.267 ms | 2.325 ms |

These are three alternating-order pairs against the previous runtime, using exact
signed runners. Native Linux runs in the local ARM64 CI environment on the same
Mac (six virtual CPUs); output goes to local storage rather than its evidence
sharing mount. It is not native macOS epoll. Longer one-core controls changed
between -2.27% and +0.61% over six combinations. Native console writes still pay
for the existing bounded I/O pool; absolute syscall cost remains above native
Linux. No whole-browser improvement is inferred from this fixture.

### Stage 63 entropy measurements

Three alternating pairs measure 200 64-byte requests per thread:

| getrandom policy | 4 CPUs before → after | 12 CPUs before → after |
| --- | ---: | ---: |
| Raw | 47.344 → 0.888 ms | 398.956 → 3.380 ms |
| ALLOW | 46.688 → 1.012 ms | 402.456 → 3.423 ms |

The short raw single-CPU and twelve-CPU random-device readv cases initially vary
by +7.63%/+13.31%. At 20,000 calls their medians change by -0.09%/+1.07%.
Already-local ERRNO controls at 100,000 calls range from -0.19% to +0.78%.
These samples do not prove universal zero overhead. Unchanged device paths' short
improvements are not attributed to a new byte-I/O algorithm. macOS getentropy and
native ARM64 Linux references use the same 64-byte granularity; their APIs and
background workload differ. These are entropy fixtures, not Firefox/Geekbench scores.

### Stage 62 memory-barrier measurements (historical)

Three alternating pairs measure 50 calls per thread:

| Scope | 4 CPUs before → after | 12 CPUs before → after |
| --- | ---: | ---: |
| Private expedited | 13.348 → 0.671 ms | 73.062 → 2.688 ms |
| Global | 12.565 → 0.755 ms | 75.511 → 1.697 ms |

Single-CPU calls remain near 0.1 ms for this fixture. Separate 100,000-call ordinary
identity and already-local ERRNO controls range from -0.35% to +1.16%; three samples
do not prove universal zero overhead. Native Linux references and raw samples are
recorded. Linux GLOBAL includes a real system-wide grace period and a different
population of kernel/userspace tasks, so its timings are not a direct primitive
comparison. These are synchronization fixture results, not Firefox/Geekbench scores.
The previous stage-61 short denied-control latency issue remains unattributed.

### Stage 61 unimplemented-call measurements (historical)

Three alternating unprofiled pairs run 1,000 calls per thread. Raw unknown calls
fall from **232.415 to 3.553 ms at four CPUs**, and **2020.502 to 9.456 ms at twelve
CPUs**. ALLOW-filtered unknown calls fall from **230.869 to 3.591 ms** and
**2045.767 to 10.080 ms**. Single-CPU raw/ALLOW results stay around 2.1 ms.

Filter-denied calls were already local. Their short four/twelve-CPU controls get
about 0.9/3.3 ms slower. With 100,000 calls, medians are approximately 2% higher at
four/twelve CPUs and 5% at one CPU, with a larger single-CPU outlier retained.
The fixed-latency cause remains unattributed; these results do not prove zero
regression or universal acceleration. Native Linux references, all raw samples
and the earlier build-overlapped run are recorded. Native Linux does not produce
the same per-call diagnostic; guest results are not Firefox or Geekbench scores.

### Stage 60 BPF measurements (historical)

Three alternating, unprofiled pairs measure 30 complete policy-management rounds
per thread; setup and thread creation precede the timed release. Raw four-CPU
work falls from **109.684 to 1.759 ms**, and twelve-CPU work from **988.187 to
5.834 ms**. ALLOW-filtered results fall from **122.323 to 1.558 ms** and
**992.495 to 6.261 ms** respectively. One-CPU results remain near 1.2 ms.

These are BPF management fixtures, not Firefox, Docker startup or Geekbench scores.
The final diagnostics contain no BPF (280) or cgroup target-open (56) cohort-return
entries. Other startup/control/console boundaries remain; aggregate service counts
are not claimed to be zero. Native Linux references and an earlier run overlapping
build activity are retained separately. macOS has no equivalent Linux BPF syscall.

### Stage 59 policy measurements (historical)

Five alternating unprofiled pairs measure 5,000 calls per thread. Filter-denied
calls fall from **932.887 to 6.124 ms at four CPUs**, and **5775.095 to 12.548 ms
at twelve CPUs**, by eliminating repeated whole-cohort returns. These are syscall
fixture results, not Firefox or Geekbench speedups. Unfiltered/ALLOW controls range
from roughly -5.3% to +2.8%; single-CPU controls stay within 1%. Native macOS and
Linux references, final-source confirmation, raw samples and limits are recorded
in the validation artifact. The coordinator and the explicit fallback rows remain.

### Stage 58 measurements and remaining costs (historical)

Nine alternating unprofiled pairs measure 100 mapped-file resizes while child
processes read existing aliases and repeatedly map new ones:

| Execution | One CPU / reader | Four CPUs / readers |
| --- | ---: | ---: |
| Stage 57 | 158.022 ms | 226.243 ms |
| Stage 58 | 5.885 ms | 34.260 ms |
| Native macOS | 0.465 ms | 0.653 ms |
| Native ARM64 Linux CI guest | 0.074 ms | 0.112 ms |

Separate diagnostic four-CPU runs reduce cohort rounds from 101 to one, with no
compatibility service requests. The remaining round is the final console boundary.
This is a fixture result, not a Firefox/Geekbench speedup. Native references use
their own page sizes and schedulers; Linux has three samples and six provisioned
CPUs. An earlier instrumented sample reached 813 ms; all nine unprofiled four-CPU
samples took 25–41 ms. The outlier is retained, with no unsupported causal claim.

Ordinary append/grow controls range from roughly -15% to +7% in short timed regions;
three samples do not prove a general improvement. Native comparisons still show
substantial original-mm pause, scheduling, hardware-exit and disk-dispatch costs.
Actual policy/control bodies, exceptional native import, legacy resources, host
administration and the general joined fallback remain in the inventory above.
The earlier stage-56 Docker cleanup timeout remains unattributed.

### Stage 57 measurements and final regression (historical)

| Fixed-work comparison | 4 CPUs | 12 CPUs |
| --- | ---: | ---: |
| Mapping + short compute, overall elapsed time | −9.1% | −24.8% |
| Mapping + longer compute, overall elapsed time | −18.5% | −43.7% |
| Plain vfork, end-to-end source lifecycle versus pre-source baseline | −11.0% | −14.3% |
| Vfork with control object, same baseline | −22.9% | −24.4% |
| Failed private-disk exec, incremental image-I/O cost | +10.5% | +6.3% |

These are fixture results, not Firefox/Geekbench speedups. Native macOS and native
ARM64 Linux references, exact checksums and all samples are retained. Short mapping
work increases summed peer wall latency even while total time improves; longer
work decreases it. Actual shared-mm pauses and startup overlap remain measurable.
Native macOS uses 16KiB pages; the Linux guest uses 4KiB pages.

The private-disk failure cost is about 2.8/1.8 microseconds per call. A short
four-CPU control-vfork mean was dominated by a large tail; nine 256-round alternating
pairs give plain +0.5%, control +6.3%. These residual costs remain recorded.

Long nonleader exec stress found and fixed a real cancellation defect: an unfinished
image could be mistaken for an installed replacement when the epoch stopped,
allowing the old guest to run past exec. The pending request now explicitly retains
its blocked syscall and accepted arguments. A deterministic before-fix failure is
retained; the fixed test passes on both hosts. Forty hardware runs perform 512
nonleader replacements each (20,480 total) in the previously failing private store.
Late completion after termination cannot revive a task.

Final acceptance includes 369 process cases per host (three ignored), all runtime
workspace checks on macOS/Linux, 66 lifecycle, 24 mapping and 24 private-store exec
guest cases at 1/4/12 CPUs, thirty Go GC starts and eight isolated Docker rounds.
The Wayland test entry needed the same type-recursion limit as existing product
entry points to avoid a pinned-nightly compiler ICE; no production code changed.
Its four optional graphical tests remain ignored in the default test suite.
Unchanged guest inputs and unrelated graphical acceptance are reused.

The earlier stage-56 Docker round-6 cleanup timeout remains unattributed; this
passing run is not evidence that its cause is fixed. Remaining work is recorded
in the table above, especially proc/cold-mm checkpoints, mapped-file layouts,
actual policy/control bodies and generic fallback.

## Twenty-second migration implemented — worker Unix namespace controls and waits — 2026-11-03

Runtime `fcfb8936848c36714a704c6c8ce30d9a2b86a347` moves Unix bind/connect and
accept/accept4 onto execution workers, shared by Mac and iPad. Pathnames use the
same VFS resolver, original filesystem/inodes, credentials and actual shared route
domain. Abstract addresses use the creating socket's original network namespace.
Excluded owner paths defer before effects and retain the accepted full OFD and
copied address bytes for the original owner resolver.

| Operation | Original shared authority / worker behavior | Remaining boundary |
| --- | --- | --- |
| Bind | Existing permissions and namespace, exclusive inode insertion; competing binders cannot overwrite the winner | Excluded owner-only path resolution |
| Connect | Original client and weak resolved target, existing listener queue and credentials | Native-service adapters and task-level signal effects |
| Accept | Original listener OFD, exact queued peer/preflight, actual hidden Files reservation; distinct concurrent FD publication | Structural task/namespace effects |
| Blocking controls | Internal Unix readiness subscriptions return the execution slot; original IP host waits/timeouts stay separate | Explicit original owner-resource fallback |

A backlog waiter keeps the listener's notification source, not its open lifetime.
Final listener close wakes it with the original refusal result, and replacing a
name cannot redirect the call. The accepting call itself retains its original full
listener OFD across close/reuse. Accept chooses the peer address and preflights that
same queue item under one guard; flags/capacity order remains unchanged. The guard
is released before notifications/output, and copyout or unwinding cannot publish a
partial FD or leak its reservation. No normal byte operation takes connection locks.

The one-slot test exposed a preexisting type assumption: any retained socket was
treated as an external host wait. Unix controls now select their actual internal
notification source, including headless runs without a host event hub. Worker task
identity also reaches DescriptorFactory in either capture order, preserving original
effective credentials while avoiding an inherited parent PID for child sockets and
SO_PEERCRED. These changes add no global socket/VFS lock, resource thread, bus,
polling loop, guest memory fence or native permission policy. Ordinary byte entry
keeps its original path without an extra duplicate control-admission probe.

| Four processes × 1,000 checked lifecycles, longest-child median | Previous | Current | Native macOS | Native ARM Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pathname connect / accept / alias / bytes / EOF / close | 191.70 ms | 52.21 ms | 12.11 ms | 4.81 ms | 3.67x |
| Abstract connect / accept / alias / bytes / EOF / close | 150.50 ms | 19.55 ms | Unsupported API | 5.10 ms | 7.70x |
| Existing socketpair / flags / options / bytes / shutdown / close | 22.24 ms | 21.83 ms | 4.87 ms | 4.74 ms | No significant change claimed |

Process-wall medians are **259.23 → 135.80 ms**, **189.53 → 57.80 ms**, and
**61.48 → 54.68 ms**. Separate diagnostic cohort counts are **3,004 → 49**,
**3,026 → 50**, and **48 → 48**. Service requests are **2,966 → 2**,
**3,004 → 3**, and **3 → 3**, including process startup/teardown. Residual
joins are recorded rather than presented as zero for a whole application.

Three unprofiled counterbalanced samples run after all compilation and FFI checks,
with local Linux CI stopped and four Trames vCPUs. Listener creation/bind/listen and
unlink are outside the session timer; client creation, connection, acceptance, alias,
data, EOF and final close are inside. Native Linux references are recollected after
all host work has finished using the exact static musl ELF on the existing six-vCPU
ARM64 machine. Native macOS uses its own APIs, with no abstract-socket substitute.
Different kernels, filesystems/API semantics and compiler targets make these contextual
references. No Firefox/whole-app/physical-iPad or generally native-speed claim follows.

Apple/Linux VFS **445/444**, ABI **184 each**, kernel **270 each**, strict checks,
Mac product **13**, both product boundaries, all **13** production FFI environment
checks and the iPad static build pass. Unchanged VFS results are reused after the
Linux-only identity/hot-entry changes; guest artifacts and unrelated acceptance are
unchanged. Physical iPad testing remains skipped per owner. Eight binders produce one
winner; eight acceptors produce 32 distinct FDs with exact peer addresses. Tests cover
output/panic rollback, target close/rebind, accepted endpoint reuse and owner handoff.

The filtered two-task lifecycle performs 192 control/byte/membership calls plus two
final waits, with 194 filter decisions and zero services on one/two execution slots.
Filtered/unfiltered blocking accept/connect performs nine calls; saturated-backlog
connect/accept performs seven. Both release a single execution slot and finish without
coordinator services. The guest CLOEXEC test uses the required shifted MOVZ encoding;
bounded rounds observe final parked tasks without changing production wait policy.

The goal remains active. Next are addressed unpaired-datagram I/O, Unix-bearing
ancillary owning graphs/cycle reclamation, native-service adapters, task/job control,
io_uring, mount/proc ownership and structural VM/task transitions. See the
[validation record](../validation/worker-unix-connect-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#worker-unix-namespace-controls-and-connection-waits-2026-11-03).

## Twenty-first migration implemented — complete Unix socket resources and worker lifecycle — 2026-11-03

Runtime `2b2194fee02e63b0ac6d0dcac11e9829206758ff` moves the actual Unix socket
control graph and full open descriptions to shared ownership, common to Mac and
iPad. This removes the ownership boundary that previously forced ordinary socket
creation, aliases, option controls and final close through joined execution.

| Resource / operation | Shared original authority and worker behavior | Remaining joined boundary |
| --- | --- | --- |
| Unix socket graph | Actual network identity, abstract namespace, pathname routes, names/credentials, connection, listener queue and options; derived Send/Sync | Bind/connect/accept and addressed datagram entry/wait admission |
| Creation and membership | socket/socketpair, actual Files reservations, full OFDs, dup/flags/close | Excluded legacy ancillary owning graphs |
| Short controls | Listen, address/peer queries, original Unix options and shutdown share one owner/worker ABI | Structural namespace and task transitions |
| Network namespace OFDs | Actual shared identity, IP/Unix resources and quota; aliases and portable rights retain original setns authority | unshare/setns, proc controls and mount/nsfs topology |

Connected byte/message I/O reads one canonical immutable binding; it does not lock
the connection graph or consult a second connection cache. A connection inherits
its original options before publication and keeps the client's preconnect readiness
source, preserving existing subscriptions without a forwarding thread. Locks leave
before notifications. Fresh socketpairs use normal Files publication rather than
invalidating every existing descriptor snapshot. There is no new global lock,
resource thread, bus, ordinary guest memory fence or native permission policy.

Accepted operations retain complete original OFDs across close/reuse; prepared maps
and epoll watches stay weak. Transport admission accepts once before choosing Unix
or IP. Owner message handoff receives that accepted lease directly instead of looking
up a number after another worker may have closed/reused it. Legacy Unix-bearing rights
remain excluded from portable sets: original owner graph/cycle checks stay in place,
and closed destinations reap their owner payloads on the existing joined boundary.
Actual endpoint close/notification still occurs immediately on its final worker drop.

Production FFI systemd boot exposed an old assumption that all shared raw byte reads
could finish without owner ancillary handling. Retained Unix byte handoff now covers
immediate and parked read/readv, using the same original socket and original ABI.
Tests close/reuse the FD, deliver legacy rights, verify original bytes and final peer
hangup, and confirm that replacement eventfds remain untouched. Final systemd/DBus
startup and both real FFI terminals pass.

| Four processes × 1,000 checked operations, longest-child median | Previous | Current | Native macOS | Native Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| Unix create / alias / flags / options / close | 394.28 ms | 13.21 ms | 5.01 ms | 4.07 ms | 29.85x |
| Socketpair / alias / flags / options / bytes / shutdown / EOF / close | 536.50 ms | 21.92 ms | 4.77 ms | 6.46 ms | 24.47x |
| Existing pipe-rights transfer / bytes / close | 10.48 ms | 10.21 ms | 1.51 ms | 1.97 ms | No significant change claimed |

Process-wall medians fall **432.49 → 51.49 ms**, **572.95 → 60.48 ms**, and
**48.67 → 45.46 ms**. Separate diagnostic cohort counts are **11,788 → 45**,
**16,327 → 50**, and **57 → 44**. Service requests are **6,038 → 3**,
**5,060 → 3**, and **3 → 2**, including process startup/teardown. These residual
counts are recorded, not presented as zero joins for a whole application.

Three unprofiled counterbalanced samples run after compilation, with local Linux CI
stopped and four Trames vCPUs. Creation is inside the first two loops; existing
rights transports are outside that timer. Native macOS uses its native Unix APIs;
native ARM64 Linux runs the identical static musl ELF on the existing six-vCPU CI
machine. Different kernels/API semantics are contextual references. This is neither
a Firefox/whole-app/physical-iPad measurement nor a claim of generally native speed.

Validation covers Apple/Linux VFS **441/440**, ABI **179 each**, kernel **267
unique cases each**, strict checks, Mac product **13**, both product boundaries,
all **13** production FFI environment checks, and the iPad static build. Physical
iPad testing remains skipped per owner. Eight creators exercise all three Unix
kinds, original credentials/network after unshare, rollback and last-close lifetime.
The filtered two-task regression executes 176 lifecycle/control/byte calls plus two
final futex waits with zero services and one filter decision per entry on one/two
slots. Linux exposed old signalfd/directory tests whose completed task could spin
away the shared instruction allowance. Those tasks and the equivalent PTY test now
park; only affected cases are rerun, reusing unchanged passes and guest artifacts.

The goal remains active. Next are worker Unix bind/connect/accept/addressed datagram
controls, portable Unix-bearing ancillary ownership and cycle reclamation, task/job
control and legacy PTY paths, io_uring controls, mount topology, proc controls and
structural VM/task transitions. See the [validation record](../validation/worker-unix-graph-20261103.json)
and [shared contract](../../runtime/docs/architecture/execution-backends.md#shared-unix-control-ownership-and-worker-creation-2026-11-03).

## Twentieth migration implemented — worker PTY creation and live device policy — 2026-11-03

Runtime `8bb98ca0706c2192ce01064c11b6e0d92035063e` shares the actual per-cgroup
device attachment graph and moves peer opening and real-devpts creation onto
execution workers, common to Mac and iPad.

| Operation | Shared authority and worker behavior | Remaining joined boundary |
| --- | --- | --- |
| Device policy | Original per-group program list/flags and immutable parent resources; owner attach/detach/query and worker checks use the same fields | Structural cgroup and task membership transitions |
| TIOCGPTPEER | Accepted original master OFD, original inode/mount, original credentials, actual live policy and hidden Files reservation | Excluded owner controls retain their original path |
| Real devpts creation | Common pathname resolver, actual PTY registry, atomic number/node publication and real shared OFDs | Legacy pseudo-devpts node cache; slave opens that can acquire a controlling terminal |
| Ordinary permissions | One unchanged Credentials permission implementation for owner and worker | No additional native or device permission policy is layered on top |

There is no effective-policy copy or allow/deny cache. Retained factories observe
ancestor attach/detach through the same original shared objects. Each group's
program list and attachment flags share one object lock; the cgroup/task graph
stays owner-local. Workers retain their actual current domain and credentials for
that epoch, like the existing namespace/credential captures. Checks use no whole
cgroup/VFS lock, new service thread or message bus.

Peer opening accepts once and keeps the existing type, flags, locked-master,
FD-capacity, inode-permission and device-policy error order. Failed opens roll back
the hidden reservation before opening a slave lease. The common constructor keeps
the original devpts resource after namespace changes and CLOEXEC remains per FD.

Concurrent allocation previously searched for a local slave number separately from
insertion. It now selects and inserts under one devpts filesystem write guard, while
the existing registry lock protects scheduler ID selection/publication. Eight worker
creators retain distinct scheduler IDs and local numbers and the original slave
nodes; final closes remove their own nodes. Locks cover allocation metadata and do
not span ordinary terminal I/O or notification callbacks. PTY admission checks the
shared resource boundary before allocation, rather than walking ancestry on every
node lookup. Slave pathname opens without O_NOCTTY still defer before task effects.

| Four processes × 1,000 checked operations, longest-child median | Previous | Current | Native Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: |
| Peer open / CLOEXEC query / write / read / close | 98.28 ms | 9.83 ms | 25.67 ms | 10.00x |
| Real-devpts master create / unlock / peer / flags / I/O / close | 229.34 ms | 34.94 ms | 54.14 ms | 6.56x |

Process-wall medians fall **135.88 → 46.90 ms** and **274.48 → 75.56 ms**.
Separate diagnostic cohort counts fall **2,574 → 61** and **5,375 → 58**;
service requests remain **3 → 3**, including startup/teardown. These counts are
reported rather than claiming all whole-cohort joins have disappeared. The remaining
joins have not all been attributed; task/PTY control, legacy pseudo resources and
startup/teardown still require separate examination.

Three unprofiled counterbalanced samples run after compilation and with local Linux
CI stopped. Peer setup is outside its loop; full creation is inside the second loop.
The full-creation guest mounts one real devpts instance before launching the children.
Native Linux runs the identical static musl ELF against its original /dev/ptmx on the
existing six-vCPU ARM64 CI machine. Linux's native TTY implementation differs, so the
lower Trames loop times establish neither generally faster native execution nor a
Firefox/whole-app/iPad speedup. No native macOS variant of Linux-specific controls is
substituted.

Apple/Linux VFS checks pass **436/435**, ABI **178 each**, kernel **264 each**, and
Mac product **13**, with strict checks and both product boundary checks passing.
The worker regression performs 80 creation/control/close calls plus two final futex
waits on one/two slots, with zero services and one filter decision per entry. Completed
test tasks park instead of burning the shared instruction budget while a peer finishes
host metadata. Only that affected test is rerun after the wait adjustment; unchanged
full-suite results are reused. Real tests also cover live ancestor policy, original
credentials, exact errors, rollback, eight simultaneous allocations and cleanup.
All 13 production FFI environment checks and the iPad static build pass. Physical iPad
testing remains skipped per owner; guest artifacts and unrelated acceptance are reused.

The goal remains active. Pending work includes the legacy pseudo-devpts allocation
cache, controlling-terminal and task signal/hangup paths; full Unix socket creation,
listener/connection/addressed datagram ownership; io_uring controls; network/mount
namespace graphs; and task/structural VM transitions. See the
[validation record](../validation/worker-pty-creation-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#worker-devpts-creation-and-live-device-policy-2026-11-03).

## Nineteenth migration implemented — complete shared PTY endpoints — 2026-11-03

Runtime `658f8d5b51089a7c490d9959ff47b7d8ad07f71a` preserves the shared
ownership commit `b4a2352fb9073ad557191ec2f96687033bdb6714` and its tested
joined-retry correction. Mac and iPad use the same endpoint implementation.

| Resource or operation | Shared authority and worker behavior | Remaining joined boundary |
| --- | --- | --- |
| PTY master/slave OFDs | Actual Arc endpoint, original slave inode/devpts mount and terminal queues; dup/close, flags and portable Unix rights | Initial allocation, namespace/permission/device policy and TIOCGPTPEER |
| Short PTY controls | TIOCGPTN, TIOCSPTLCK, TIOCGPTLCK, TIOCPKT and TIOCGPKT use the same accepted OFD and common ABI implementation | Remaining terminal ioctls and session/controlling-terminal changes |
| PTY byte waits | Ordinary reads/writes park on their original resource; accepted operations retain the full endpoint across close/reuse | Background job checks and input requiring task signal delivery |
| Last close and host notifications | Real slave count, original devpts unlink and wakeups; shared weak registry and relaxed state bits | SIGHUP/SIGCONT and host resize/signal routing to tasks |

No owner-local Rc graph crosses the worker boundary. Prepared byte/poll captures
remain weak; an accepted syscall owns the actual complete OFD. Independent slave
opens create distinct endpoint leases, while aliases and rights share the original
open. Final master drop only unlinks its own devpts inode, including when its last
accepted operation finishes on a worker. Registry locks are released before terminal,
filesystem and wake callbacks. No new service thread, bus, global terminal lock or
ordinary-memory fence is introduced. Concurrent host signal bits use fetch-or/swap.

Production FFI validation exposed an old joined-byte assumption: it could not receive
a shared PTY, so initial shell input triggered an unreachable assertion. Joined retry
now receives the original terminal authority. Task-level signal fallbacks retain the
accepted endpoint and original call arguments, without storing an extra argument copy
on every byte operation. A regression closes/reuses the slave during input wait and
the master during control-C handoff, then verifies original bytes, signal and last
close. The published ownership commit is retained with the correction as a follow-up.

| Four processes × 1,000 checked operations, longest-child median | Previous | Current | Native Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: |
| PTY dup/flags/three short controls/close | 384.37 ms | 9.41 ms | 1.30 ms | 40.84x |
| PTY Unix-rights transfer/write/read/close | 244.38 ms | 10.89 ms | 26.18 ms | 22.45x |

Process-wall medians are **421.84 → 44.75 ms** and **282.92 → 48.62 ms**.
Separate diagnostic cohort counts fall **13,387 → 46** and **7,735 → 63**;
service requests remain **3 → 3**, including process startup/teardown. The loop
creates its original PTY outside the timer. Three unprofiled counterbalanced
repetitions run after compilation and with local Linux CI stopped. Earlier samples
that overlapped product checks are excluded. The native reference runs the exact
static musl ELF on the existing six-vCPU ARM64 Linux machine. It exercises Linux's
TTY implementation, so the faster rights loop does not establish generally faster
native execution. There is no substituted native macOS version of these Linux PTY
control calls, nor a whole-app/Firefox/physical-iPad speedup claim.

Apple/Linux VFS coverage is **433/432** passing cases, ABI **177 each**, kernel
**263 each**, and Mac product **13**; strict checks pass. Passing unchanged cases
are reused and affected byte/dispatch checks rerun for the correction. A filtered
two-task test performs 112 lifecycle/control calls on one/two slots with zero
coordinator services and one filter decision per entry. Real ancillary tests cover
source FD reuse, original master/slave identity and final endpoint release. All
13 real FFI environment checks and the final iPad static build pass. Physical iPad
testing remains skipped per owner; guest artifacts and unrelated CPU/memory/graphics
acceptance are unchanged and reused.

The overall goal remains active. Next ownership boundaries include PTY creation/peer
open and job control, full Unix socket creation/listener/connection/addressed datagram
graphs, io_uring controls, network/mount namespaces and nsfs topology, pending task
signal queues, and structural file/VM/task transitions. See the
[validation record](../validation/worker-shared-pty-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#shared-pty-ownership-and-worker-controls-2026-11-03).

## Eighteenth migration implemented — shared descriptor controls — 2026-11-03

Final runtime `30accf899f60f4a47a558c4c763bcc546c6aaeb3` includes the tested
ownership migration `dfbac7a34f5f24a7ef9c5dd3e3bf54476fa205aa` and a directory
lock-scope correction discovered during its regression measurements. Both
products retain these exact commits and pin the final runtime.

| Resource | Shared authority and worker operations | Remaining owner boundary |
| --- | --- | --- |
| signalfd | One actual atomic mask; create/update, dup/close and Unix rights | Reads and signal-dependent poll/epoll use the reading task's original pending queues. |
| BPF descriptor | Same immutable verified program, registry identity and last-reference quota; dup/close and rights | Program verification/load/attachment and task permissions retain their original paths. |
| UTS/PID namespace descriptor | Original shared namespace object, metadata and descriptor lifetime | Proc open, unshare/setns and task transitions remain joined. |
| IPC/cgroup namespace descriptor | Actual Arc identity, weak snapshots and atomic quota lease; one initialized mqueue filesystem | Task/namespace mutation remains joined; network/mount namespace graphs and nsfs mount topology stay excluded. |

The mask uses relaxed atomic access without a task/global lock. Updating an
existing signalfd preserves per-FD CLOEXEC and its original status flags. Common
owner/worker input decoding retains the original size, fault, flag and type-error
order. Shared descriptions retain the actual objects, not mirrored state or an
owner-service endpoint. Proc namespace reopen preserves that capability in a
fresh OFD; mount association is supplied before ownership classification so a
temporary shared projection cannot discard an nsfs mount pin.

Five resource tests cover whole-mask updates, current-task readiness, alias
flags/fdinfo, original namespace types/permissions, nsfs mount pins and worker
last-close quota reclamation. Real ancillary ABI tests transfer signalfd, BPF and
namespace descriptors after sender close/reuse. A filtered two-task lifecycle
test performs 80 calls on one/two execution slots with zero coordinator services
and one filter decision per entry. Another test proves inherited signalfd reads
consume the current task's queue, preserving the creator's pending signal.

| Four processes, 1,000 operations each; longest-child median | Previous | Current | Native Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: |
| signalfd create/dup/update/close | 383.88 ms | 9.55 ms | 1.86 ms | 40.22x |
| signalfd Unix-rights transfer/update/close | 317.78 ms | 9.88 ms | 1.81 ms | 32.15x |

Process-wall medians fall **420.35 → 47.23 ms** and **352.97 → 48.88 ms**.
Separate diagnostic cohort counts fall **14,013 → 52** and **10,610 → 57**;
service requests remain **3 → 3** in both cases. This removes repeated
whole-cohort joins. The native reference executes the same static musl ELF on
the existing six-vCPU ARM64 Linux CI machine. No native macOS signalfd API is
substituted. Three unprofiled counterbalanced samples are used, with that CI VM
stopped during Mac measurements. No whole-app/Firefox/iPad speedup is inferred.

The directory regression uncovered a real baseline deadlock, not a slow timing
sample. getdents held a filesystem read guard while parent/mount resolution
could acquire that same lock again. A queued writer could then wait for the
first guard while blocking the recursive reader. Sampled hardware stacks show
readers in parent/root-path resolution and writers in insert_entry. Child
metadata now comes from a closed helper returning owned values; namespace
resolution and encoding cannot retain that helper's guard. No new lock, check,
barrier, service hop or serialization of unrelated operations is added.

The corrected concurrent regression fails on the isolated committed unfixed
runtime at its five-second progress deadline, and passes in about 30 ms after
the fix. Its 512 created names remain within the existing memory fixture quota.
Six final hardware cases also pass concurrent first-time creation/enumeration
with fresh private/shared directories, three each. The failed baseline sample,
explicit stack reproduction and negative control are retained in the record.

Directory timing is reported separately with `--prepare-directory`, which
prepares identical contents sequentially before hot-loop measurements:

| Directory hot loop | Previous | Current | Native macOS |
| --- | ---: | ---: | ---: |
| Private | 50.24 ms | 48.36 ms | 33.19 ms |
| Shared host | 158.98 ms | 164.63 ms | 33.19 ms |

Private time falls about 3.7%; shared-host time rises about 3.6%. Three samples
are insufficient to attribute these small differences to this change. They are
not presented as additional directory speedups. The unchanged exact-ELF native
Linux directory reference (6.62 ms) is reused from the previous stage.

Final Apple/Linux VFS coverage is 432/431 passing cases. Full ABI (175 each),
kernel (261 each), strict checks and Mac product checks (13) passed for the
ownership migration. Only affected directory cases and failed permission-bound
VFS cases were rerun for the follow-up, reusing unchanged passing checks.
All 13 real FFI environment checks and the iPad static build also pass with
the final fix. Physical iPad tests remain skipped per owner. The final measured
runner is installed in Trames Dev; the stable application is unchanged.

The overall goal remains active. Full Unix creation/listener/connection/addressed
datagram ownership, PTY/ring/network/mount control graphs, pending signal queues,
remaining native/OFD controls and task/structural VM/directory transitions still
need migration. See the [validation record](../validation/worker-shared-controls-20261103.json)
and [shared contract](../../runtime/docs/architecture/execution-backends.md#shared-descriptor-controls-and-worker-signalfd-2026-11-03).

## Seventeenth migration implemented — worker directories and memfd ownership — 2026-11-02

Runtime `748d75e2cfed670d041f9a75e1e24152d7068dae` makes static directory and
memfd descriptions complete shared OFDs. Ordinary directory open, getdents64,
seek, dup and close now run on execution workers. Owner and worker enumeration
use the same actual cursor and dirent encoder; directory offsets stay coherent
across aliases, fork, fdinfo and Unix rights. Only the affected OFD's cursor
commit is locked, after namespace lookup and before guest-memory output.

Accepted enumeration retains the original descriptor before output preflight,
so concurrent numeric FD reuse cannot redirect it. Dynamic pseudo directories
and excluded mounts still defer before cursor effects. O_PATH reads return
EBADF on both paths. Shared directory leases also preserve lazy-unmounted
subtree traversal and release the original mount topology after last use.

Directory and memfd Unix rights can now use the existing portable worker path.
The received object keeps its real offset, storage, seals and backing even after
the sender closes/reuses its descriptors. This grants no additional byte or VM
capability: sealed/shared storage remains excluded from FileEpoch's ordinary
file fast path. io_uring constructs its backing before publishing its final
owner OFD, because its ring control graph remains owner-local.

Five directory tests cover concurrent cursors, reuse/faults, dynamic fallback,
detached traversal and live host contents. ABI tests compare worker/owner errors,
output and cookies. A two-task filtered test runs 160 directory lifecycle calls
on one and two execution slots with zero coordinator services and exactly one
filter decision per entry. Apple/Linux VFS (426/425), ABI (174 each), kernel
(259 each), strict checks, Mac product (13), real FFI environment checks (13)
and the iPad static build pass. Physical iPad tests remain skipped per owner;
unaffected standalone CPU/memory/graphics checks are reused.

| Directory loop, longest-child median | Previous | Current | Improvement |
| --- | ---: | ---: | ---: |
| Private directory | 519.25 ms | 47.73 ms | 10.88x |
| Shared host directory | 832.85 ms | 167.92 ms | 4.96x |

Native macOS takes **35.39 ms** using its native libc APIs. Native Linux takes
**6.62 ms** with the exact static musl ELF on its own filesystem in the existing
six-vCPU ARM64 CI environment; a supplemental glibc reference takes 6.02 ms.
An initial reference accidentally placed the native directory on virtiofs and
is explicitly excluded. Four processes each perform 1,000 checked
open/scan/rewind/scan/close iterations over 16 entries. Timings use three
unprofiled counterbalanced samples, with the CI VM stopped for Mac runs.
Private/shared process-wall medians fall **560.98 → 87.54 ms** and
**878.82 → 209.04 ms**. Separate diagnostic cohort counts fall
**14,555 → 51** and **14,607 → 52**. Whole-command diagnostic service counts
are still nonzero and include setup/teardown; the zero-service assertion applies
to the isolated directory test. No Firefox or iPad overall speedup is inferred.

The measured runner is installed in Trames Dev; the stable binary and running
sessions are preserved. Both products pin the same tested runtime commit.

| Remaining ownership boundary | Next work |
| --- | --- |
| Unix creation, listener/connection controls, addressed datagrams | Share the actual socket graph; its ancillary ledger can still retain owner-only resources. Directory/memfd dependencies are removed in this stage. |
| PTY job control/master lifetime, signalfd, namespaces and ring descriptions | Move their actual control/lifetime state before allowing the full Unix graph to cross workers. |
| Directory/namespace mutations and structural file/VM/device transitions | Preserve original namespace and mapping commit boundaries while reducing the affected stop domain. |
| Task, signal and process lifecycle | Shared task/resource authority with object-level synchronization, retaining existing blocking-slot release. |

The overall coordinator-removal goal remains active. See the
[validation record](../validation/worker-directories-20261102.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#worker-directories-and-portable-memfd-ownership-2026-11-02).

## Sixteenth migration implemented — worker Unix ancillary messages — 2026-11-02

Runtime `245eb520bb19682fd01b325e1343f7e2dedf9ed7` moves paired Unix stream/datagram/seqpacket
sendmsg/recvmsg onto executing workers through one original ancillary ABI.
Portable rights travel as the actual shared OFDs in their packet, without the
owner-only rights ledger. The original identity, offsets, flags, close lifetime,
MSG_PEEK, CMSG_CLOEXEC, truncation, credentials, timestamps and existing Files
limits remain intact. Owner-only or mixed unsupported rights defer before effects.
The full coordinator-removal objective remains active.

Accepted waits import the message header, vectors, credentials and source OFDs
once. They retain the message through source FD close/reuse, changed guest headers
and cohort changes. Receive rechecks original output ranges after a possible VM
change before consuming input. Existing resource subscriptions return execution
slots. Filtered calls retain exactly one accepted decision, while the next SVC
uses the current filter. Ready messages need no retained-message allocation,
empty rights avoid Files locking, and receive options are captured under the
existing channel lock instead of taking extra option locks.

The Unix control graph still has owner projections. On handoff, the joined owner
retains the original local OFD before any returned owner mutation executes; that
lease stays outside worker state and drops on completion, signal or exit. Fallback
uses the original transport and accepted message. Tests close/reuse the receiver's
numeric endpoint while preserving the accepted receive, and reuse a blocked
sender's source FD while proving the original right and bytes are delivered.
Stopped tasks are now excluded before I/O eligibility, so a retained wait cannot
accidentally resume guest execution while stopped; SIGCONT restores the wait.

Five new direct-message tests cover cross-thread independent Files, original OFDs,
credentials/timestamps, peeking, truncation, faults, mixed owner rights, blocked
source reuse and owner fallback. A single-worker filtered cohort test covers
cross-cohort receiver FD reuse, stop/continue and a later denying filter, with
zero coordinator service requests and one decision per entry. Apple/Linux VFS,
ABI, affected kernel and strict checks pass, together with Mac product tests,
13 actual FFI checks and the iPad static build. A mistaken test expectation for
SIGCONT was corrected and rerun alone; unaffected passing checks were reused.
Targeted final Mac hardware runs also pass real descriptor transfers on paired
datagram and seqpacket transports (32 checked iterations per process, four
processes, worker execution verified). These are functional checks, not timing
comparisons. Physical iPad checks remain skipped per owner.

| Longest-child loop median | Previous | Current | Native macOS | Native Linux reference |
| --- | ---: | ---: | ---: | ---: |
| Unix FD transfers, 1,000 each | 173.38 ms | 10.87 ms | 3.46 ms | 1.87 ms |
| TCP epoll byte cycles, 2,000 each | 28.89 ms | 29.38 ms | 19.51 ms | 5.54 ms |

Unix descriptor-transfer loops improve **15.95x**, and process wall time
falls **210.02 → 49.52 ms**. A separate diagnostic sample
reduces cohort rounds **5,109 → 52**. Service requests remain
3 → 3: this migration removes repeated whole-cohort joins,
not an already-rare service hop. The sustained TCP/epoll comparison stays
approximately flat. Native macOS remains about 3.14x faster in the
rights loop; no whole-app, Firefox or iPad speedup is inferred. Four processes
perform identical checked work, including real I/O through each imported FD and
its close. Timings use three unprofiled counterbalanced runs. The native Linux
reference uses the exact ELF/work on the existing six-vCPU ARM64 CI machine.

The final measured runner is installed in Trames Dev, preserving running sessions
and the stable application. Unix creation/control, addressed datagrams,
owner-only rights, native service pumping and task/VM/device ownership remain
separate migration work. See the [validation record](../validation/worker-unix-rights-20261102.json)
and [shared contract](../../runtime/docs/architecture/execution-backends.md#worker-unix-ancillary-messages-2026-11-02).

## Fifteenth migration implemented — retained blocking socket waits — 2026-11-02

Runtime `b69b222fdb875ffbcae160d22da159f552f8a7ec` moves blocking TCP connect/accept/accept4
onto executing workers. A task waiting for a connection releases its execution
slot. The original socket OFD, creating Files/credentials, absolute deadline and
handshake phase survive cohort changes and the existing owner fallback. Numeric
close/reuse cannot retarget the call, and connect never repeats its side effects
or rereads a changed peer buffer. The overall coordinator-removal goal remains active.

The wait uses existing native edge queues and the packet stack's existing
transport/timer sources. Stale edges are drained before the deciding retry;
existing workers dispatch events and rearm the operation. No reactor/resource
thread, polling bus, new quota or ordinary guest-memory synchronization is added.
Normal native TCP bytes retain their existing implementation. Cold admission now
recognizes related full socket OFDs, including listeners, so a connected peer or
an all-blocked related cohort can retain worker execution without a GUI service.

One-host-worker tests prove blocked accept releases the slot for a connecting
peer, with zero coordinator service requests under both unfiltered and filtered
entries. Other tests cover all-blocked adoption, original Files after close/reuse,
idle listener events, virtual handshake recovery and timeout, missing-reactor
fallback, later filter changes, and signal/restart cleanup. An accepted entry
does not repeat BPF; the next SVC uses the current filter. Stop retains the
operation; caught signals, exit and exec release it. Native accept output and
publication use the existing shared ABI. Apple/Linux affected checks, strict
checks, Mac product tests, 13 real FFI checks and the iPad static build pass.
Physical iPad checks remain skipped per owner; unaffected acceptance is reused.

| Longest-child loop median | Previous | Current | Native macOS | Native Linux reference |
| --- | ---: | ---: | ---: | ---: |
| Blocking TCP sessions, 256 each | 69.85 ms | 23.48 ms | 19.16 ms | 4.64 ms |
| Nonblocking TCP sessions, 256 each | 21.88 ms | 22.53 ms | 16.27 ms | 3.78 ms |
| TCP epoll byte cycles, 2,000 each | 29.16 ms | 29.59 ms | 19.45 ms | 5.54 ms |

Blocking sessions improve **2.97x**, with process wall time
102.36 → 55.50 ms. A separate diagnostic profile
reduces service requests **731 → 3**, including startup/teardown.
Nonblocking sessions and sustained TCP/epoll remain approximately flat. The
blocking fixture measures complete connection lifecycles; the dedicated tests,
rather than a throughput timing, prove actual wait/slot handoff. Four processes
perform identical checked work; timing uses three unprofiled counterbalanced
samples. Native Linux uses the existing six-vCPU local CI machine and the exact
same ELF/work; it is a reference, not Trames. No Firefox/whole-app speedup is inferred.

A first combined connection-heavy run failed a connect assertion. Adding failure
diagnostics reproduced Linux EADDRNOTAVAIL (99) on both the old and new runners:
native macOS had about 16,360 TIME_WAIT entries against its 16,384-port ephemeral
range. The independent nonblocking rerun passes, and the native blocking reference
above is measured again after those connections drain. Host settings and runtime
limits are unchanged. This is preserved in the validation record rather than
discarded as an unexplained retry; the fixture now prints the actual connect error.

The final measured runner is installed in Trames Dev, without restarting existing
sessions or changing the stable application. Continue with excluded Unix socket/
SCM_RIGHTS ownership, then task and structural VM/device boundaries. See the
[validation record](../validation/worker-socket-waits-20261102.json) and
[shared wait contract](../../runtime/docs/architecture/execution-backends.md#retained-blocking-socket-controls-2026-11-02).

## Fourteenth migration implemented — worker full socket controls — 2026-11-02

Runtime `5acb2fb7a0b68348cd2c893937d706887f4791b0` moves bind/listen and nonblocking TCP connect/accept/accept4
onto executing workers. Address, option, timeout and shutdown controls use the
complete shared socket OFD, including virtual sockets, rather than requiring the
old limited native projection. Blocking UDP/netlink connect is immediate and also
runs there. Blocking TCP connect/accept still defer before effects to their
original scheduler; their event wait and cross-cohort lifetime remain next work.
The overall coordinator-removal objective is still active.

Production owner and worker entries invoke one original control ABI. The creating
credentials, low-port checks, namespaces, error ordering, flags, existing bounds
and output preflight remain intact. Filtered worker entries carry exactly one
seccomp decision through a completed call or joined fallback. AF_UNIX, tracing and
mutable authority retain their original boundaries. Explicit native-only captures
keep compatibility, while product captures carry the real descriptor factory.

accept preflights output, reserves the actual Files slot, consumes the connection,
writes output and prepares the new original OFD's native TCP view before its
number becomes visible. It never re-looks up the new number for publication.
Control mutation plus the resulting byte-capability/timeouts view reuse the
existing per-OFD admission/publication transaction. No Files operation occurs
under that mutation callback; accept owns its separate unpublished description.
Pure queries/shutdown avoid unnecessary republication. Native bytes, quotas and
guest-memory synchronization are unchanged; no service thread or polling bus is
added.

Native/virtual complete lifecycle and permission/error tests pass. Two CPUs run
all ten control numbers under native/virtual networking and unfiltered/allow/deny
policies: **60 cases**, zero coordinator service requests for allowed calls, and
one decision per filtered syscall. Publication contention also proves actual
control mutation waits for its transaction while unrelated Files remain usable.
Affected Apple/Linux ABI and VFS checks, strict checks, 20 service regressions,
Mac product tests, 13 real FFI checks and the iPad static build pass. Physical iPad
verification remains skipped per owner; unaffected acceptance is reused.

| Longest-child loop median | Previous | Current | Native macOS | Native Linux reference |
| --- | ---: | ---: | ---: | ---: |
| Complete TCP sessions, 256 each | 29.07 ms | 22.08 ms | 15.11 ms | 3.78 ms |
| TCP state/options cycles, 2,000 each | 56.02 ms | 53.64 ms | 16.55 ms | 5.99 ms |
| TCP epoll byte cycles, 2,000 each | 29.05 ms | 29.42 ms | 19.85 ms | 5.54 ms |

Complete sessions improve **1.32x**, with process wall time
61.50 → 53.81 ms. State/options and sustained
TCP/epoll are approximately flat. In a separate session diagnostic sample,
service requests fall **3,769 → 3**. Counters include startup/teardown and
nonblocking retries; they are not CPU utilization. Four processes perform checked
fixed work; timings are medians of three unprofiled counterbalanced runs. Native
Linux references are reused after exact ELF/kind/work identity checks and use the
local six-vCPU CI machine; native macOS uses corresponding kqueue APIs. No
whole-application or Firefox speedup is inferred.

The final measured runner is installed in Trames Dev, without restarting existing
sessions. Continue with retained blocking TCP connect/accept waits, then excluded
Unix/task/VM resources. See [the validation record](../validation/worker-network-control-20261102.json)
and [the shared control contract](../../runtime/docs/architecture/execution-backends.md#worker-full-socket-controls-2026-11-02).

## Thirteenth migration implemented — complete socket ownership and worker creation — 2026-11-02

Runtime `0104a848c7a130d32c782f45176d88734985a737` makes the complete network namespace, socket and virtual
fabric graphs Send+Sync by composition. Actual socket ownership now lives in the
shared OFD. Production IP/netlink socket creation (syscall 198), duplication and
final close complete on the executing worker through the original Files domain.
The creating namespace, credentials, permissions, flags, error ordering and
existing limits are preserved; AF_UNIX defers before effects.

The socket owns its control gate; VFS no longer keeps an independent publication
gate. Scalar metadata uses relaxed atomics and compound invariants use object
locks. Packet-stack observers retain metadata without close callbacks, avoiding
last-reference teardown under the fabric lock. Port/quota admission, cold stack
creation and lazy binding/DNS/native/guest initialization are atomic transactions.
Ordinary direct native TCP bytes keep their existing path. No new unsafe trait
override, global native-I/O lock, service thread, bus, spin loop or quota is added.

An admitted owner/service call retains its original socket OFD across eligibility,
flags, accept/address copyout and TCP publication. Another worker can close/reuse
the numeric FD without retargeting that operation. A deterministic regression
accepts from a retained original listener while the reused FD names a different
listener. Original epoll edge/oneshot and io_uring final-close behavior is retained;
shared ownership alone never grants direct byte/poll capability.

Two-CPU creation/close loops now make **zero coordinator service requests**.
Allow/deny seccomp loops prove one decision per syscall. Mac/ARM64 Linux affected
checks pass: 79 network unit + 11 integration, 420/419 VFS respectively, 161 ABI,
9 worker-dispatch, 20 service and one multi-case seccomp regression. Strict product
checks, 13 real FFI checks (Alpine/Debian/custom/systemd and dual PTYs), Mac product
checks and the iPad static build pass. Physical iPad testing remains skipped per
owner; unchanged CPU/memory/graphics acceptance is reused.

| Longest-child loop median | Previous | Current | Native macOS | Native Linux reference |
| --- | ---: | ---: | ---: | ---: |
| TCP creation/configuration/close, 10,000 each | 503.84 ms | 171.00 ms | 52.96 ms | 19.79 ms |
| Complete TCP sessions, 256 each | 44.26 ms | 27.83 ms | 15.32 ms | 3.78 ms |
| TCP epoll byte cycles, 2,000 each | 29.19 ms | 29.24 ms | 19.29 ms | 5.54 ms |

Creation/configuration/close is **2.95x faster**; complete sessions are
**1.59x faster**; sustained TCP/epoll is approximately flat. Four processes
perform the stated work with checked results. Timings are three unprofiled,
counterbalanced runs. In a separate creation diagnostic sample, service requests
fall from 79,982 to 3; these counters include startup/teardown and retries,
and do not establish CPU utilization. Native Linux creation is measured afresh;
other Linux references are reused only after exact fixture ELF identity checks.
Linux uses the local 6-vCPU CI machine and a different kernel; native macOS uses
corresponding kqueue APIs. No Firefox or whole-application speedup is inferred.

The final measured binary is installed in Trames Dev. Running sessions were not
restarted. Remaining work includes worker bind/connect/listen/accept, blocking and
policy-aware byte paths, Unix/SCM_RIGHTS, task/signal and structural VM/device
ownership. The coordinator-removal objective remains in progress.
See [the validation record](../validation/shared-network-20261102.json) and the
[shared socket contract](../../runtime/docs/architecture/execution-backends.md#shared-complete-sockets-and-worker-creation-2026-11-02).

## Twelfth migration implemented — shared live routing resources — 2026-11-02

Runtime `4021141d5064d5f554f7bf94e0efa80ac34f4619` shares the actual topology/View, ruleset, conntrack
and loopback resources by composition. These resources contain no owner Rc graph;
complete namespace/fabric/socket projection lifetime and production socket factory
routing still need migration. No new worker opcode is claimed in this stage.

Topology queries use concurrent readers; structural changes alone need a writer.
Connection/NAT invariants synchronize per namespace, and a route releases that
state before the next namespace/bridge. Configuration handlers capture resources
and leave topology before taking their transaction lock. Nested read reacquisition
was removed to avoid deadlock when a writer is pending.

Packet accounting no longer takes a topology writer. Stored links hold the actual
relaxed atomic counters; public Device remains a reporting snapshot, and moving
a link preserves its counter resource. Unchanged native metadata refresh likewise
needs only a reader, allocates no address vector, and never performs host interface
queries under topology. Changed metadata retains the original writer transaction.

Rule batches and ctnetlink changes hold their actual object lock from cloning
through validation, output-capacity preflight and publication. Concurrent batches
cannot overwrite a successful generation; deletes cannot restore deleted flows
or erase newly inserted packet state. Rule counters use relaxed atomic updates,
and publication preserves packet increments. The original protocol, permissions,
rollback, limits and private redirect previews are retained. No global network
mutex, new resource-service thread, message bus or polling loop is introduced.

Concurrency coverage proves another namespace progresses while one is locked,
topology readers coexist, exact packet/drop totals need no writer, link moves
preserve counters, unchanged native refresh progresses under a held reader, all
concurrent rule batches publish, live publication preserves packet counts, and
concurrent deletes/inserts retain every intended flow. Apple and ARM64 Linux pass
75 network unit tests, 11 integration tests, affected VFS/ABI tests and strict
checks. Mac product checks, 13 production FFI checks and the iPad static build
passed; physical iPad checks remain skipped per owner.

TCP regression results are approximately flat, as expected for this ownership
stage. Four processes each perform the stated fixed work; results are medians of
three unprofiled counterbalanced runs, with separate profiling samples:

| Longest-child loop median | Previous | Current | Native macOS | Reused native Linux |
| --- | ---: | ---: | ---: | ---: |
| TCP epoll, 2,000 cycles each | 29.24 ms | 29.26 ms | 19.39 ms | 5.54 ms |
| Complete TCP sessions, 256 each | 45.70 ms | 45.08 ms | 15.63 ms | 3.78 ms |

Native Linux references are reused with exact ELF identity verified per case;
macOS equivalents are measured afresh. Their APIs, kernels and CPU allocation
differ. Startup/teardown is excluded from child loops and included in wall time.
Diagnostic counts include nonblocking retries and are not a performance claim.
No application speedup is inferred. Unchanged CPU/memory/graphics/interpreter
scheduling acceptance is reused rather than rerun for documentation or pins.

Next work is complete namespace/virtual-fabric ownership and shared socket OFDs,
then actual factory/control/accept dispatch, alongside Unix/task/VM dependencies.
See [the validation record](../validation/shared-routing-20261102.json) and the
[shared routing contract](../../runtime/docs/architecture/execution-backends.md#shared-routing-filtering-and-connection-tracking-2026-11-02).

## Eleventh migration implemented — shared native network lifetime — 2026-11-02

Runtime `4d69b7e016ed83e5783d4c5b668c31c9084d440a` removes native namespace/DNS Rc ownership and the
aggregate socket counter's dependency on the Linux namespace owner projection.
The native transport namespace, its original quota, resolver and timed interface
cache are shared actual resources. This does not make the complete Linux network
namespace, topology, virtual stack or routing policy Send+Sync.

Native creation now has a transferable prepared transport: one host descriptor,
reservation and creating namespace. Attaching controls reuses that same endpoint.
Aggregate socket creation/accept reserves its existing namespace slot before any
backend effects; failure rolls back and final release requires no Rc access.
The guest socket syscall still uses the existing owner service. This stage adds
no worker opcode and does not claim another reduction in coordinator joins.

DNS endpoints use Arc/Weak, per-endpoint framed input and reply state, and the
original per-namespace resolver. Concurrent admission preserves the original
eight-entry pending-plus-reply bound, including local/malformed replies. Errors
return their reservation, weak pending requests do not retain closed endpoints,
and buffered frames survive backpressure/half-close without duplicate delivery.
No new host DNS workers, service thread, bus or polling loop is added. Ordinary
TCP byte/poll paths take none of these DNS/interface-cache locks.

Performance regression checks use four processes, each with 2,000 checked TCP
epoll one-shot cycles or 256 complete TCP sessions. Timings are medians of three
unprofiled counterbalanced runs; profiling is separate. Results are effectively
flat, as expected for this ownership prerequisite:

| Longest-child loop median | Previous | Current | Native macOS | Reused native Linux |
| --- | ---: | ---: | ---: | ---: |
| TCP epoll, 2,000 cycles each | 28.89 ms | 28.98 ms | 19.50 ms | 5.54 ms |
| Complete TCP sessions, 256 each | 44.63 ms | 44.51 ms | 16.70 ms | 3.78 ms |

Unchanged native Linux references are reused with the exact guest ELF hash
verified per case. Native macOS equivalents are measured afresh; APIs, kernel
and CPU allocation differ. Setup/teardown is excluded from child loops and
included in process wall time. Diagnostic counts include nonblocking retries;
changes in them are not a claimed performance gain. No Firefox speedup is inferred.

Apple/native Linux network, VFS and ABI tests and strict checks passed, including
parallel real socket cancellation, creator lifetime, namespace-quota rollback,
accept queue preservation, concurrent DNS framing/capacity and dead requests.
Mac product tests, 13 production FFI checks and the shared iPad static build
passed. Physical iPad testing remains skipped per owner. Existing unaffected
CPU/memory/graphics/interpreter-scheduling acceptance is reused.

Next dependencies are complete Linux network namespace/policy ownership and
shared socket OFD publication so factory, control and accept can actually run
on workers. Unix rights, task/signal and structural VM/device work also remains.
See [the validation record](../validation/native-network-shared-20261102.json)
and [the resource contract](../../runtime/docs/architecture/execution-backends.md#shared-native-network-lifetime-and-socket-reservations-2026-11-02).

## Tenth migration implemented — 2026-11-02

Runtime `cd78a85d7e2c163b9eec179e260222813879f11e` moves connected native TCP poll/epoll readiness onto
workers. Interests installed before connection completion migrate into the
original live index with their ET/ONESHOT state and delivery cursor preserved.
Worker control and waits no longer join every CPU for these interests.

Waits register duplicates of the persistent native event queue, not the socket.
They cannot delay final FIN or socket quota return. The existing host poller
handles wakeups; no service thread, message bus, synthetic readiness lock or
extra guest-memory barrier is introduced. Socket byte/policy admission remains
unchanged. Factory/full teardown and namespace-dependent connection control,
AF_UNIX/SCM_RIGHTS, task/signal and structural VM/device work remains open.

Four processes each reuse one TCP connection for 2,000 checked byte transfers.
The epoll case MOD-rearms a one-shot interest before each transfer/wait/read;
the poll case waits for readability each time. Connections, queues, startup
and teardown are outside the child loop and inside workload wall time.

| Longest-child loop median | Previous | Current | Native macOS | Native Linux |
| --- | ---: | ---: | ---: | ---: |
| TCP epoll, 2,000 cycles each | 394.8 ms | 29.0 ms | 18.9 ms | 5.54 ms |
| TCP ppoll, 2,000 cycles each | 225.3 ms | 25.0 ms | 19.5 ms | 5.16 ms |

The loop improves 13.61× and 9.01× respectively. Full workload wall time
decreases 428.2 → 62.5 ms and 257.6 → 59.6 ms. Diagnostic cohort rounds fall
from 9,846 to 53 and 5,446 to 49. These include
startup/teardown and are not CPU utilization. Each loop result is the median of
three unprofiled counterbalanced runs; profiling uses separate samples.

Trames and native Linux run the identical static ARM64 ELF. Native macOS uses
the same source with kqueue READ/EV_DISPATCH for epoll and native poll for ppoll;
these are corresponding APIs rather than identical syscall sequences. Native
Linux runs on the existing six-vCPU/8 GiB reference, stopped before Mac timing.
These are event-loop results, not Firefox or whole-product speedup claims.

Apple-host and ARM64 Linux network/shared/VFS/ABI tests, interpreter-kernel and
Mac product checks, strict affected-target checks, production FFI environments
and the iPad static build passed. Physical iPad checks remain skipped per owner.
See [the validation record](../validation/tcp-poll-20261102.json) and the
[shared ownership contract](../../runtime/docs/architecture/execution-backends.md#connected-tcp-pollepoll-ownership-2026-11-02).

## Ninth migration implemented — 2026-11-02

Runtime `0dd8d25685d58f51b477ec48a36c1a99ec3c05f6` replaces frozen admitted epoll interest snapshots with
one shared live index on the original epoll object. Worker ADD/MOD/DEL and waits
share membership, ET/ONESHOT state and the original delivery cursor. Changes to
these interests no longer invalidate global FD snapshots or drain every CPU.
Only owner-only interests retain the original capture boundary. Allowed filtered
calls also run directly with one seccomp decision; denials and unsupported cases
retain their existing owner effects and entry token.

Epoll creation uses a single-caller owner service and is visible to existing
workers immediately. Creation and close/dup ownership themselves remain on the
owner. Worker inotify add-watch now uses the original inode/instance authority
through the sealed pathname resolver. Native watcher topology changes update
already-parked reads/epoll subscriptions through the existing host poller. No new
service thread, polling bus, guest quota, race errno or memory barrier is added.

Four processes each execute 2,000 event-loop or watch-registration cycles. The
epoll fixture reuses one queue and pipe pair per process, executes ADD/MOD,
transmits/checks one byte with a one-shot wait, then DEL. The inotify fixture adds
and removes a watch on its private directory and consumes IN_IGNORED. Setup and
teardown are outside the child loop but included in full workload wall time.
The unchanged 256-session TCP fixture supplies a regression comparison.

| Longest-child loop median | Previous | Current | Native macOS equivalent | Native Linux |
| --- | ---: | ---: | ---: | ---: |
| Pipe epoll control/I/O, 2,000 cycles each | 471.7 ms | 18.4 ms | 5.79 ms | 2.86 ms |
| Directory watch registration, 2,000 cycles each | 239.4 ms | 83.1 ms | 25.1 ms | 3.56 ms |
| Complete TCP sessions, 256 each | 44.7 ms | 44.2 ms | 15.6 ms | 3.78 ms |

The event loop improves **25.69×**; diagnostic cohort rounds decrease from
**15,505 to 47**. Full workload wall time decreases **508.0 to 55.4 ms** (9.17×).
Watch registration improves **2.88×**, with rounds decreasing **4,998 to 45**;
full workload wall time decreases **304.0 to 150.2 ms** (2.02×). TCP loop/wall
results are approximately flat. Diagnostic counts include startup/teardown;
summed caller waits are not CPU utilization.

Timings are medians of three unprofiled counterbalanced runs, with separate
profiling samples. Trames and native Linux execute the exact same static ARM64
ELF. The same C source uses macOS kqueue READ/EV_DISPATCH for the epoll equivalent
and VNODE add/delete/drain for the watch equivalent; those are corresponding
functional operations, not identical Linux syscalls or notification payloads.
The six-vCPU/8 GiB Linux reference was stopped before Mac timings and restored
after targeted Linux CI. Event loops still take 3.17× native macOS and 6.43× the
Linux reference; watch registration takes 3.32× macOS. Kernels and CPU allocation
differ. These fixtures do not establish Firefox or whole-application speedups.

Admitted resources include pipes, eventfd/timerfd/inotify and supported Unix/PTY
poll resources. Native TCP, task-signal and unsupported/nested epoll interests
still need owner authority; socket factory/full close, connect/accept and task
registry work also remain. This is not complete epoll/coordinator removal.

Validation passed: on Mac, 417 VFS, 160 Linux ABI and 14 shared-notification tests;
on native ARM64 Linux, 416 VFS, 160 Linux ABI and 14 shared-notification tests.
Strict affected-crate checks passed on both hosts. Also passed: 252 interpreter
kernel tests including filtered controls, strict kernel/Mac checks, 13 Mac product
tests, 13 production FFI checks (Alpine/Debian/custom/original systemd/two PTYs),
iPad static cross-build and paired-product boundaries. Unchanged CPU, guest-memory
and graphics checks were reused; physical iPad checks remain skipped.

[Samples and validation](../validation/epoll-live-20261102.json),
[fixture](../../tests/fixtures/fd-create-bench.c),
[harness](../../tests/performance/fd-create.py), and
[shared ownership](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#live-epoll-interests-and-worker-inotify-registration-2026-11-02)
record the implementation and remaining boundaries.

## Eighth migration implemented — 2026-11-02

Runtime `d055678a2c897c498d0b9e3cededda268ff1ba2f` shares the original native TCP connection/error state
between owners and workers. SO_ERROR now completes an admitted native connection
on the worker and publishes its byte capability to the original OFD immediately,
including already-running tasks and aliases. Error consumption is atomic across
workers and the owner. There is no mirrored connection state or later repair.

Native TCP keepalive/reuse/buffer/IPv4/IPv6 controls, SO_ACCEPTCONN and shutdown
also run on workers through the original shared host option helpers. Hybrid
option replay and the original Linux ABI preflight remain in place. A connection
completion lock and a keepalive-helper lock synchronize only their actual
multi-step socket invariants; TCP byte I/O gains no lock or guest-memory barrier.
Namespace-dependent completion, filtered controls, virtual/DNS branches and guest
timeout setters retain their previous boundaries.

Three unprofiled counterbalanced repetitions compare the previous runner with the
candidate. A separate diagnostic run supplies counts. Four processes execute
2,000 socket lifecycles with TCP_NODELAY/address and keepalive/error/buffer
controls, 256 complete TCP sessions, or the unchanged 5,000-cycle basic control
fixture. Trames and native Linux run the identical static ARM64 ELF; native macOS
compiles the same source. All references were freshly measured. The Linux
reference has six vCPUs/8 GiB and was stopped before Mac timing.

| Longest-child loop median | Previous | Current | Native macOS | Native Linux |
| --- | ---: | ---: | ---: | ---: |
| TCP state/options, 2,000 cycles each | 259.2 ms | 117.8 ms | 23.1 ms | 5.99 ms |
| Complete TCP sessions, 256 each | 47.2 ms | 44.1 ms | 15.2 ms | 4.22 ms |
| Basic TCP controls, 5,000 cycles each | 237.9 ms | 240.2 ms | 25.6 ms | 9.93 ms |

The state/options fixture improves **2.20×**; owner requests decrease from
**55,992 to 15,999** and worker socket calls rise from 24,000 to 64,000. Its
remaining owner calls are predominantly creation/close, which this stage does
not move. Complete sessions improve **1.07×**; requests decrease from 6,507 to
5,254. Basic controls are approximately flat (**1.0% slower** in this sample).
The state/options workload remains 5.10× native macOS; complete sessions remain
2.90×. Linux uses a different kernel/CPU configuration. These are socket
microbenchmarks, not bulk throughput or measured Firefox responsiveness. Summed
caller waits are not CPU utilization.

Validation: 412 VFS tests with host-directory support, 158 Linux ABI tests,
60 network unit and 11 integration tests, 251 interpreter-kernel tests, 13 Mac
product tests, strict affected-crate/kernel/Mac checks, 13 production FFI checks
(Alpine, Debian, custom rootfs and original systemd/two PTYs), iPad static
cross-build and paired-product boundaries. Later test-only additions were checked
with targeted ABI parity and native-control runs. Unchanged CPU, guest-memory and
graphics results were reused; physical iPad testing remains skipped.

[Samples and validation](../validation/tcp-state-controls-20261102.json),
[fixture](../../tests/fixtures/fd-create-bench.c),
[harness](../../tests/performance/fd-create.py), and
[shared state ownership](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#shared-native-tcp-connection-state-and-controls-2026-11-02)
record this stage. Socket factory/full close ownership and the remaining
connection/accept controls are still next, followed by epoll and task/signal
ownership. The small lifecycle owner loop remains deliberate.

## Seventh migration implemented — 2026-11-02

Runtime `7547fb8f8d3cecef3c04e88d0bfc6e104314d7dc` allows quiescent connected
native TCP to close through the existing single-caller owner service. A close no
longer drains every CPU merely because the socket was connected. The owner still
owns its Rc projection and performs teardown; socket creation/close themselves
have not moved onto workers in this stage.

TCP byte calls now hold one original transport from acceptance through transfer,
nonblocking/deadline handling, readiness registration, same-cohort retries and
completion accounting. They no longer re-resolve a possibly reused numeric FD
between those phases. Quiescence checking and removal share the actual Files
transaction, excluding a newly admitted worker lease. The predicate reads only
existing owner metadata/reference counts; teardown occurs outside Files. Native
controls remain excluded by the existing per-socket gate. There is no new guest
quota, race errno, service thread, polling bus or guest-memory ordering rule.

A second review covered queued work between cohorts: passive transport lifetime
now survives PendingIo/IoCall/Process/job handoffs, preventing a pending read from
being mistaken for an unused socket. This carries no native byte or namespace
policy authority. The next cohort still resolves its current FD and policy;
unsupported admission falls back before effects. Completion, cancellation,
signals and exit release the token. Busy connections, rule-bearing connected
transports, virtual/DNS branches, listeners, connecting/failed sockets and filtered
closes retain their existing boundaries. Original idle close stays supported.

The new fixture measures four processes, each running 256 complete local TCP
sessions: create/nonblocking connect, accept, finalize SO_ERROR, transmit/check a
byte, close the client, verify EOF on the peer, and close the peer. Per-process
listener setup/teardown is outside the child loop but inside full workload time.
The test client uses bounded nonblocking retry/yield loops; production code uses
existing host notifications. The previous 5,000-cycle control fixture is also
remeasured. Each result is a median of three unprofiled counterbalanced runs;
separate profiles supply counts. Both Trames versions and the native ARM64 Linux
reference run the exact same static ELF. macOS compiles the same fixture source.
Native references are fresh, and the six-vCPU/8 GiB Linux reference ran after Mac
timing and was restored to its original stopped state.

| Measurement | Previous | Current | Native macOS | Native Linux |
| --- | ---: | ---: | ---: | ---: |
| TCP sessions, longest-child loop | 94.2 ms | 49.1 ms | 15.4 ms | 3.61 ms |
| TCP sessions, full workload wall | 128.9 ms | 82.6 ms | 19.2 ms | 4.11 ms |
| TCP controls, longest-child loop | 231.7 ms | 237.7 ms | 25.7 ms | 10.0 ms |
| Session cohort rounds, diagnostic | 1,310 | 45 | — | — |
| Session owner service requests, diagnostic | 3,991 | 6,283 | — | — |

Complete sessions improve **1.92×** and diagnostic cohort rounds decrease **96.6%**.
The original control workload is approximately flat (**2.6% slower** in this
three-run sample); it has no connected-close drain to eliminate. Session loops
remain **3.19× native macOS** and **13.6× native Linux**, whose kernel and CPU
configuration differ. Owner requests increase because closes now complete as
single-caller services instead of joining: successful services rise from 2,712
to 6,280. Remaining cohort rounds include startup/teardown, listeners and other
control. Drain time alone excludes repeated cohort preparation; summed caller
waits are not CPU utilization. These are short connection lifecycles, not bulk
TCP throughput or Firefox performance.

Validation passed: 411 VFS, 157 Linux ABI, 69 network and 251 interpreter-kernel
tests; strict shared/kernel/Mac checks; Mac product tests; 13 production FFI
checks including original systemd with two PTYs; iPad static cross-build and
product boundaries. Unchanged CPU, memory and graphics validation was reused.
Coverage includes atomic close refusal without FD mutation, alias/EOF lifetime,
original transport after forced close/reuse, cancellation/timeouts, passive
handoffs and owner continuations without stale native authority. Physical iPad
testing remains skipped. The installed Mac development app contains the measured
binary; existing sessions keep their old executable until restarted.

[Samples, hashes and validation](../validation/tcp-close-20261102.json),
the [session/control fixture](../../tests/fixtures/fd-create-bench.c),
[comparison harness](../../tests/performance/fd-create.py), and shared
[ownership design](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#quiescent-connected-tcp-close-2026-11-02)
record reproduction and remaining scope. Creation, stateful network controls and
broader close ownership remain the next migration work.

## Sixth migration implemented — 2026-11-02

Runtime `d9c9cf9c86f567475a0f1979085c3409d2e14acf` executes unfiltered
native TCP address queries, TCP_NODELAY set/get and immutable SO_TYPE/
SO_DOMAIN/SO_PROTOCOL queries on the current CPU worker. The original host
endpoint is used, with common owner/worker Linux ABI helpers. SO_ERROR, stateful
options, routing/namespace authority and filtered controls retain their original
paths. Ordinary worker byte I/O gains no new control lock or policy checks.

Review of the initial control primitive identified integration work that was
completed before enabling it:

- A per-socket control read/write gate coordinates worker controls with owner
  transitions, including retained io_uring bind/connect. Files membership is
  released before waiting on that gate or touching guest memory; unrelated
  sockets and Files allocation can continue.
- Publication refresh targets the original OFD, preserving other aliases when
  an FD is closed or reused. Weak stored projections do not delay final close;
  accepted operations alone retain the actual endpoint.
- TCP_NODELAY host changes and the real shared hybrid replay record update
  together. Lazy creation of a guest route branch replays the same options.
- Explicit DNS sendto/sendmsg can change the transport without connect. Those
  owner transitions also retire the native control projection. UDP, virtual
  branches and DNS transport remain outside this direct-control path.
- Shared address/option helpers preserve output preflight and error ordering.
  Filtered allow/deny calls still defer and evaluate once on their original
  owner. No seccomp implementation or guest-memory policy was changed.

The complete integrated candidate was tested independently before copying its
verified sources into canonical runtime and product trees. This stage uses safe
Rust, existing resources and object synchronization; it adds no coordinator,
resource thread, message bus, busy loop or global network lock.

The unchanged four-process fixture runs 5,000 TCP control lifecycles per process.
Every cycle creates a socket, sets/gets TCP_NODELAY, queries its local address and
closes it. Loop/wall figures are medians of three unprofiled counterbalanced
repetitions. A separate diagnostic repetition supplies counts. macOS native is
freshly measured; the same-ELF native Linux result is reused from the earlier
2026-11-02 reference run, with unchanged fixture/workload and six vCPUs/8 GiB.

| Measurement | Previous | Current | Native macOS | Native Linux, reused |
| --- | ---: | ---: | ---: | ---: |
| Longest-child loop | 477.8 ms | 235.4 ms | 25.7 ms | 9.54 ms |
| Full workload wall | 514.6 ms | 274.7 ms | 29.9 ms | 10.1 ms |
| Owner service requests, diagnostic | 99,993 | 39,996 | — | — |
| Worker socket controls, diagnostic | 0 | 60,000 | — | — |
| Cohort rounds, diagnostic | 41 | 45 | — | — |

The loop improves **2.03×**; **60,000 controls** run directly and owner requests
fall by approximately **60%**. Startup/teardown and scheduling tails account for
small cohort-count differences; this stage targets remaining per-call owner
transport, not another cohort-drain reduction. Creation and idle close still use
the owner service, leaving roughly 40,000 requests. Current loops remain
**9.16× native macOS** and **24.7× the reused native Linux reference**. These are
control-lifecycle results with no payload traffic; they do not establish TCP
throughput or Firefox performance. Drain/service counters are not CPU percentages.

Validation passed: 410 VFS, 155 Linux ABI, 69 network and 249 interpreter-kernel
tests; strict final shared/kernel/Mac Clippy; Mac product tests; 13 production FFI
checks including systemd with two PTYs; iPad static cross-build and product
boundaries. Passing unaffected results were reused; the direct/filtered dispatch
regressions were rerun after placing the fast path in its specific syscall arm.
Coverage includes gate ordering, independent sockets/Files progress, actual guest
option replay, aliases and reuse, weak/final-close lifetimes, copyout faults, DNS/
virtual exclusions and once-only owner seccomp decisions. Physical iPad testing
remains skipped. The installed Mac development app contains the measured binary;
existing sessions keep their prior executable until restarted.

[Samples, reviewed source hashes and validation](../validation/socket-control-direct-20261102.json),
the shared [control design](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#direct-native-tcp-controls-2026-11-02),
and [native implementation](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-net/src/native/control.rs)
record the reviewed behavior and remaining scope.

## Fifth migration implemented — 2026-11-02

Runtime `36c64a8109a9b53496a3f0cfd662a4ae302f63ce` moves eligible idle TCP
close into the existing single-caller owner service. Admission remains owner-only:
the native socket must be Idle; hybrid sockets additionally have no peer,
listener or guest-stack branch. The original VFS close preserves alias and FD
lifetimes. Connecting, connected, failed, listening, virtual/non-TCP and filtered
closes keep their prior boundary. Actual socket membership gates the worker
candidate, so unrelated owner resources retain their original checkpoints.
No new cross-thread TCP control projection, resource thread or policy route is
introduced in this stage.

Two live-publication correctness issues were fixed before measurement:

- Forks can share a descriptor-content version while owning distinct mutable
  FD tables. `NetworkEpoch` previously reused a capture by that content version,
  which could attach a child's live socket lookup to its parent's table. Captures
  now bind to actual Files membership; its retained allocation prevents identity
  reuse. CLONE_FILES siblings still share their correct capture.
- FD flags now resolve newly created socket OFDs through their existing weak
  membership projection. Existing workers see nonblocking/cloexec updates and
  independent aliases/reused numbers. Only existing atomic bits are exposed;
  endpoint and namespace controls remain on the owner. The ordinary shared-file
  flags branch keeps its original borrowed access without an added Arc increment.

The unchanged four-process, 5,000-cycles-each TCP control fixture measures this
stage against the preceding measured binary. Each cycle creates a TCP socket,
sets/gets TCP_NODELAY, queries its local address and closes it. Loop and wall
values are medians of three unprofiled counterbalanced runs; a separate profile
supplies counts. macOS native was freshly measured. The native Linux reference is
reused from the preceding stage: the exact fixture ELF SHA and native workload
are unchanged, and the reference VM was not started again.

| Measurement | Previous | Current | Native macOS | Native Linux, reused |
| --- | ---: | ---: | ---: | ---: |
| Longest-child loop | 1,021.2 ms | 466.0 ms | 42.9 ms | 9.54 ms |
| Full workload wall | 1,058.7 ms | 504.3 ms | 48.3 ms | 10.1 ms |
| Cohort rounds, diagnostic sample | 17,597 | 49 | — | — |

The loop improves **2.19×** and diagnostic cohort rounds decrease **99.7%**.
Remaining rounds include process startup/teardown and unrelated control. This
workload still issues **99,978 owner service requests**, so eliminating drains
has not eliminated per-call central dispatch. Current loops remain **10.9× native
macOS** and **48.9× the reused native Linux reference**. Actual direct socket
controls and broader close ownership remain the next performance target. These
are control-lifecycle results, not connected TCP throughput or Firefox scores;
service/drain counters are not CPU percentages.

Affected checks passed: 408 VFS tests, 152 Linux ABI tests, 67 network tests,
248 interpreter-kernel tests, Mac product checks/strict Clippy, 13 production FFI
environment checks and the iPad static cross-build. Passing unaffected results
were reused. New coverage checks independent fork tables with equal versions,
existing captures seeing new socket flags, alias status versus descriptor flags,
FD reuse, idle-versus-active close admission, and two CPUs creating/closing
sockets without per-call cohort checkpoints. Physical iPad testing remains
skipped. Mac's installed development app contains the measured binary; existing
sessions keep their old executable until restarted.

[Raw samples, hashes and validation](../validation/socket-idle-close-20261102.json)
and the shared [Files/close design](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#idle-tcp-close-and-live-files-identity-2026-11-02)
record reproduction and the remaining boundary.

## Fourth migration implemented — 2026-11-02

Runtime `860005dff5f914a28fa88c4f1bce561a1b089d17` moves unfiltered Internet
socket creation, bind/listen, address/option control, shutdown and nonblocking
connect/accept into the existing single-caller owner service. Peer CPUs continue;
the caller releases its execution slot. The original namespace, hybrid routing,
firewall and connection control remain on their owner. This removes cohort
drains for these calls; it does not remove their owner hop or create a new service
thread. Mac and iPad use the same implementation.

Connected TCP authority is now published on the actual shared OFD. Existing
worker captures consult live Files membership and can immediately use newly
connected/accepted transports without recapturing the cohort. Accepted lookup
retains the real transport; stored publications remain weak and do not delay FIN.
Only socket owner slots expose this transport projection, preserving unrelated
exclusive-construction contracts and separate metadata admission. Routing and
firewall authority changes retain their original joined boundary.

Socket/accept reserve numbers through the same atomic Files allocator as worker
resource creation. Accept preflights guest output and publishes its FD only after
address copyout succeeds. Failure releases the reservation and accepted socket.
The original Linux ABI accepts the caller's live memory View. Admission is before
effects; EAGAIN/EINPROGRESS are final results, never a reason to replay the call.
Blocking connect/accept, socket close/replacement, AF_UNIX/SCM_RIGHTS, netlink and
seccomp-filtered socket controls keep their prior paths. Seccomp decision and
denied-entry handling are unchanged.

Four processes each run 5,000 `socket → TCP_NODELAY set/get → getsockname → close`
cycles. Values are longest-child loop medians from three unprofiled, counterbalanced
repetitions; a separate diagnostic repetition supplies cohort counts. This is a
control-lifecycle benchmark with no payload traffic. Both Trames versions and the
native Linux reference run the identical static ELF; macOS compiles the same
fixture source against its native socket API.

| Measurement | Previous | Current | Native macOS | Native Linux |
| --- | ---: | ---: | ---: | ---: |
| Longest-child loop | 2,062.8 ms | 986.2 ms | 38.6 ms | 9.54 ms |
| Full workload wall | 2,099.9 ms | 1,023.9 ms | 43.0 ms | 10.1 ms |
| Cohort rounds, diagnostic sample | 69,373 | 17,585 | — | — |

The loop improves **2.09×** and whole-cohort rounds decrease **74.7%**. The current
loop still costs approximately **25.6× macOS** and **103× native Linux**. Socket
close still joins, and short controls still incur owner dispatch/resource work;
73,808 requests used that service in the diagnostic sample. The next network
stage should share the actual socket control/close ownership rather than treat
this intermediate hop as finished. These numbers do not establish connected TCP
throughput or Firefox performance. Diagnostic counts include startup/teardown
and serial tails; they are not CPU percentages. The six-vCPU Linux reference ran
after Mac timing and was restored to its original stopped state.

Affected validation passed: 406 VFS, 151 Linux ABI, 67 network, 247 interpreter
kernel and 13 Mac tests, strict shared/kernel/Mac Clippy, 13 production FFI
checks including systemd with two PTYs, product boundaries and the iPad static
cross-build. Passing unaffected results were reused; two new networking tests
were corrected for asynchronous accept and closure field capture, and one new
kernel helper for writable code/nonzero output capacity, then rerun successfully.
Regression coverage includes live new-connection visibility, accepted transport
lifetime across close/reuse, accept copyout rollback alongside worker allocation,
blocking-before-effects admission, final nonblocking errors, and two CPUs making
progress through repeated socket controls without per-call cohort joins.
Physical iPad testing remains skipped at the owner's request. The installed Mac
development app contains the measured binary; existing sessions keep their
current executable until restarted.

[Samples, hashes and validation](../validation/socket-control-20261102.json),
the [fixture](../../tests/fixtures/fd-create-bench.c),
[comparison harness](../../tests/performance/fd-create.py), and shared
[ownership design](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#live-tcp-publication-and-short-socket-services-2026-11-02)
record the result and remaining boundaries.

## Third migration implemented — 2026-11-02

Runtime `eef3631c6f1dccdcf863d3765379be93cdcc6e4b` moves `pipe2`, `eventfd2`,
`timerfd_create` and `inotify_init1` creation/publication onto existing CPU workers.
The common `DescriptorFactory` contains only actual Files membership, the shared
OFD ID allocator and captured filesystem UID/GID. Mac and iPad use the same
constructors, resources, FD limits and byte/control/wait mechanisms. No VFS/Rc control crosses
this boundary and no new coordinator/resource thread is introduced.

Pipe creation reserves two numbers in one Files transaction, copies output
without holding the table lock, then publishes both complete OFDs together.
Failed output or unwinding releases both slots and endpoints. The owner uses the
same ABI/rollback transport. Existing worker views immediately accept all four
new resource types; blocked I/O retains the original object across close/reuse.
Timer notifications/deadlines, pipe EOF and inotify delivery reuse existing
signals. Seccomp evaluates each entry once, preserving normal denied-entry effects.
Non-I/O calls also skip the byte dispatcher's unused FD lookup for argument zero.

Four processes each run 10,000 complete lifecycles. Each pipe cycle sends/reads a
byte and checks EOF; eventfd checks its counter; timerfd sets/gets a disarmed timer;
inotify checks nonblocking empty read. Timings below are longest-child loop
medians from three unprofiled repetitions; separate diagnostic samples supply
cohort counts. The identical static ELF runs in both Trames versions and the
Linux-kernel reference. The macOS fixture measures the pipe lifecycle only.

| Resource | Previous loop | Current loop | Gain | Native Linux loop | Native macOS loop | Cohort rounds, previous → current |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Pipe | 976.1 ms | 97.3 ms | 10.0× | 20.3 ms | 25.7 ms | 25,336 → 48 |
| eventfd | 885.3 ms | 58.5 ms | 15.1× | 14.4 ms | — | 25,012 → 43 |
| timerfd | 889.1 ms | 59.6 ms | 14.9× | 14.5 ms | — | 25,298 → 44 |
| inotify | 838.5 ms | 46.7 ms | 18.0× | 15.0 ms | — | 24,776 → 38 |

Full-workload wall medians, including startup/fork/exec/output/teardown, improve
from **1013.2 → 134.9 ms**, **926.0 → 95.0 ms**, **923.9 → 94.5 ms** and
**874.2 → 84.3 ms** respectively. These are lifecycle microbenchmarks, not general
application or Firefox scores. Current loops remain approximately **3.1–4.8×**
slower than the native Linux reference. Its existing ARM64 Lima/VZ environment
uses six vCPUs and four benchmark processes; it ran after the Mac measurements
and was stopped afterward. Native inotify here measures empty initialization/
read/close, not watched-directory throughput. Diagnostic counts include startup,
teardown and serial tails; drain time alone is not a CPU percentage.

Validation passed: 404 VFS tests, 150 Linux tests, 245 interpreter-kernel tests,
Mac product checks and strict Clippy, 13 production FFI environment checks
(including original systemd with two PTYs), product boundaries and the iPad
device static-library cross-build. Tests cover concurrent unique live FD/OFD
allocation, real pipe ownership/flags, copyout rollback, existing-view visibility,
waits after close/reuse, live timer deadlines and allowed/denied seccomp paths.
Physical iPad testing remains skipped at the owner's request. Mac's installed
development app contains the measured binary.

[Samples, hashes, native references and validation](../validation/descriptor-create-20261102.json),
the [fixed-work fixture](../../tests/fixtures/fd-create-bench.c) and
[comparison harness](../../tests/performance/fd-create.py) record reproduction.
The shared [execution design](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#worker-anonymous-resource-creation-2026-11-02)
documents ownership and publication. Epoll/watch state, sockets/connect/accept/
SCM_RIGHTS, inotify add-watch control, signalfd/task masks and signal-producing
pipe operations retain their existing owner boundaries, alongside structural VM
and native-device registration work.

## Second migration implemented — 2026-11-02

Runtime `721e50aa7b377d03abfc7696120dda299c71f16d` moves ordinary regular-file
`openat`/`openat2`, including creation, onto execution workers. A free FD is
reserved before resource creation and the complete shared OFD is published to
the real Files membership. Existing workers can immediately perform eligible
reads/in-place writes without an owner RPC or a table recapture. Prepared authority lives
on the OFD and is bound to its original execution epoch; no global publication
cache is consulted on each I/O. Close/reuse retains the accepted original OFD.

The shared umask, record-lock registry and OFD ID allocator preserve the original
Linux ownership. Concurrent exclusive creation has one winner; ordinary creators
reopen that inode. A reservation rolls back on failure. Cross-worker file-size
limits remain caller-specific. New-file growth and other existing I/O deferrals
can still use a single-caller owner service while peer CPUs continue.

The change also fixes recursive inode read locking during file preparation and
removes unused inode write locks from ordinary open/notification delivery. A
production systemd check exposed a refresh of an unreached lazy page; refresh
now materializes it through the original fallible source/private-COW contract.
Shared physical identity, private isolation and backing errors have a regression.

Four processes perform **40,000 open → pread → fstat → close cycles**. Values
are medians of three unprofiled repetitions; counters use separate samples.
The same static ELF runs in both Trames versions and the Linux reference.

| Case | Longest child loop, before → after | Whole workload, before → after | Cohort rounds, before → after |
| --- | ---: | ---: | ---: |
| Private store | 1673.0 → 240.7 ms | 1851.3 → 419.4 ms | 33,404 → 45 |
| Shared host directory | 1976.6 → 437.8 ms | 2052.8 → 522.2 ms | 28,983 → 44 |

Loop gains are **7.0× private** and **4.5× shared**; whole-workload gains
are approximately **4.4×** and **3.9×**. Native macOS/APFS completes the same C
work in **142.3 ms** for the loop and **148.2 ms** overall. The separate native
Linux-kernel reference completes it in **13.6 ms** and **23.3 ms**: the existing
ARM64 Lima/VZ reference has six vCPUs, uses ext4 and runs four benchmark processes.
It was started after the Mac measurements and stopped again afterward. The
filesystem/kernel paths differ, so both references remain explicit. A substantial
Linux-native gap remains; these are not Firefox or general application scores.

All affected shared tests, 244 interpreter kernel regressions, Mac checks and
13 production FFI environment checks passed, including original systemd with
two PTYs. The device static library cross-build passed; physical iPad testing
remains skipped at the owner's request. Mac's installed development app has the
measured binary. [Raw samples, hashes and validation](../validation/worker-open-20261102.json)
and the [comparison harness](../../tests/performance/path-metadata.py) are retained.
The harness also runs a Linux/macOS native-only reference.

At this second checkpoint, `O_TRUNC` opens, directory cursors and mutations, lower-overlay
files, special endpoints, pipe/socket/accept/event/timer creation, signal/task
state, structural VM changes and native-device registration still had owner
boundaries. The shared [execution design](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#worker-regular-file-opens-and-publication-2026-11-02)
records the publication and ownership rules. No coordinator/resource thread or
polling bus was added.

## First migration implemented — 2026-11-02

Runtime `0a3a99ca676f0008a524658529e8c5dde0bc0b95` moves ordinary
`newfstatat`, `fstat`, `statx`, `faccessat`, `faccessat2` and `readlinkat` queries
onto the existing execution workers. Mac and iPad pin the same implementation.
The original resolver, credentials and live inode/host metadata are reused; the
namespace tree and stat results are not copied or cached. `statx` resolves metadata
and mount identity once. Directory/O_PATH metadata leases do not migrate directory
cursors or retain last-close ownership. Seccomp decisions survive a deferred call
without being evaluated twice. No extra execution/resource thread is added.

The ownership proof and remaining epoch boundary for root/cwd/mount/credential
changes are documented in the shared runtime's
[execution design](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/execution-backends.md#worker-pathname-metadata-2026-11-02).
Dynamic proc/sys/dev endpoints and unsupported control descriptors still defer.
At this first checkpoint, open/create, directory mutations, full FD publication,
signal/task state and structural VM ownership remained subsequent stages.

Final fixed-work comparison on Apple M6 (12 CPUs, 24 GiB), using four processes
and 40,000 total pathname stat calls. Timings are medians of three unprofiled
runs; counters come from separate diagnostic samples.

| Case | Longest child query loop, before → after | Whole workload, before → after | Cohort rounds, before → after |
| --- | ---: | ---: | ---: |
| Private store | 608.7 → 121.6 ms | 777.3 → 295.2 ms | 27,225 → 53 |
| Shared host directory | 751.0 → 176.1 ms | 832.4 → 261.3 ms | 27,843 → 58 |

Native macOS performs the same query-loop work in **10.7 ms**,
with **17.2 ms** for the full workload. Guest query-loop
speedups are approximately 5.0× private and 4.3× shared; whole-workload gains are
approximately 2.6× and 3.2×. A substantial native gap remains. These values are
not Firefox page/video or end-to-end application measurements.

The remaining ~500 services are largely file initialization writes/syncs outside
the query loops. Cohort drain alone excludes dispatch, mapping rebuild and actual
resource work. A different scheduling interleaving changes these counters; no
deterministic race outcome or exact count is imposed.

[Raw results and validation](../validation/path-queries-20261102.json) and
[reproducible comparison](../../tests/performance/path-metadata.py) record source
identities, fixture/binary hashes, all repetitions and remaining limitations.
Affected shared-module and interpreter regressions, production FFI environments,
Mac product checks and the iPad device cross-build passed locally. Physical iPad
testing was skipped at the owner's request.

---


## Baseline assessment — before the first migration

The shared kernel still has an owner event loop. It is no longer a mandatory
per-call hop for all CPU workers. Ordinary shared-resource I/O, futex waits and
wakes, retained task scheduling, and supported native operations already execute
on the existing workers. Remaining ownership is substantial, especially pathname
metadata and resource creation. This audit does not claim complete removal.

The audited shared runtime is `a45bbe3b07a2faa22a3e3a9332bd397809005e57`.
Mac uses the hardware executor; iPad uses the interpreter with the same process,
VFS, signal and memory semantics. No execution-path code changed for this audit.
The Mac `--metrics` output now exports existing execution-path counters as JSON;
normal execution acquires no new profiling counters, checks or barriers.

### Boundaries that must be counted separately

- **Worker completion:** guest CPU, syscall or native operation runs on the same
  existing execution thread, with shared resource ownership. A blocked task gives
  up its execution slot; notifications requeue the retained task.
- **Owner service:** only the caller is parked. Its CPU and memory View survive,
  and peer workers continue. The owner still processes requests, so this is a
  remaining central hop even when it causes no cohort stop.
- **VM checkpoint:** the affected address-space/backing domains quiesce, commit
  and rebuild their Views. Unrelated domains retain their jobs. A new eligible
  private anonymous interval can instead be published through an owner service.
- **Whole-cohort join:** workers return their Views and process state is updated
  by the owner. A control check alone is not a join. The persistent epoch checks
  host control at bounded intervals without automatically recreating the cohort.
- **Serial execution:** a lone eligible runnable task may execute on the owner
  without any worker RPC. Zero worker counters in this mode do not establish a
  parallel bottleneck.

### Source audit and remaining work

| Scenario | Current authority and remaining dependency | Can centralized execution be removed? |
| --- | --- | --- |
| Basic CPU execution, libc native functions, identity/clock queries | Worker snapshots / native registry; Mac monotonic vDSO calls avoid a syscall altogether. | Already off the owner for admitted parallel calls. iPad retains its own clock capability profile. |
| Futex WAIT/WAKE/REQUEUE, sleep, ordinary retained I/O waits | Shared wait queues and execution queue. Legacy owner-only futex targets still request a scheduling join. | Normal worker handoff is already direct. Move remaining owner-only task state into the same queue before removing its fallback. |
| Pipe/PTY byte I/O, TCP/UDP and Unix socket byte operations, unmasked poll/epoll, supported io_uring | Retained OFDs, shared endpoint state and host readiness notifications. Creating endpoints, accept/connect policy changes, SCM_RIGHTS imports and unsupported resource kinds still use owner authority. | Steady-state I/O is largely direct; resource creation and publication can migrate with shared Files/network authority. |
| Open regular file reads/writes, seek and fd metadata | Captured `FileEpoch`/`DirectFiles` handles cover eligible files. Uncaptured files, bulk disk service and sync requests may use owner services; mapped backing and SIGXFSZ cases retain structural/signal boundaries. | Yes, by sharing complete inode/backing and pending-I/O authority. Do not substitute one global VFS mutex or another resource-service thread. |
| Pathname metadata and namespace changes | `newfstatat` (79), path lookup/open, readlink and namespace mutations still enter owner Linux/VFS paths. The benchmark below identifies repeated path metadata as a major remaining join source. | **First priority:** shared pathname/inode query authority; then open/create/directory changes. Directory/inode invariants require object-level synchronization. |
| FD membership | dup/dup3/close, selected fcntl and CLOEXEC operations have worker implementations. New descriptor kinds and side effects outside the retained authority can fall back. | Extend shared Files/OFD registration rather than rebuild snapshots for every new FD. Preserve accepted OFDs across close/reuse. |
| brk/mmap/munmap/mprotect/mremap/mincore/madvise | Additive anonymous mmap can use a service; structural changes use scoped VM checkpoints. Filtered entries preserve the exact seccomp decision on the mm owner. | The executing owner can migrate into a per-mm transaction. Unmap/protection/replacement still need affected users and backend translations to synchronize. Hardware-backed guest addresses do not eliminate these lifetime rules. |
| clone/fork/exec/exit/wait, process groups, credentials, signal delivery and rlimits | Owner-held process registry, run-state transitions and kernel-wide relationships. Signal action queries have a worker path; delivery and mutable task authority still join. | Incrementally: shared task registry, targeted run-queue notifications and per-task/group state transitions. Fork/exec must coordinate the participating mm/Files/task domains; no Linux semantic requirement to stop unrelated processes. |
| Masked ppoll/epoll, temporary signal masks, seccomp installation/TSYNC, tracing | Unsupported/owner-retained cases keep kernel entry/restart/mask authority. `a45bbe3` prevents dormant masked epoll from repeatedly stopping unrelated workers. | Share per-task mask/restart state, filter generations and targeted wakeups before migrating. Dropping the mask/filter semantics is not an optimization. |
| Vulkan/video/audio and Wayland/native resources | Supported calls and GPU waits already use workers and notifications. Audio OPEN, video CREATE_SURFACE and non-worker Vulkan/resource operations retain owner paths. Native service pumping and FD/rights publication remain owner-side. AppKit presentation has a separate platform UI-thread requirement. | Migrate device/resource registration using shared authority. Keep platform UI dispatch isolated in the Mac/iPad adapter; it need not suspend guest CPUs. GPU context/queue synchronization remains local to the real resource. |
| Host control, PTY resize/hangup/signals, systemd/session lifecycle and memory pressure | `cohort_control` and the product adapter poll/dispatch actual changes. Real signals, timer expiry, foreground group changes or stop requests can request a join. | Keep a small session control loop; publish targeted task/domain events so unrelated workers continue. Eliminating this loop's existence is less useful than eliminating its data-path responsibility. |

Source anchors in the shared runtime:
[worker admission](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-session/src/worker_syscalls.rs),
[worker execution](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-session/src/workers.rs),
[process owner and dispatch](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-session/src/process.rs),
[VM/control boundaries](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-session/src/process_vm.rs),
[remaining services](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-session/src/process_service.rs),
[shared direct I/O](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-linux/src/direct.rs),
[file captures](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/crates/trames-vfs/src/file_io.rs).

### Current measurements

Real Apple Silicon HVF execution, 12 configured CPU slots, disposable storage and
loopback-only HTTP. Counts include program startup/teardown. These are single
instrumented diagnostic samples, not throughput or cross-platform score claims.
The iPad scheduler/source assessment shares this kernel; the numeric samples
below are Mac measurements. GUI/Vulkan workloads were source-audited, not freshly
profiled in this matrix.

| Workload | Total syscalls | Worker syscalls | Owner service requests | Cohort rounds | Summed peer-drain time |
| --- | ---: | ---: | ---: | ---: | ---: |
| 200,000 libc monotonic clock queries | 4 | 0 (serial/vDSO) | 0 | 0 | 0 ms |
| 12-thread futex ring, 2,000 rounds | 48,149 | 48,036 | 0 | 66 | 0.491 ms |
| Four-thread paging/atomics + fork COW | 89 | 16 | 0 | 21 | 2.184 ms |
| Four processes, 40,000 private pread calls | 40,689 | 40,011 | 520 | 50 | 24.884 ms |
| Four processes, 40,000 shared pread calls | 40,694 | 39,892 | 515 | 59 | 1.485 ms |
| Four processes, 40,000 private path-stat calls | 40,688 | 11 | 509 | 28,097 | 87.229 ms |
| Four processes, 40,000 shared path-stat calls | 40,688 | 11 | 514 | 27,923 | 39.985 ms |
| Four concurrent 1 MiB HTTP downloads | 411 | 81 | 7 | 117 | 2.121 ms |

The file fixtures initialize four 8 MiB files before their counted loops. Many
~512 owner services are initialization writes/syncs; the read-loop counters show
40,000 and 39,881 direct file calls respectively. Do not classify all of these
service requests as costs of the read loop. Private/shared wall times also have
different cold import/cache histories and are deliberately not compared here.

The shared metadata sample returned syscall 79 at joins 31,159 times. This
histogram counts pending returns, not unique first-stop causes; remaining calls
can execute in serial tails. It cannot be summed into a unique pause count.
The futex sample's return histogram contains mask changes (135), exits (93),
clones (220) and unmap (215), rather than the 48,000 steady-state wait/wake calls.

Peer-drain time excludes dispatch, mapping rebuild, actual syscalls and caller
service waits. Service wait is summed over callers and can overlap. Neither is
an end-to-end CPU percentage. Raw counters and commands are recorded in the
[validation record](../validation/coordinator-audit-20261102.json).

The already-validated masked epoll fix is relevant to systemd: multi-terminal
idle host CPU fell from about 107% to 2.2% in the production FFI check, and from
about 150% to 6% on the physical iPad HUD. These are reused earlier checks of the
same runtime source, not rerun measurements; see
[iPad environment validation](../validation/ipad-environments-20261102.json).

### Migration order — updated after the ninth stage

1. Path metadata, ordinary regular-file open/create and anonymous resource
   creation are implemented above. Namespace/inode topology changes and special
   backing/endpoint lifetimes still retain their real owner boundaries.
2. Native TCP bytes, addresses, common options, shutdown and admitted SO_ERROR
   completion run on workers. Idle/quiescent connected close and epoll creation
   use single-caller services. Admitted epoll ADD/MOD/DEL and inotify add/remove
   are live shared operations. Next are socket factory/full close ownership,
   remaining bind/connect/listen/accept controls, AF_UNIX/SCM_RIGHTS and
   native-TCP/task-signal/nested or unsupported epoll interests. Preserve original
   policy/routing and actual OFD lifetimes while removing the remaining hops.
3. Temporary signal-mask waits, signal delivery and task creation/exit using a
   shared task registry and targeted execution-queue events. Preserve seccomp
   decisions, EINTR/restart, process-group semantics and OFD lifetimes.
4. Complete per-mm structural ownership and native-device registration. Reduce
   synchronization to the actual mm/backing/device domains. Keep required HVF
   translation invalidation and iPad interpreter memory lifetime semantics in
   their existing backend boundary.

A final owner loop for session lifecycle is compatible with this design. The
objective is that normal CPU, filesystem, network and device operations run on
existing execution threads, while real shared-state changes synchronize their
own objects. This audit adds no new coordinator, reactor thread or polling bus.

---

## Historical migration record (through 2026-10-24)

The dated record below predates later worker FD operations, scoped VM checkpoints
and the current product builds. Use the audit above for present ownership; its
old release notes and migration batch counts are historical.

### Earlier progress notes

2026-10-24 的性能优先级审计见 [整组停顿测量](../validation/cohort-cost.md)：
先让实际 Files/OFD 结构调用在 worker 完成，再缩小 VM 修改的暂停域；
低频查询迁移不代替这两项。仅放宽周期不是生产优化。

截至 2026-10-24，已完成 39 批资源/任务路径迁移，另修复了单活跃进程的 cohort
误准入回退。这个批次数字不代表完成比例：剩余映射和生命周期变更需要更深的
所有权改造，也尚无代表整个产品的调用/CPU 周期占比，不能声称已完成某个百分比。
第 17 批之后统一并复用了 io_uring 等待订阅，未另计迁移批次；第 18 批拆出共享 OFD
可变状态，F_SETFL/FIONBIO 可以在 worker 执行，FD 表结构仍由原所有者维护。
第 19 批进一步共享描述符 CLOEXEC，接入 F_SETFD/纯 CLOEXEC close_range，并处理
fork/CLONE_FILES/unshare/exec 的元数据所有权。第 20 批拆出按 inode 共享的 POSIX
文件锁，F_GETLK/F_SETLK 及关闭/退出释放共用对象，完整迁出路线见 [共享内核状态](shared-kernel-state.md)。

## 发布状态

1.3.0 build 36 包含前 10 批。此后共享 futex/requeue、PTY、单活跃进程 MMU 修复及
第 13 批 TCP 就绪字节 I/O、第 14 批 TCP 宿主事件等待、第 15 批短 io_uring/GETEVENTS、第 16 批固定资源/链接/排空、第 17 批提供缓冲/multishot/计时/取消/CQ poll、第 18 批共享 OFD 状态控制、第 19 批描述符元数据及第 20 批共享文件锁在当前开发代码中，
以及第 21 批共享 inotify/被监视文件 I/O、第 22 批共享 FS 上下文/umask、第 23 批共享信号表/只读查询、第 24 批共享 VM 分配上下文/brk 查询、第 25 批共享 flock/OFD 锁寿命、第 26 批阻塞 flock 通知等待、第 27 批共享关闭通知生命周期、第 28 批共享 inotify 实例/注册索引、第 29 批共享原生 socket 端点/等待租约、第 30 批共享 PTY 从端寿命、第 31 批共享可写映射引用、第 32 批共享 Unix 路径绑定身份、第 33 批共享 FIFO 身份/元数据分离、第 34 批共享映射长度/变更状态及脏页元数据去重、第 35 批共享 OOM 状态与 BPF 程序寿命、第 36 批共享系统日志与非阻塞 syslog、第 37 批日志阻塞通知与 kmsg 直接 I/O、第 38 批共享 UTS/名字调用、第 39 批共享 PID namespace 身份与映射已做各自受影响验证，尚未进入新 IPA。
Linux 的结构/性能数据不能代替 Apple ARM64 与 iPad 验收。

## 当前路径

| 范围 | 已实现 | 仍需协调角色的部分 |
| --- | --- | --- |
| CPU / musl 原生函数 | 当前 worker 执行，共用 guest 内存模型 | 需要 backing checkpoint 的受限 NativeCall |
| 常用查询与 FD/OFD 标志 | 身份、时钟、getcpu、F_GETFD/F_GETFL、F_SETFD/F_SETFL/FIONBIO、纯 CLOEXEC close_range | FD 表结构仍冻结；动态 proc/sysfs 完整进程状态、可变权限 |
| 系统日志 | syslog 全部操作共用 ABI/epoch 权限；阻塞流和 kmsg I/O、seek、poll/epoll 由共享资源通知恢复；游标按实际打开对象共享 | FD 安装/关闭/dup、epoll 注册及任务/凭据生命周期提交 |
| UTS namespace | 共享实际身份/名字，原子配额；uname、sethostname/setdomainname 在 worker 复用 ABI | unshare/setns、凭据及 namespace FD 安装，其他 namespace 对象图 |
| 共享运行队列 | 原任务/View 保留；等待让出执行槽；直接通知恢复；无生产回复通道等待 | cohort 准备、统一停止、映射提交；少数 legacy 唤醒 |
| pipe/eventfd/timerfd/Unix stream | 标量/向量、字节消息、适用的 ppoll/epoll 与 deadline | FD 安装/表项变更、SCM_RIGHTS、信号、临时 signal mask |
| inotify | 共享实例寿命/注册索引及队列、read/readv/FIONREAD、poll/epoll、rm_watch；普通文件 worker 直接产生 ACCESS/MODIFY；共享关闭租约最后释放可发 IN_CLOSE，链接状态与监视对象已共享 | 创建/路径注册、实际 FD/OFD 关闭和 inode/目录结构变更 |
| FS 上下文 | CLONE_FS 共用目录/mount 引用对象，无逐任务传播；umask 共享原子在 worker 交换 | 路径/inode/mount 仍由 owner 操作；setns/pivot_root 关系检查仍需协调 |
| 信号处理函数表 | CLONE_SIGHAND 共用对象；fork/exec 按域复制/私有化，无逐任务数组传播；查询型 rt_sigaction 在 worker | 处理函数修改的 pending 清理、作业控制、信号生成/投递、mask 和任务退出 |
| VM 分配上下文 | CLONE_VM 直接共用 brk/mmap 游标；删除快照比较和逐任务传播；brk(0) 在 worker 原子读取 | 游标修改仍需 VM 映射提交；映射表、权限及 backing 生命周期仍冻结在 cohort 内 |
| futex | private/shared wait/wake、位集、requeue/cmp_requeue | 未保留 View 的外部 waiter、PI/受限操作 |
| PTY | master/slave 字节、向量、只读属性/尺寸、poll/epoll；实际从端租约最后释放可在 worker 更新计数、通知 HUP/EIO | 主端关闭/devpts、信号输入、作业控制、其余可变 ioctl、宿主 UI pump |
| 普通文件 | 合格文件的 lseek/fstat、读取与受限覆盖写、F_GETLK/F_SETLK、flock/解锁及基于原始 OFD 的阻塞通知等待；大块读取异步交还执行槽 | 文件增长/持久化/路径及权限元数据、映射冲突等服务；阻塞记录锁、FD/OFD 结构寿命及死锁检测 |
| Vulkan | worker 内资源/命令/提交/传输；context 与编译等待直接交接 | 设备 teardown、部分 FD 认证/native relay；同 context 对象图仍独占 |
| TCP | 已连接、无规则的原生 TCP read/write/readv/writev；阻塞交还执行槽，宿主 epoll/kqueue 事件直接恢复，保留 socket deadline；实际宿主端点/配额与边沿观察器共享，等待持有同一寿命租约 | 消息/控制/连接调用、网络 poll/epoll、过滤与虚拟栈；单任务串行入口仍用原网络等待 |
| UDP / DNS / 网络控制 | 原功能继续工作 | 当前仍在原网络所有者路径 |
| io_uring | 30-opcode 兼容实现；17 个 opcode 的字节、固定/提供缓冲、重复完成、链接/排空、计时/取消及 GETEVENTS/EXT_ARG 可留在 worker；五类 register 查询/卸载；CQ ppoll/epoll 直接等待 | setup、新 pin/文件注册、FD/路径/网络控制、受限文件服务及未保留执行上下文的跨任务后台请求；见 [设计](io-uring.md) |
| FD、映射、进程与命名空间变更 | OFD 状态、每描述符 CLOEXEC 共享原子字段；fork/共享/脱离元数据域；原功能继续工作 | FD 创建/安装/关闭/dup、映射及生命周期多数仍需整组 worker 返回 |

## 是否已经完全撤掉

没有。`workers::Event::Service` / `Request::for_epoch` 仍用于把受限文件等服务交给
所有者；这条生产路径保留原 Job/View，通过运行队列恢复，已经没有逐调用 reply
channel，但仍有消息转交及所有者调度成本。未准入 syscall / NativeCall、FD/映射/
进程变更仍可让整个 cohort 返回。不能把“没有新服务线程”或测试的 service_requests=0
理解为整个协调角色已经消失。

目标是普通工作直接运行、必要同步限定到实际共享对象；同一套资源语义由串行入口
和 worker 复用。单 CPU 的直接 MMU 路径仍有价值，调度/退出/映射提交的同步职责也
必须存在，但它们无需长期集中在一个所有者。删除原入口前需要满足以下条件：

| 剩余依赖 | 原因 | 要继续替换的所有权边界 |
| --- | --- | --- |
| FD 安装/关闭、固定文件表、ACCEPT/SOCKET/CLOSE | VFS/FD 表和 namespace 权限仍由原所有者维护 | 共享 FD 表 + Arc OFD，按表/对象更新；保留代际与最后关闭语义 |
| OPENAT/STATX、文件增长/同步/持久化 | 路径、权限、backing 与配额生命周期未完全共享 | 拆分路径与文件对象，不把完整 VFS 套进全局锁；复用原文件算法 |
| CONNECT、网络控制、native TCP POLL_ADD | 网络 namespace/规则和边沿状态仍有所有者数据 | 共享 socket 控制/边沿事件状态，沿用已有宿主 reactor；不能反复订阅 level-ready socket 模拟边沿 |
| 无 enter actor 的跨任务后台请求 | CQ 观察者只有自己的 View，不能借自己的同址 VA 执行他人请求 | 为原提交 VM 保留可调度执行上下文与取消/退出归属 |
| mmap/exec/exit、namespace 改动 | 映射与权限快照需一致提交 | 按相关 VM/FD 表/线程组停止和提交，让无关任务继续运行 |

io_uring 的 owner/worker 引擎还没有合并；计时、缓冲与 probe 已共享，这次将 enter/CQ
poll 的资源等待也抽到 `direct/uring/wait.rs`。继续新增重复的控制/文件实现会增加维护
成本，应先建立上述共享所有权，再统一执行逻辑，而非以更多 worker opcode 作为终点。

第 18 批已将 OFD 的不可变访问位与共享可变状态分开：F_SETFL 使用一次 relaxed store，
FIONBIO 使用单字段原子位操作；pipe/TCP 的阻塞判断和文件 append 准入读取同一状态。
owner 与 worker 共用设置算法，无全局 revision 失效、资源锁或额外普通访存屏障。
已有阻塞字节调用在下一次重试时也读取新状态；设置标志本身不制造资源唤醒，poll
等待不受 O_NONBLOCK 影响。FD 表/CLOEXEC、OFD 本体及 namespace 权限尚未迁移。
结果和固定工作量对照见 [共享 OFD 状态](../validation/worker-fd-status.md)。
上述为第 18 批边界；第 19 批已迁出 CLOEXEC 元数据，结构 map 与 OFD 本体仍待后续。
元数据域与结构缓存身份独立，fork 不重复准备相同 OFD 文件/字节能力；共享同一 Files
域的任务直接观察标志，成功 exec/UNSHARE 脱离，失败 exec 保留共享关系。
证据见 [描述符元数据](../validation/worker-fd-table.md)。

## 后续顺序

1. 优先共享 FD/OFD 与文件对象，按对象处理可独立的安装、关闭及元数据变化；保留
   generation/权限/生命周期边界，避免把整棵 VFS 放到一个全局锁下。
2. 在已有宿主事件接入基础上扩展 UDP、消息 ABI、网络 poll/epoll 与连接控制，
   并让串行入口复用同一资源/等待实现；为后台请求保留原 VM 执行上下文。
   不新建资源服务线程或周期轮询。
3. 将映射和进程变更的暂停范围缩小到相关 VM/FD 表/线程组，使不相关 guest 工作
   继续执行。这是撤掉全体协调依赖的主要剩余架构工作。

单活跃 CPU 的计算继续使用串行 Memory 直接 MMU，不因无关 idle waiter 强迫进入
parallel View；真正相关的等待与多活跃任务保持交接/并行。无 JIT、无额外普通访存
屏障；原算法的线程安全和 ARM 内存模型责任不变。

详细边界与历史批次见 [epoch services](epoch-services.md)。最近的证据：
[共享 futex](../validation/worker-futex.md)、[PTY](../validation/worker-pty.md)、
[CoreMark 回退修复](../validation/coremark-build35-36.md)、[TCP](../validation/worker-tcp.md)、
[TCP 事件等待](../validation/worker-tcp-waits.md)、[io_uring worker](../validation/io-uring-worker.md)、[固定资源/链接](../validation/io-uring-worker-registered.md)、[重复完成/计时/CQ 等待](../validation/io-uring-worker-async.md)。
等待订阅复用、别名去重和剩余依赖审计见 [等待路径整理](../validation/io-uring-wait-reuse.md)。

第 20 批已把 POSIX 范围集合移到 inode 的共享对象，冲突检测/提交原子完成；owner 与
worker 共用算法/ABI，不再修改 Filesystem 中的 owner 锁表。关闭释放使用同一对象，
进程退出从弱引用注册表逐 inode 清理。F_SETLKW 等待/死锁检测及 FD 表本身仍待迁移。
576 次定向控制操作的 cohort 从 577 降到 1；见 [文件锁验证](../validation/worker-record-locks.md)。

第 20 批之后继续拆分了实际 pipe 端点寿命与 owner inode 投影：Arc 端点最后引用可在
worker 释放并直接产生 EOF/HUP/EPIPE，原生剪贴板 relay 保留独立元数据 anchor。
本次没有新增 worker syscall，不另计批次；FD 表、完整 OFD 和 io_uring owner 保留
引用还需继续迁移，见 [共享管道端点](../validation/shared-pipe-endpoint.md)。

随后 CLONE_FILES 改为真正共用一个 owner 表，删除结构修改后的逐任务 map 传播、
每次 syscall 的 FD 版本跟踪和 Process.files_id。退出/exec 依据表引用释放，lookup
保留稳定 OFD，dup3 先替换再关闭旧目标；共享 clone 不再分配临时私有表。
327 项受影响 Rust 检查及 1/2/4 CPU 的 FD/锁/FIFO/io_uring 验证通过。
close/dup 仍需 join，OFD 和 worker 动态查表尚未迁完；不另计 worker syscall 批次。
见 [共享 Files 表](../validation/shared-files-table.md)。

第 21 批把 inotify 队列与 watch 消费移出 owner，普通文件 worker 可直接发通知，
read/poll 等待和 rm_watch 不再转交。实例锁之外唤醒，不增加服务线程；路径注册仍冻结。
64 轮通知交接在双核/四核均完成 64 次 worker 文件调用、32 次 poll 和 1 次撤销，
服务请求降为 0。实际文件表/OFD 生命周期仍待迁移，见 [inotify 验证](../validation/worker-inotify.md)。

第 22 批删除每次 syscall 的 FS 快照比较、ResourceChanges.fs、Process.fs_id 和目录
上下文传播；umask 以对象内原子交换直接执行。普通 exec 按 Linux 行为保留 CLONE_FS，
同时保持文件表私有化。256 次交换的双核/四核并行批次约 258 → 3–4，详见
[共享 FS 上下文](../validation/shared-fs-context.md)。在该批次结束时 heap/actions 及路径/生命周期边界仍待迁移。

第 23 批进一步删除 actions_id、处理函数数组传播与通用 ResourceChanges，CLONE_SIGHAND
直接共用信号表；fork/exec/reset 保持各自共享域。查询型 rt_sigaction 在 worker 执行，
512 次查询的双核/四核批次约 513 → 3–4。heap 传播、pending/作业控制与其他 task
生命周期仍需继续迁移，见 [共享信号表](../validation/shared-signal-actions.md)。

第 24 批按实际 VM 身份共享分配上下文，删除 heap 快照/比较及最后的逐任务传播。
brk(0) 在 worker 用共享原子读取，512 次查询的双核/四核批次约 514 → 3–4；改变映射
的调用仍保留 VM 提交边界。原 Files/FS/actions/heap 传播至此全部移除，整组停止、
映射与生命周期协调还在，见 [共享 VM 分配上下文](../validation/shared-vm-allocation.md)。

第 25 批删除文件系统级 flock 表/失效 OFD 扫描，改为 inode 对象和 OFD 共享锁租约，
最后租约可在 worker 直接释放/通知。合格文件的非阻塞操作与解锁进入 worker；
640 次控制的双核/四核并行批次 643 → 3–4，单核未见改善。该批尚未涵盖完整 OFD/FD 结构和阻塞
flock（阻塞等待后续见第 26 批），见 [flock 验证](../validation/shared-flock.md)。

第 26 批将阻塞 flock 接入共享运行队列，冲突让出执行槽，inode 解锁直接唤醒；
停止/重入保存原始请求，join 在处理任何 close/dup 前保留完整 OFD，避免 FD 重用
导致锁错文件。信号中断/重启、停止、退出按原有任务语义清理。普通无竞争的单 CPU
调用不额外构造等待请求。原始 OFD 结构/最终析构仍受 joined 生命周期限制，
没有据此宣称 close/dup 已迁出。256 次控制的多核协调批次约 132 → 3–4，
详见 [阻塞 flock 验证](../validation/blocking-flock.md)。

第 27 批将文件关闭通知从 owner inode/路径 Rc 中拆出，OFD 持有 Arc<FileClose>，
监视目标按 inode 共享，目录链接的 parent/name/linked 按对象同步。最后租约可在
worker 直接产生 IN_CLOSE；实时注册、rename、EXCL_UNLINK 和关闭后删除事件顺序
都复用同一组通知实现。普通字节能力不延长租约，未监视/未使用的 inode 不分配通知对象。
实际 close/dup 和部分端点仍未迁出，此批是生命周期依赖拆分，不增加 worker opcode
或声称整体加速，见 [共享关闭通知验证](../validation/shared-close-notifications.md)。

第 28 批把 inotify 实例由 Rc/RefCell 变为 Arc 对象，注册索引仅持有共享通知目标的
弱引用，不再依赖 Node。worker rm_watch 共用完整撤销逻辑，立即清理索引与标记；
普通字节快照不保留实际实例，最后实例租约可在 worker 清掉监视。一次性 watch 的
存在检查与更新合成同一队列事务，避免复活已发 IGNORED 的旧编号。路径解析、安装
FD 和完整 OFD 仍未迁出，见 [共享 inotify 实例验证](../validation/shared-inotify-instance.md)。

第 29 批把原生宿主端点、配额租约和持久边沿观察器从 socket 的 Rc 控制投影拆出。
创建/accept 在宿主调用前原子预留，失败自动回收；最后实际端点引用可在 worker
关闭宿主 FD 并回收配额。普通 DirectTcp 仍为 Weak；普通阻塞 I/O 和 io_uring
的宿主等待注册持有真实端点租约，取消时先注销/关 duplicate FD，再在注册锁外
释放资源，无额外堆包装。namespace/规则/连接控制、完整 OFD、动态 FD 表及其
join 仍待迁移，不据此新增 worker opcode 或声称整体提速。
见 [共享原生端点验证](../validation/shared-native-endpoint.md)。

第 30 批把 PTY 从端实际打开引用拆为 Arc<PtySlaveEndpoint>；dup/fork 和进行中的
引用保持同一打开计数，独立 open 才增加。最后 worker 租约释放直接原子递减并
通知主端，无 inode/mount/terminal 锁，缓冲输出仍可读完后返回 EIO。字节能力
不保留从端引用。主端删除 devpts 节点及作业控制仍依赖 owner，需要和目录/任务
域继续迁移，本批不移除这些 join；见 [从端验证](../validation/shared-pty-slave.md)。

第 31 批删除 mapped inode 的逐页 writer 扫描和 writers_cached 失效，实际
FileMapping 持有共享计数租约，按 inode 与具体 mount 分别计数。最后引用可在
worker 释放，owner 查询立即看到；只读 remount、普通 umount 与懒卸载树回收
都检查同一引用状态。普通映射先完成回写尝试再释放租约，字节/MMU 路径不变。
这只迁移可写引用及忙状态；FilePage、Node、映射表、回写提交与主端 devpts
生命周期仍要继续拆分，见 [映射引用验证](../validation/shared-mapping-writers.md)。

第 32 批把 inode 的 Unix socket 引用换成共享弱身份，消除到 LocalSocket Rc
控制图的直接依赖。关闭在资源通知前立即使身份失效；插入失败/关闭清理原所有者
的弱解析索引。索引按 guest 共享，保留跨网络 namespace 的 pathname 访问与
abstract 名称隔离；硬链接、overlay、磁盘恢复和 Wayland/X11 清理使用相同语义。
这仍是 inode 共享化的前置拆分，完整 FD、Unix listener/rights 和连接控制未迁出；
字节 I/O 热路径不变，见 [绑定身份验证](../validation/shared-unix-binding.md)。

第 33 批删除 Rc<RefCell<Pipe>>，FIFO inode 直接持有稳定编号与共享资源；匿名
pipe 元数据移到实际打开描述符和原生 relay 的 owner 投影。资源不反向引用 Node，
不保活已删除 inode；splice 以共享资源身份判断别名。少一次管道包装分配，原
字节/握手/关闭行为不变；同时补齐既有 FIFO 硬链接误报 EPERM，含恢复后的共享
inode 与通道检查。完整 FD/OFD 与路径控制未迁出，见 [FIFO 验证](../validation/shared-fifo-identity.md)。

第 33 批之后试验了 Arc FilePage + 布局 RwLock，功能验证通过，但单核映射读写
微基准耗时增加约 26%–27%，已撤回，未计入迁移批次。下一步先解决串行映射的
稳定页能力及失效，再共享 inode；不能把协调成本转为逐次访存锁。见
[未采用的映射页原型](../validation/shared-file-page-experiment.md)。

第 34 批先拆开映射检查与 inode：MappingState 按文件共享稳定身份/原子变更世代，
普通文件和 memfd 的 live EOF 直接读取实际存储能力，无长度快照和 inode 借用。
overlay 下层及会整体刷新内容的伪节点继续沿原 owner 检查。修改时间在页面首次
变脏时更新，后续 CPU/native 写入保留逐次 page/backing 失效；成功回写/截断重置
脏页状态。这移除了热循环的 inode 元数据和时钟操作，未加布局锁、CPU 普通访存
屏障或新线程。固定单核 mmap 微基准耗时约降 19%–20%，不代表整体/iPad 速度。
完整 FilePage、首次脏写元数据、Node/Lower、FD/OFD、VM/task 仍待迁出；没有新增
worker syscall 或删除映射提交 join，见 [映射状态验证](../validation/shared-mapped-state.md)。

第 35 批拆掉 OFD 中 OOM/BPF 两条 owner Rc 依赖。OOM 的任务弱身份改为共享
弱引用，分数与权限下限打包成单个 32-bit 原子；非特权写入在 CAS 提交时重新
检查下限，fork 取同一原子快照，线程共享偏好但保持独立任务寿命。BPF 已验证
指令、FD 和 cgroup 附加使用同一 Arc 程序，弱注册表不保活，最后 worker 引用
释放后可立即回收配额。程序执行没有新增锁；完整进程/proc 权限与 cgroup/FD
安装仍待迁移，不增加 worker syscall，见 [验证](../validation/shared-ofd-leaves.md)。

第 36 批把会话日志从 Rc/RefCell 拆为共享资源，非阻塞 syslog 在 worker 共用
原 Linux ABI，日志事务只锁自身，写 guest 内存时已释放。权限按 epoch 捕获，
seccomp 与输出故障保留原语义；action 2 阻塞读取及 kmsg FD 游标仍走原路径。
768 次双线程查询的内存后端双核/四核并行批次 406 → 10，未增加协调服务请求。
这不是完整 FD/等待迁移或整体加速比，见 [系统日志验证](../validation/shared-kernel-log.md)。

第 37 批把 syslog 权限能力并入 DirectSyscalls，legacy stream 复用 PendingIo 和
共享运行队列，append 解锁后直接唤醒；kmsg 游标按实际打开对象共享，字节 I/O、
seek、poll/epoll 共用同一资源。补齐 kmsg 的 epoll 准入、ET 世代和最后关闭失效，
无写入者的日志等待不改变单活跃 CPU 路径。单宿主槽验证无重放/服务请求；共同
支持的三类等待双核路由批次 55 → 17，不代表整体速度。完整 FD 生命周期仍待
迁移，见 [日志等待验证](../validation/shared-log-waits.md)。

第 38 批把 UTS 对象及弱快照改为 Arc/Weak，名字按对象同步、配额原子预留与
回收，最后引用可在 worker 释放。uname、sethostname/setdomainname 共用 ABI
和 epoch 权限直接执行；unshare/setns 与其他 namespace 仍保留任务边界。
1024 次双线程名字操作的双核/四核并行批次分别 978 → 11、935 → 8，不代表
产品整体速度，见 [UTS 验证](../validation/shared-uts.md)。

第 39 批将 PID namespace 身份、弱注册与嵌套映射迁为实际共享对象；默认空间
转换不取得映射/注册表锁，祖先分配按根到叶完整提交，死亡/上限失败不消耗编号。
任务生命周期及裁剪仍在原提交边界；本批没有新增 worker opcode 或整体提速承诺，
见 [PID namespace 验证](../validation/shared-pid-namespace.md)。


## Shared Mac/iPad follow-up (2026-10-30)

Native display connections no longer force every CPU to return on a control
check. The existing owner pumps Wayland/XWM bytes and clipboard resource relays
while workers retain their Views. Active and already-idle native cohorts use
the existing resource wakeups and bounded host-control wait.

Sandboxed mapping calls also enter the common mm-local checkpoint path. The
owner evaluates the original seccomp entry once; ALLOW executes only after its
related address-space Views quiesce. Other actions retain the full joined
boundary and carry their exact-entry result to the ordinary handler. An entry
or immutable filter-chain change invalidates the cached result. This keeps
Firefox's sandbox enabled and preserves the same iPad kernel implementation.

See [the memory contract](../../runtime/docs/architecture/region-memory.md#filtered-memory-calls)
for admission, cached-decision identity and negative-control coverage. Mac's
`--metrics` reports pending call occurrences at whole-cohort joins; these are
not unique first-stop causes or wall-time percentages. The Mac navigation
validation records the intermediate display-only result as well as the final
combined result, including failed/error observations and sampling exclusions.

## Twenty-third migration implemented — worker addressed Unix datagrams — 2026-11-03

Runtime `8c129587f5f5ea42e140962160f38dfecce0d787` moves unpaired AF_UNIX
read/write, readv/writev, sendto/recvfrom and sendmsg/recvmsg to workers in the common
Mac/iPad runtime. Source address, sender credentials and portable shared OFDs now
travel with the actual packet. A byte-only datagram no longer creates an owner
ancillary-ledger entry. Unix-bearing owning graphs keep their original owner ledger,
cycle checks and reclamation; no Rc graph is declared Send.

An accepted send keeps its original full source OFD, copied input and one resolved
target. The target is a weak endpoint plus the original queue/readiness, so a blocked
call cannot keep the receiver open or follow an unlinked/rebound pathname. Sender
capacity waits subscribe to the actual destination queue and include its resource
in scheduling hints. Original output records survive waits and owner handoffs;
copyout ranges are checked before dequeue without rereading changed guest headers.
Owner waits now also subscribe then retry before parking. Callbacks and final OFD
drops run outside queue/name/peer guards. Native default-connected interfaces use
that same queue. No global socket/VFS lock, service thread, bus, polling loop,
ordinary-memory fence or extra native permission policy is added.

The single-slot test caught an old admission assumption: all nonstream Unix sockets
were restricted to controls/messages, so the new byte implementation still fell
back to the owner. Addressed byte/packet entry now admits the retained call, including
the existing filtered path. Sixteen filtered/unfiltered, one/two-slot cases cover
all four byte/message syscall pairs. Each case performs one send, two receives and
two final waits: five calls, five filtered decisions when enabled, zero services,
and slot release under queue-capacity pressure.

| Four processes × 1,000 checked iterations, longest-child median | Previous | Current | Native macOS | Native ARM Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| Connected datagram write/read | 211.85 ms | 44.10 ms | 0.97 ms | 0.86 ms | 4.80x |
| Addressed sendto/recvfrom + source check | 212.84 ms | 61.86 ms | 3.93 ms | 0.91 ms | 3.44x |
| Portable pipe rights / alias EOF / close | 197.33 ms | 46.02 ms | 1.74 ms | 1.61 ms | 4.29x |
| Owner-only device open / rights / read / close | 523.41 ms | 524.02 ms | 12.38 ms | 2.51 ms | No significant change claimed |
| Existing socketpair lifecycle | 21.91 ms | 21.65 ms | 5.06 ms | 4.64 ms | No significant change claimed |

The first three whole-process wall medians are **330.67 → 160.79 ms**,
**323.66 → 168.97 ms** and **322.70 → 136.81 ms**. Separate diagnostic cohort
counts fall **5,405 → 60**, **5,091 → 69** and **4,892 → 60**. Service requests
remain three in those whole-process diagnostic runs, including startup/teardown;
previous byte operations mostly forced cohort joins rather than counted services.
The owner-device case remains **15,331 → 15,102** cohorts and is explicitly not
claimed as migrated. Existing socketpair counts are **43 → 42**, with no meaningful
regression or improvement claimed.

Endpoint setup and cleanup are outside datagram loop timing; each one-byte packet
is checked, and addressed receive verifies its exact source pathname. Portable
rights use a precreated pipe and check the received alias with EOF/read/close. The
separate device case includes `/dev/null` open/rights/alias operations and records
the still-owned boundary. An initial device-rights measurement showed no material
gain; it was retained as that separate case, not represented as portable rights.

Three unprofiled counterbalanced samples run after all compilation, FFI acceptance
and local Linux CI, with the Linux instance stopped during Mac timing. Native Linux
uses the identical static ARM64 musl ELF on the existing six-vCPU environment;
Trames uses four vCPUs and native macOS uses native APIs. Kernel/API/compiler and
CPU-count differences make native values contextual. These microbenchmarks do not
establish Firefox/whole-app/iPad performance or generally native-speed execution.

Apple/Linux VFS **447/446**, ABI **186 each**, kernel **271 each**, strict checks,
Mac **13**, both product execution boundaries, all **13** production FFI environment
checks and the iPad static build pass. Tests include 64 packets from eight concurrent
senders, exact source/credential association, portable rights without ledger entries,
late passcred, timestamps, final target close/rebind, sender FD reuse, frozen vectors
and original owner-only rights fallback. Unchanged checks/guest artifacts are reused;
physical iPad testing remains skipped per owner. The measured runner is installed
in Trames Dev; the stable app is unchanged.

The coordinator-removal goal remains active. Next boundaries include batch
sendmmsg/recvmmsg, Unix-bearing owning graphs, device opens/rights, native-service
adapters, task/job control, io_uring, mount/proc ownership and structural VM/task
transitions. See the [validation record](../validation/worker-unix-datagram-20261103.json)
and [shared contract](../../runtime/docs/architecture/execution-backends.md#worker-addressed-unix-datagrams-2026-11-03).

## Twenty-fourth migration implemented — retained Unix batches and paired packet bytes — 2026-11-03

Runtime `a7a92f114c4f30a2756fce7646083b26c4c91e47` runs AF_UNIX sendmmsg on
workers for stream, datagram and seqpacket endpoints. It also admits paired packet
read/write, readv/writev and sendto/recvfrom through the common retained packet ABI.
The original immediate paired sendmsg/recvmsg and stream byte paths are preserved.

One accepted batch keeps the original full source OFD and Files/credential context,
completed count, active message and its resolved weak target. Later headers are
imported lazily with the original ABI: a later fault cannot suppress earlier sends.
Each addressed item resolves its own target. The currently waiting portable item
keeps its input/rights/target, while the next unaccepted item can observe a new
binding. Original-owner handoff resumes at the exact item and count without
replaying prior messages, including after source FD reuse. A negative control
supplies another factory on resume and proves the accepted Files domain survives.

First-item capacity waits subscribe/retry and release their execution slot. EAGAIN
after progress returns the completed count; short stream sends end the batch.
The compat-flag check, empty batch, 1,024 cap and post-send msg_len write order remain
unchanged. A msg_len EFAULT cannot undo delivery. Partial EPIPE retains the positive
count and queues task-level SIGPIPE unless MSG_NOSIGNAL is set. The worker records
that terminal result and hands task effects to the original owner; no packet is
resent to rediscover the error, and the filter decision is not repeated.

The progress mutex belongs to that accepted call, not to a socket, Files or resource
service. It does not serialize unrelated guest calls. Single-message records stay
inline in their existing Arc allocation rather than gaining another heap allocation
for a batch feature. No global resource lock, new service thread, bus, polling loop,
ordinary-memory fence or native-only permission policy is introduced. A still-
unaccepted owner-only rights item retains its existing owner decoder/ledger boundary.

| Four processes × 1,000 checked iterations, longest-child median | Previous | Current | Native macOS* | Native ARM Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| Eight-packet libc sendmmsg wrapper / checked receive | 721.82 ms | 36.71 ms | 6.67 ms | 4.96 ms | 19.66x |
| Paired datagram scalar write/read | 157.00 ms | 6.04 ms | 0.42 ms | 0.80 ms | 25.99x |
| Paired datagram vector writev/readv | 158.59 ms | 6.50 ms | 0.53 ms | 0.81 ms | 24.39x |
| Existing addressed connected datagram write/read | 44.76 ms | 46.11 ms | 1.05 ms | 0.80 ms | No gain claimed |
| Existing socketpair lifecycle | 21.47 ms | 22.11 ms | 4.99 ms | 4.91 ms | No gain claimed |

The first three whole-process wall medians are **759.04 → 71.68 ms**,
**192.92 → 40.61 ms** and **192.01 → 40.66 ms**. Separate diagnostic cohort
counts fall **23,172 → 50**, **5,596 → 50** and **5,626 → 56**. Whole-process
service requests remain three, including startup/teardown; previous packet calls
mostly forced joined execution rather than counted resource services. Regression
controls have about three percent higher loop medians, slightly lower process wall
medians and similarly small cohort counts (**54 → 51**, **48 → 49**); no gain or
statistical regression bound is claimed for those small differences.

Measurement correction, discovered during stage26: disassembly of the exact
recorded stage24 ELF (`defc37834ea0daf74675bb143962af8aafdf2063287d8cc5bd44665c02b99455`)
shows libc sendmmsg looping over sendmsg, without a sendmmsg syscall. The row above
measures that eight-message workload, not SYS_sendmmsg throughput. Actual batch
syscall functional tests remain valid. Stage26 below uses an explicit kernel call.

Batch iterations send eight checked one-byte datagrams and verify each receive;
paired scalar/vector iterations check two bytes in one packet. Setup/cleanup is
outside loop timing. *The macOS batch row uses eight native POSIX sendmsg calls,
not a claimed native sendmmsg implementation.* Native Linux uses the exact static
musl ELF used by Trames, on the existing six-vCPU ARM64 environment. All compilation,
affected Linux CI and FFI acceptance finish before the three unprofiled
counterbalanced samples; Linux is stopped during Mac timing. Different APIs,
kernels, compilers and CPU counts make these contextual references. No whole-app,
Firefox, physical-iPad or generally native-speed claim follows.

Apple/Linux ABI **191 each**, kernel **273 each**, strict checks, Mac **13**, both
product boundaries, all **13** FFI environment checks and the iPad static build
pass. Unchanged VFS **447/446**, CPU/memory/graphics acceptance and guest artifacts
are reused. Physical iPad testing remains skipped per owner. Packet capacity tests
cover **48** filtered/unfiltered one/two-slot cases with five calls, one decision
per filtered entry and zero services. Batch capacity tests cover **12** cases with
six calls and zero services. Real guest partial-SIGPIPE/NOSIGNAL controls retain
three exact entry decisions. Further tests cover caps, empty/short/partial sends,
post-send faults, target rebinding, descriptor reuse and owner-only rights handoff.
The measured runner is installed in Trames Dev; the stable app is unchanged.

The coordinator-removal goal remains active. Native TCP/UDP batches, Unix-bearing
owning graphs, device opens/rights, native-service adapters, task/job control,
io_uring, mount/proc ownership and structural VM/task changes remain. recvmmsg is
currently unimplemented rather than an implemented coordinator path. See the
[validation record](../validation/worker-unix-batch-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#retained-unix-batches-and-paired-packet-bytes-2026-11-03).

## Twenty-fifth migration implemented — shared stateless memory devices — 2026-11-03

Runtime `e4a9f217ac17821cef0097d465466d3c340c7fa4` moves null, zero, full,
random and urandom descriptions into actual shared Files/OFD ownership. Admission
uses immutable inode behavior, not a pathname shortcut. Renamed/custom mknod nodes
and opened mount identities retain their original policy and lifetime. Owner/native
and workers use the same device algorithms and access/mount/timestamp/notification
handling. Raw and positioned error order and existing synchronous results remain.
Stateless operations gain no storage or RLIMIT_FSIZE charge.

The fixed synthetic devices use the original shared pseudo-inode cache even on
concurrent first opens. The cache guard protects only registration, identity and
pruning; hits release it before content refresh, and fixed behavior needs no rewrite.
Hot reads/writes never acquire it or resolve a name. The PathQueries Rc/control
island stays private: only the actual Arc cache, whose fields are Send+Sync by
composition, crosses the boundary. Proc/sysfs, magic device links, terminals, PTMX,
render/bridge/block/kernel-log controls retain their owner boundaries. The common
walker still applies original credentials, device policy, nodev and mount checks.
No new service thread, bus, polling, global resource lock or ordinary-memory fence
is introduced. Actual device OFDs can now travel as portable Unix rights without
the owner ledger; last close and notification occur on the real last reference.

| Four processes × 1,000 checked iterations, longest-child median | Previous | Current | Native macOS | Native ARM Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| Null complete device lifecycle | 684.28 ms | 37.95 ms | 30.81 ms | 2.81 ms | 18.03x |
| Zero complete device lifecycle | 687.53 ms | 36.75 ms | 23.34 ms | 2.98 ms | 18.71x |
| Full complete device lifecycle + ENOSPC | 687.11 ms | 37.31 ms | No device | 2.83 ms | 18.41x |
| Urandom complete device lifecycle | 691.91 ms | 39.28 ms | Different write permission | 3.96 ms | 17.61x |
| Null OFD open / Unix rights / alias read / close | 520.24 ms | 85.00 ms | 12.45 ms | 2.48 ms | 6.12x |
| Existing socketpair lifecycle | 21.69 ms | 21.91 ms | 4.82 ms | 4.71 ms | No gain claimed |
| Ordinary positioned-file write/read control | 52.15 ms | 45.17 ms | 4.52 ms | 0.66 ms | No gain claimed |

Whole-process wall medians for null/zero/full/urandom are **720.44 → 71.89 ms**,
**724.08 → 71.30 ms**, **722.03 → 72.25 ms**, **729.02 → 74.49 ms**;
null-rights are **636.60 → 180.29 ms**. Separate diagnostic cohort counts fall
**20,714 → 50**, **20,861 → 48**, **20,956 → 51**, **20,816 → 50**, and
**14,684 → 57**. Device lifecycle service requests fall from roughly 6,300–6,500
to three, including process startup/teardown; rights requests remain three because
previous rights mostly forced joined execution rather than counted services.
Socketpair loops are **21.69 → 21.91 ms**, wall **55.70 → 55.58 ms**, cohorts
**48 → 50**. The additional ordinary positioned-file control is **52.15 → 45.17 ms**,
wall **134.65 → 133.34 ms**, with **50 → 50** cohorts. No statistically established
gain/regression bound is claimed for those controls; they check for cross-path cost.

Device iterations include open/read/write/pread/pwrite/readv/writev/seek/fstat/close,
checking counts, EOF/zeros, character type and ENOSPC. File setup/cleanup is outside
its timer. All final compilation, local Linux CI and FFI acceptance precede the
three unprofiled counterbalanced samples. Native Linux uses the exact static ARM64
musl ELF on the existing six-vCPU environment and is stopped during Mac timing;
Trames uses four vCPUs. macOS null/zero seek returns the requested end offset 27,
while Linux returns zero, explicitly checked per platform. This Mac has no full
device and unprivileged urandom writes return EPERM, so no throughput substitute is
invented for those rows. Original Trames RNG writes remain acknowledged discard;
native Linux executes its own entropy-device behavior. Different kernels, APIs,
filesystems, compiler targets and CPU counts make native results contextual.
These loops do not establish whole-app, Firefox, iPad or universal native speed.

Apple/Linux VFS **452/451**, ABI **192 each**, kernel **274 each**, strict checks,
Mac **13**, both product boundaries, all **13** FFI environment checks and the iPad
static build pass. The final added cfg(test)-only ABI matrix reruns only affected
Apple/Linux ABI/strict checks; unchanged production passes/artifacts are reused.
Physical iPad testing remains skipped per owner. Five device tests cover cold
identity, renamed nodes, live/retained mounts, actual notifications and numeric
reuse. The ABI matrix checks scalar/vector/positioned buffers, offsets and concrete
errors. Four real guest null/zero cases on one/two slots each complete **122 calls
entirely on workers**, with **122 filter decisions and zero services**, including
open, byte/positioned I/O, seek/stat, flags/alias/close and Unix rights.

An initial compiler ENOSPC was resolved by clearing only regenerable incremental
caches; final debug checks disable incremental caching and release settings stay
unchanged. Source, rootfs and measured artifacts remain intact. The measured runner
is installed in Trames Dev, and the stable app is unchanged.

The coordinator-removal goal remains active. Dynamic device/native-service controls,
Unix owning graphs, native IP batches, task/job control, io_uring, mount/proc ownership
and structural VM/task transitions remain. Further FD routing consolidation should
remove repeated membership lookup; the ordinary-file control records the current
cost rather than asserting all paths are free. See the
[validation record](../validation/worker-simple-device-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#shared-stateless-memory-devices-2026-11-03).


## Twenty-sixth migration implemented — retained IP messages and batches — 2026-11-03

Runtime `6f8955705daa8149aefc24a0e616c29d976c19dc` moves IPv4/IPv6 sendto, recvfrom,
sendmsg, recvmsg and sendmmsg onto the calling worker using the real shared socket
OFD. The common prepared message decoder also serves the owner. Original native,
hybrid and packet-stack socket algorithms, policy, accounting and locks remain;
netlink namespace/control data still defers before guest import or packet effects.
The existing connected TCP byte path is unchanged.

Send bytes, destination and packet info are imported once per accepted item.
Receive scatter/address/ancillary destinations retain their original identity.
Blocking EAGAIN subscribes through the existing socket event sources and stack
deadlines, releases the execution slot, then retries. Only parked receive outputs
are rechecked before dequeue. FD close/reuse and later guest header edits cannot
redirect accepted calls. Original live descriptor flags, MSG_DONTWAIT and socket
timeouts still apply. No new service thread, global resource lock, polling bus,
ordinary guest-memory fence or native permission layer is introduced.

One accepted batch keeps completed count, its current prepared item and terminal
result; later messages are imported lazily. The original 1,024 cap, empty-batch FD
validation, compat rejection, requested-length checks, short stream termination,
positive partial errors and post-send msg_len copyout order remain. A delivery
followed by EFAULT is terminal and never replayed. The progress mutex belongs to
one syscall only. Each item uses the existing socket publication guard separately,
without a batch-wide socket algorithm lock. SIGPIPE uses a relaxed per-call flag
and saved result; only task effects go back to the original owner, with no resend
and no second seccomp entry. MSG_NOSIGNAL completes entirely on the worker.

| Four processes × 1,000 iterations, longest-child median | Previous | Current | Native macOS* | Native ARM Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| UDP sendmsg/recvmsg | 179.36 ms | 15.32 ms | 19.96 ms | 1.39 ms | 11.71x |
| TCP sendmsg/recvmsg | 190.31 ms | 15.84 ms | 10.16 ms | 2.42 ms | 12.02x |
| UDP kernel batch + eight recvmsg | 888.57 ms | 87.73 ms | 30.71 ms | 6.38 ms | 10.13x |
| TCP kernel batch + eight frame receives | 890.05 ms | 80.15 ms | 23.33 ms | 9.54 ms | 11.11x |
| Existing TCP poll control (short sample) | 13.79 ms | 16.76 ms | 9.82 ms | 3.01 ms | See longer control below |

Whole-process wall medians for single UDP/TCP are **211.71 → 46.15 ms** and
**223.14 → 49.51 ms**; batch UDP/TCP are **925.63 → 120.00 ms** and
**932.32 → 113.01 ms**. Separate diagnostic cohort counts fall **5,485 → 49**,
**5,289 → 48**, **25,040 → 47** and **24,837 → 50**. Service requests remain
three, including startup/teardown: old calls mostly joined rather than counted
as resource services. This distinction prevents mistaking a flat service counter
for the absence of coordination overhead.

The short TCP-poll control increases **13.79 → 16.76 ms**, with overlapping
sample ranges. A longer confirmation uses **10,000 iterations and nine unprofiled
counterbalanced samples**: loops **125.451 → 125.480 ms**, wall **158.194 →
157.938 ms**, cohorts **48 → 48**. No control gain or statistical regression bound
is claimed. Both the initial short result and longer confirmation are retained.

Single-message iterations check 64 bytes across two iovecs. Batch iterations
explicitly invoke SYS_sendmmsg for eight 64-byte messages and verify each recvmsg;
TCP handles any short receive while assembling each checked frame. Disassembly
verifies syscall number269 entering x8 and SVC0. Endpoint setup/cleanup is outside
the loop timer. *Native macOS uses eight POSIX sendmsg calls as its contextual
batch reference.* Native Linux runs the same static musl ELF on six vCPUs; Trames
uses four. All compilation, affected CI and FFI acceptance finish before final
measurements, and Linux is stopped during Mac timing. Different kernels/APIs,
CPU counts and short-sample variability make native numbers contextual. These are
not whole-app, Firefox, physical-iPad or universally native-speed results.

Apple/Linux ABI **197 each**, kernel **276 each**, strict checks, Mac **13**, all
**13** FFI environment checks, product boundaries and the iPad static build pass.
Actual guest IPv4/IPv6 tests on one/two slots each complete **nine calls on workers,
with nine filter evaluations and zero services**. One-slot receive waits let the
sender run. Native stream EPIPE tests cover sendto, sendmsg and sendmmsg, with and
without MSG_NOSIGNAL. Other ABI cases cover lazy items, waits, short/partial
errors, post-send faults without replay, original OFD/output after close/reuse,
remap faults before dequeue, packet info/truncation, timeouts and netlink exclusion.
The final cfg(test)-only SIGPIPE fixture accepts the connection first so Linux
listener close cannot substitute ECONNRESET for EPIPE. Only that changed case and
strict checks rerun; other results and unchanged production artifacts are reused.
Physical iPad testing remains skipped per owner. The measured runner is installed
in Trames Dev; the stable app hash remains unchanged.

The coordinator-removal goal remains active. Raw UDP byte/vector I/O, netlink,
Unix owning ancillary graphs, native-service/device and io_uring controls,
task/job control, mount/proc ownership and structural file/VM/task transitions
remain. recvmmsg is unimplemented, not an existing coordinator path. See the
[validation record](../validation/worker-ip-message-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#retained-ip-messages-and-batches-2026-11-03).


## Twenty-seventh migration implemented — raw IP byte operations — 2026-11-03

Runtime `0c051a18b2298b97fa31fa87a4e766faa3db271e` removes raw IPv4/IPv6 read, write,
readv and writev ownership from the coordinator for UDP and IP sockets without a
native TCP byte capability. Native connected TCP retains its original capability
path, and existing pipe/device/Unix byte resources return before the new fallback.
Those accepted paths gain no membership lookup, allocation or publication guard.
The fallback accepts the actual complete shared socket OFD, rather than a numeric
FD or a new resource-service proxy.

Raw calls reuse the original Linux scalar/vector preparation helpers, preserving
scalar caps, vector-count/signed-length errors, empty-vector behavior and original
access-versus-buffer fault order. UDP vectors gather/scatter one packet. Scalar
receive preflight is factored once; no duplicate ordinary check is added. Accepted
send bytes or receive scatter destinations survive close/reuse and later iovec
edits. Only parked receives recheck their original outputs before dequeue. The
original live O_NONBLOCK status, accepted socket timeout, Socket algorithms,
namespace policy and accounting remain in use. Raw writes do not acquire the TCP
publication algorithm guard that the original raw VFS write did not acquire.

Existing socket events and packet-stack timers release execution slots. Virtual
TCP EPIPE retains its saved result and original socket through task handoff, with
no packet resend or numeric re-resolution. Netlink byte/control calls still defer
before guest input import. No new global resource lock, coordinator/service thread,
message bus, busy polling or ordinary guest-memory barrier is introduced.

| Four processes × 1,000 checked round trips, longest-child median | Previous | Current | Native macOS | Native ARM Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| UDP scalar write/read | 177.37 ms | 15.06 ms | 9.74 ms | 1.16 ms | 11.78x |
| UDP vector writev/readv | 180.63 ms | 14.72 ms | 10.04 ms | 1.23 ms | 12.27x |
| Existing native TCP scalar | 14.91 ms | 16.17 ms | 13.63 ms | 2.36 ms | No gain claimed |
| Existing native TCP vector | 15.51 ms | 15.66 ms | 10.05 ms | 2.48 ms | No gain claimed |
| Existing native TCP sendmsg/recvmsg | 15.94 ms | 17.25 ms | 10.72 ms | 2.66 ms | No gain claimed |
| Existing pipe lifecycle | 12.52 ms | 12.34 ms | 1.84 ms | 2.18 ms | No gain claimed |

UDP scalar/vector process-wall medians are **208.75 → 46.19 ms** and
**213.00 → 45.66 ms**. Diagnostic cohort counts fall **5,650 → 48** and
**5,759 → 49**. Service requests remain three, including process startup/teardown;
old data calls generally joined rather than counted as resource services.

The short native-TCP scalar and message controls initially show approximately
8% higher loop medians; both raw data sets are retained. Longer confirmation uses
**10,000 iterations and nine unprofiled counterbalanced samples**. Native-TCP
scalar loops are **93.121 → 93.616 ms**, wall **126.363 → 126.582 ms**; existing
message loops are **139.132 → 139.020 ms**, wall **171.204 → 172.480 ms**. No
control gain or statistical regression bound is claimed. Native TCP vector and
pipe short controls also remain close; their complete records are retained.

Each new iteration checks a 64-byte round trip. Vector cases use two iovecs and
preserve one UDP packet; TCP handles any short read while assembling its frame.
Endpoint setup/cleanup is outside the loop timer. Native ARM Linux uses the same
static musl ELF on six vCPUs; Trames uses four. macOS uses its native byte/vector
APIs. All compilation, affected local CI and FFI acceptance finish before timing,
and Linux is stopped during Mac timing. Different kernels/APIs/CPU counts and
short-sample variability make native results contextual. These are not whole-app,
Firefox, physical-iPad or universally native-speed results.

Apple/Linux ABI **201 each**, kernel **277 each**, strict checks, Mac **13**, all
**13** FFI environment checks, product boundaries and iPad static build pass.
Sixteen actual guest combinations cover native/packet-stack × IPv4/IPv6 × TCP/UDP
× one/two execution slots. Each completes **ten calls on workers, ten filter
evaluations and zero services**. Virtual TCP is explicitly checked to have no
native byte capability; single-slot cases assert a real socket wait. ABI cases
cover packet boundaries, faults before effects, original OFD/output after reuse,
parked remap faults before dequeue, live nonblocking/timeouts, virtual TCP EPIPE
handoff and netlink exclusion. The final test-only wait assertion reruns its
sixteen-combination Apple case and strict checks; other passes and unchanged
production artifacts are reused. Linux full277 includes the assertion. Physical
iPad testing remains skipped per owner. The measured binary is installed in
Trames Dev; the stable application hash is unchanged.

The coordinator-removal goal remains active. Netlink, Unix owning ancillary graphs,
native-service/device and io_uring controls, task/job control and pending signals,
mount/proc ownership and structural file/VM/task transitions remain. recvmmsg is
unimplemented rather than an existing coordinator path. See the
[validation record](../validation/worker-raw-ip-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#retained-raw-ip-byte-operations-2026-11-03).


## Twenty-eighth migration implemented — retained Netlink data and resource wakeups — 2026-11-03

Runtime `0119510ad2457277e72d1bdd2401f29fa3f3b015` moves Netlink raw/vector/message/batch
data, supported route/link/address changes, and non-publishing netfilter/conntrack
operations onto the calling worker, shared by Mac and iPad. The actual socket OFD,
creating namespace, accepted credentials, input/output and batch progress remain
the same across waits and owner handoff. Close/reuse cannot redirect accepted work.

Each real Netlink queue owns its readiness. Replies and multicast wake reads, poll
and epoll through existing resource subscriptions, without a reactor or periodic
service. Poll captures and watches remain weak, so they do not pin final close.
Notifications are deduplicated and flushed only after queue, registry, topology
and outer Socket gates release; callbacks can receive and change options on the
same socket. Blocking receives return execution slots instead of occupying them.

Namespace-FD targets are resolved through actual shared namespace OFDs before the
topology writer. Canonical validators retain original error order. Both Network
and complete OFD lifetimes extend beyond topology/source gates and notification
flush, preventing final namespace teardown from reentering those locks. Top-level
and nested veth-peer targets are covered. No new global resource lock, coordinator
or service thread, message bus, busy polling, ordinary guest-memory fence, native
access policy or unsafe code is introduced. Existing TCP and accepted byte-resource
paths retain their earlier admission without a new fallback lookup.

**nf_tables ruleset publication remains joined.** Only a netfilter batch beginning
with NFNL_MSG_BATCH_BEGIN takes that boundary. Newly published rules can invalidate
already-captured native TCP policy-free capabilities. This stage preserves that
invariant without adding a permission/generation load or fence to every TCP byte
operation. Earlier sendmmsg items can finish on the worker; the original current
item, source and completed count survive owner handoff without replay or another
seccomp decision. Existing connected native TCP transport stays selected across
route/link changes, so those updates can proceed without this policy boundary.

| Four processes × 1,000 checked iterations, longest-child median | Previous | Current | Native macOS | Native ARM Linux, exact ELF | Improvement |
| --- | ---: | ---: | ---: | ---: | ---: |
| Netlink scalar write/read | 160.22 ms | 7.41 ms | No native API | 4.29 ms | 21.62x |
| Netlink vector writev/readv | 163.38 ms | 7.62 ms | No native API | 2.25 ms | 21.45x |
| Netlink sendmsg/recvmsg | 160.50 ms | 8.06 ms | No native API | 4.79 ms | 19.92x |
| Netlink query + poll + read | 237.56 ms | 8.99 ms | No native API | 4.45 ms | 26.43x |
| Existing native TCP scalar | 16.98 ms | 15.10 ms | 15.54 ms | 2.33 ms | No gain claimed |
| Existing pipe lifecycle | 12.49 ms | 12.42 ms | 1.90 ms | 2.23 ms | No gain claimed |

Netlink process-wall medians are **193.36 → 41.60 ms** (scalar),
**197.65 → 41.67 ms** (vector), **193.70 → 42.39 ms** (message), and
**272.43 → 44.55 ms** (poll). Separate diagnostic cohort counts fall
**5,402 → 50**, **5,312 → 48**, **5,624 → 43**, and **7,961 → 51**.
Service requests remain three, including startup/teardown; old Netlink calls
usually joined rather than counted as resource services. TCP/pipe controls show
no observed degradation; no control gain or statistical regression bound is claimed.

Three unprofiled counterbalanced samples run after release builds, initial affected
CI and FFI checks; diagnostics use a separate repeat. The final test-only epoll
assertion is checked afterward without changing measured production code. Four Trames vCPUs serve four
independent processes. Endpoint setup/cleanup is outside the loop timer. Queries
use RTM_GETLINK for interface index 1 without dump; every reply checks declared
length, type, sequence and interface index. Native ARM Linux runs the same static
musl ELF on six vCPUs, then is stopped during Mac timing. **Trames replies contain
292 bytes; native Linux replies contain 1,488 bytes**, reflecting different
attribute sets. Native timing is therefore contextual, not identical reply work.
macOS has no native Netlink API; only TCP/pipe controls have native Mac references.
These are not whole-app, Firefox, physical-iPad or universally native-speed results.

Apple/Linux ABI **205 each**, VFS **453/452**, network **81 unit + 11 integration
each**, kernel **278 each**, Mac **13**, strict checks, all **13** production FFI
environment checks, three project-boundary checks and iPad static build pass.
Filtered distinct-process guest tests with one/two execution slots exchange link
multicast, poll, receive and ACK: all **six calls finish on workers**, with **six
filter decisions and zero services**; the single-slot case asserts a real wait.
ABI tests cover scalar/vector/message/kernel-batch queries, reentrant callbacks,
original OFD/output after close/reuse, pure resource wakeups, real epoll readiness,
weak final-close lifetime and partial batch owner handoff. VFS/network tests cover
namespace identity, original authority/error order and teardown outside locks.

The final cfg(test)-only epoll assertion reruns its affected Apple/Linux case and
strict checks; full passes and unchanged production artifacts are reused. Final
full kernel runs include distinct VM identities. An initial new test attempted
unsupported bridge placement; it was corrected to already-supported veth targets,
without a production workaround. Physical iPad testing remains skipped per owner.
The measured binary is installed in Trames Dev; stable remains unchanged.

The coordinator-removal goal remains active. nf_tables publication, Unix owning
ancillary graphs, native-service/device and io_uring controls, task/job control,
mount/proc ownership and structural file/VM/task transitions remain. recvmmsg is
unimplemented rather than an existing coordinator path. See the
[validation record](../validation/worker-netlink-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#retained-netlink-data-and-resource-wakeups-2026-11-03).


## Twenty-ninth migration implemented — worker firewall publication — 2026-11-03

Runtime `8e7cb1f5cf1c2e6edbf4c207810410d2c3708fa8` removes the nf_tables publication join.
The calling worker uses the original shared rules writer, bounded staging,
rollback, validation, generation, response-capacity and counter algorithms.
Netlink no longer needs per-message worker admission or owner/deferred state for
these batches. This stage supersedes the firewall boundary retained in stage 28.

The actual rules object now includes a relaxed atomic empty-hook selector, updated
with successful rules publication under its existing writer. TCP capabilities
refer to their original shared socket and live policy, instead of capturing an
unfiltered decision. With no hook, a byte attempt adds one selector read and takes
no namespace/rules lock. The original general socket policy path also loses its
empty-policy locks. With hooks, the same original packet/conntrack/counter/error
algorithm runs; rule removal restores the same fast endpoint. Overlapping calls
may observe either side of publication, but a parked call does not retain an allow.

Policy attachment is immutable before socket publication. Byte/control/poll
capabilities keep one weak original-socket pointer; their size and per-admission
reference-count operation counts stay unchanged. Accepted leases retain the full
original shared native socket and namespace after close/reuse. Poll snapshots do
not pin final close. Host registries retain that same Arc without a new wrapper;
their existing final-resource destruction runs outside the registry lock. Original
conditional-close admission observes both socket and transport leases. SO_ERROR
uses original live-policy connection completion, closing the second stale-decision
path. Ordinary byte I/O takes no connection-completion or control algorithm lock.
No global resource lock, coordinator/service thread, polling bus, ordinary guest
fence, unsafe code or native-only access policy is added.

| Four processes, longest-child median | Iterations each | Previous | Current | Native macOS | Native ARM Linux, exact ELF |
| --- | ---: | ---: | ---: | ---: | ---: |
| nf_tables create/delete batch | 1,000 | 81.89 ms | 6.93 ms | No native API | 25.73 ms |
| Existing Netlink query | 1,000 | 7.74 ms | 7.08 ms | No native API | 4.28 ms |
| Existing native TCP scalar | 10,000 | 92.25 ms | 91.19 ms | 99.35 ms | 20.51 ms |
| Existing native TCP vector | 10,000 | 94.68 ms | 94.63 ms | 101.20 ms | 20.77 ms |
| Existing TCP sendmsg/recvmsg | 10,000 | 139.82 ms | 137.00 ms | 101.08 ms | 21.26 ms |
| Existing pipe lifecycle | 1,000 | 12.31 ms | 12.04 ms | 1.81 ms | 2.39 ms |

Publication loops improve **11.82x**, with process wall **114.29 → 43.30 ms**
and diagnostic cohort counts **2,295 → 48**. Service requests remain three,
including startup/teardown; the old publication usually joined rather than counted
as a resource service. TCP scalar/vector/message controls use **10,000 iterations
and nine unprofiled counterbalanced samples**: loop medians **92.250 → 91.186 ms**,
**94.678 → 94.633 ms**, and **139.820 → 137.003 ms**. Controls show no observed degradation. No control gain or statistical regression
bound is claimed. Query/pipe use three
unprofiled samples; complete raw results and separate diagnostic repeats are saved.

Each publication iteration creates and deletes a process-specific table in one
NFNL batch, then checks its 36-byte success ACK and sequence. It measures the
publication mechanism without live hooks; live DROP, counters, receive shutdown
and connection completion are verified separately. Native Linux runs the identical
static musl ELF on six vCPUs, with NET_ADMIN/root only inside the isolated local
validation machine. Its full kernel nf_tables semantics and differing PID/name
lengths make it a contextual reference. Trames has four vCPUs. Native macOS has no
Netlink API; TCP/pipe use original native APIs. Query replies remain 292 versus
1,488 bytes in Trames/Linux. All final compilation, affected CI and FFI checks
finish before timing; Linux is stopped before Mac timing. Setup/cleanup is outside
the loop. These are scoped results, not whole-app/Firefox/physical-iPad measurements
or a general faster-than-native claim.

Final Apple/Linux ABI **205 each**, VFS **453/452**, network **82 unit + 11
integration each**, kernel **279 each**, Mac **13**, strict checks, all **13**
production FFI environment checks, project boundaries and iPad static build pass.
Actual guest tests on one/two execution slots publish DROP, reject an old TCP
capability, delete rules and send again while a distinct process uses an eventfd.
All **seven calls finish on workers**, with **seven filter decisions and zero
services**. New network cases check active counters exactly once, captured
send/receive policy, retained original lifetime, weak final namespace teardown and
SO_ERROR denial/error consumption after publication. Existing concurrent rules and
packet-counter tests pass. Physical iPad testing remains skipped per owner. The
measured binary is installed in Trames Dev; stable remains unchanged.

The goal remains active. Unix owning ancillary graphs, native-service/device and
io_uring controls, task/job control, mount/proc ownership and structural file/VM/task
transitions still depend on separate owner boundaries. recvmmsg is unimplemented
rather than an existing coordinator path. See the
[validation record](../validation/worker-nft-publication-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#worker-nf_tables-publication-and-live-native-tcp-policy-2026-11-03).


## Thirtieth migration implemented — Unix IPC subsystem — 2026-11-03

Runtime `6ab059a7f0b57be7cab7e867c315ed8ed06ecd05` completes this batch across owning SCM_RIGHTS,
cycle admission/reclamation, native Wayland/XWM connections and clipboard relays.
Implementation is integrated as one subsystem change after concentrated local
validation. Unchanged passing checks are reused; failed-stage selection avoids
replaying complete graphics acceptance for a fixture correction.

Unix-bearing rights retain complete original shared OFDs. One runtime ownership
root validates/reserves the graph before queue publication; queue links are weak
back to that root, avoiding an Arc ownership cycle. Concurrent reciprocal sends
cannot both create a cycle. Listener-owned unaccepted endpoints participate in the
same graph. Last-close reclamation drains actual removed rights iteratively after
graph/queue/Files guards leave. No normal byte call takes the ownership graph
mutex. Non-Unix shared rights keep their direct queue path. Endpoint identity weak
references move into the existing byte resource rather than adding a second set;
there is one additional weak ownership-root reference per endpoint and a strong
root reference in owner socket views. No unchanged-total-refcount claim is made.

Native adapters now contain shared endpoints/OFDs and use per-connection state.
Guest queue changes drive the connection on the calling worker; native socket/pipe
readiness is nested into the existing idle executor's one-shot event queue.
Connection registry locks cover membership/auth revisions only, without native
I/O, callbacks or final destruction. There is no cohort-control pump, new resource
thread or polling bus. The joined single-task executor waits on that same queue
and dispatches actual events instead of scanning connections. Original compositor
and presenter affinity remains. Native import uses the original shared filesystem
and open-ID domain; the receiving task publishes actual guest FDs through Files.

Pipe readiness comes from the original PipeGuard. Full guest queues suppress host
reads; draining them rearms the original socket. Export acknowledgements notify
after the inode registry unlocks; compositor command capacity is subscribed only
when exports actually block. Closing the last compositor lease from its own event
loop cannot send into a full self-owned queue or join its own thread. No ordinary
guest memory fence, native-only permission model, global I/O algorithm lock or new
production unsafe is introduced.

| Four processes, longest-child median | Iterations each | Previous | Current | Native macOS | Native ARM Linux, exact ELF |
| --- | ---: | ---: | ---: | ---: | ---: |
| Unix OFD sendmsg/recvmsg + original endpoint use | 1,000 | 180.61 ms | 12.67 ms | 1.37 ms | 2.36 ms |
| Unix OFD raw-read discard + last-close EOF | 1,000 | 185.59 ms | 16.03 ms | Omitted (local API behavior) | 4.33 ms |
| Addressed datagram carrying a Unix OFD | 1,000 | 221.71 ms | 49.30 ms | 1.78 ms | 2.04 ms |
| Existing pipe-FD ancillary control | 10,000 | 93.46 ms | 93.67 ms | 16.42 ms | 16.28 ms |
| Existing socketpair/control/close | 10,000 | 200.92 ms | 201.91 ms | 50.88 ms | 44.25 ms |
| Existing pathname connect/accept/close | 10,000 | 227.65 ms | 226.12 ms | 62.65 ms | 43.42 ms |
| Existing native TCP scalar | 10,000 | 92.08 ms | 93.50 ms | 98.54 ms | 20.29 ms |

The three owning-rights loops improve **14.26x / 11.58x / 4.50x**; process-wall
medians are **215.49 → 48.48 ms**, **221.04 → 49.63 ms**, and
**341.32 → 172.89 ms**. Separate diagnostic cohort counts fall from
**5,241 / 4,706 / 4,913** to **46 / 50 / 52**. The three service requests still
include startup/teardown; a whole application is not claimed coordinator-free.

Owning cases use three unprofiled counterbalanced samples; ordinary controls use
10,000 iterations and nine samples. Controls remain close (pipe +0.2%, socketpair
+0.5%, connect -0.7%, TCP +1.5%); no control improvement or statistical regression
bound is claimed. All final builds/functional acceptance finish before host timing,
with local Linux CI stopped. Unchanged native references are reused; the two longer
controls are recollected at the same iteration counts. Both Trames CPU benchmark
paths disable windows/Vulkan and record zero GPU calls. The final candidate includes
pinned static MoltenVK, whereas the saved baseline used the dynamic-loader build.
These are scoped CPU/IPC measurements, not Firefox/GUI or physical-iPad speedups.
Native kernel, compiler and API differences make contextual references.

A bounded local Darwin probe found raw read made a previously closed descriptor
number live again while discarding ancillary bytes, keeping its peer open until
explicit cleanup. That raw native macOS workload is omitted; no different
recvmsg/close workload is substituted. Native ARM Linux runs the identical ELF.
This records the local observation without generalizing to every macOS version.

Apple/Linux ABI **207 each**, verified VFS cases **462/461**, shared resources
**15 each**, Wayland **21 each**, kernel **280 each**, strict checks and both
product boundaries pass. The Apple VFS full pass plus affected native refinements
are reused rather than replayed; Linux runs the final complete affected suite.
The filtered owning-rights guest finishes ten calls/ten policy decisions with zero
services on one/two slots. All three real native Wayland cohort scenarios finish
with **one cohort and zero services**: an unrelated CPU runs, all CPUs park, and an
already idle cohort is admitted. New regressions exercise reciprocal/listener
cycles, close/reuse, deterministic parked receive, reentrant final release, independent
native connections, bidirectional backpressure and clipboard EOF.

The production FFI passes all **13** Alpine/Debian/custom/systemd/multi-terminal
checks. Original guest Wayland info, two-window resize/close, weston SHM and Chinese
editor input pass. Native-linked vkcube passes with **27 frame revisions**; retained
SHM and GPU capabilities remain valid after session shutdown and release cleanly.
The initial cube failure was a fixture/native-link configuration issue; the final
script supports selecting failed stages, explicitly supplies the musl runtime and
fails immediately on a nonzero guest exit. The iPad static build includes the
pinned native GPU archive; unchanged guest outputs are reused. Physical iPad
validation remains skipped per owner. The measured complete native-linked binary
is installed in Trames Dev; the stable binary is verified unchanged.

Passing an actual owner-only task, mount or proc control projection still crosses
that resource's boundary; a shared Unix carrier cannot invent its thread authority.
GPU auth refresh/device controls, io_uring controls, job control and structural
filesystem/VM/task changes remain separate batches. See the
[validation record](../validation/worker-unix-ipc-20261103.json) and
[shared contract](../../runtime/docs/architecture/execution-backends.md#event-driven-native-unix-adapters-2026-11-03).

## Thirty-first migration implemented — native device controls — 2026-11-03

Runtime `ee0102c867930bb6b47cd6bc101ab471eed36a46` integrates audio OPEN, video surface
creation, live graphics FD authority and explicit GPU device/instance destruction
as one complete subsystem batch, with concentrated validation and one paired
runtime pin for both products. Unix IPC stage 30 remains the baseline.

Native audio/video callbacks retain the original event counter and the joined
executor's wake token. A producer can appear after a task parks, and survives
closing/reusing its descriptor without notifying a replacement. First/last native
producer transitions update existing subscriptions. The weak notification driver
is installed only for an actual native producer; ordinary eventfds keep their
previous construction path. The idle executor initializes its existing wait hub
under the original queue arm lock, closing the late-producer wake race.

Graphics calls resolve one live original OFD, retaining it only through that call.
Frozen grant tables, membership revision invalidation and their inode retention
are removed. New connections are visible immediately; foreign/copy-only FDs keep
original backend errors on the calling worker. Original shared-file flags and
fault/error order are preserved. Native image FD publication observes the actual
live Linux descriptor limit.

GPU close is an asynchronous job while native compilation or observed submissions
remain unfinished. It retains the accepted original target and releases its
execution slot. The pinned driver completion observer also covers native Wayland
frame fences. The original final native idle/destruction algorithm still runs on
the caller. Existing compiler threads publish completion before backend dependency
release; completed work does not join notification callbacks under a context lease.
No new resource thread, whole-cohort service pump, ordinary guest memory barrier,
native-only permission policy or production unsafe is introduced.

| Workload | Previous | Current | Native macOS | Native ARM Linux |
| --- | ---: | ---: | ---: | ---: |
| 2,000 video surface create/drop pairs, concurrent CPU peer | 743.28 ms | 4.04 ms | No equivalent API | No equivalent API |
| 20 GPU instance/device create/destroy cycles, concurrent CPU peer | 44.60 ms | 13.41 ms | 12.51 ms | No comparable GPU in local test machine |
| 4 processes × 10,000 ordinary eventfd lifecycles, longest child | 62.17 ms | 61.74 ms | No eventfd API | 14.60 ms |
| 4 processes × 10,000 existing pipe lifecycles, longest child | 101.72 ms | 102.32 ms | 21.20 ms | 16.28 ms (unchanged reference reused) |

Video/GPU loops improve **184.09x / 3.32x** in this scoped control benchmark.
Separate diagnostics show video cohort returns **2,003 → 3**, with **2,000 → 0**
native returns; GPU returns **45 → 3**, with **42 → 0** native returns. Both
candidate device cases have zero resource-service requests. One-slot execution
also passes. The GPU case includes one untimed warmup cycle on all implementations;
20 timed cycles use the same static MoltenVK archive as the native macOS reference.
Trames also performs its full capability/resource processing, while the native
reference uses minimum Vulkan features. This is a native API context reference,
not an assertion of identical driver configuration or Firefox frame rate.

Three unprofiled counterbalanced samples determine device medians; diagnostics
are separate. Total process-wall medians are **776.53 → 36.60 ms** for video and
**89.15 → 54.68 ms** for GPU, versus **28.77 ms** native macOS GPU. Native Linux
has no comparable GPU in this test environment, so a CPU driver is not substituted.
Ordinary controls use nine unprofiled samples: eventfd changes -0.7%, pipe +0.6%;
no control speedup or statistical regression bound is claimed. The current exact
Linux eventfd ELF is measured natively; the unchanged native Linux pipe reference
is reused. Builds/functional checks finish and Linux CI stops before Mac timing.
Both baseline and candidate are native-linked development runners. CPU controls
record zero GPU calls. These results do not predict physical-iPad or Firefox FPS.

Apple resource validation verifies **464 VFS, 15 shared, 57 Vulkan and 1 audio**
cases. Apple kernel validation reuses 282 passing cases plus the corrected late
notification case (283 total); only that failed test was repeated after correcting
its standalone/persistent-epoch setup. Linux runs the final affected suite:
**207 ABI, 463 VFS, 15 shared, 57 Vulkan, 1 audio and 283 kernel** cases. Strict
checks, Mac's 13 product tests and both product boundaries pass. Unchanged Apple
ABI and CPU/memory/network checks are reused rather than replayed.

The native-linked iPad static build and Mac development bundle build successfully.
All **13** production FFI environment checks pass, including Alpine, Debian,
systemd, custom roots and multiple terminals. Original Wayland info, windows,
SHM, Chinese editor and native vkcube pass with **28 frame revisions**; retained
frames survive session shutdown and release cleanly. Stock FFmpeg/libva hardware
H.264 export before sync, fork/SCM_RIGHTS and real GPU plane import pass.
Virtual Steam audio devices pass **48,000 input frames / 144,000 output frames**;
output drains successfully in 3.076 seconds. Capture retains aggregate levels
only and does not access the unreliable physical USB microphone. Physical iPad
validation remains skipped per owner.

Early extra acceptance failures were test configuration issues (unsupported raw
pure-profile calls in a Mac fixture, a nested rather than flat library directory,
and loader portability flags on the direct static native reference). The final
fixtures use ordinary Linux CPU work, the correct prepared dependencies and
supported direct driver initialization. Unaffected suites were not replayed.

The remaining boundaries are io_uring controls, task/job/proc/mount control,
structural filesystem/VM/task transitions, native guest mapping import and final
cancellation of unfinished context work. The overall removal goal remains active.
See the [source-bound validation record](../validation/worker-native-devices-20261103.json)
and [shared contract](../../runtime/docs/architecture/execution-backends.md#live-native-device-authority--2026-11-03).


## Thirty-second migration implemented — io_uring subsystem — 2026-11-03

Runtime `f9b6aec3a9eff34b7f733c2f1058684d49fbe80f` replaces the per-cohort io_uring mirror
with the original shared OFD, queue pages and one progression engine. This is one
subsystem batch, validated together across creation, registration, submission,
completion, cancellation, provided buffers, source-task background work and disk
sync. Mac and iPad use the same implementation.

| Subsystem operation | Execution and authority | Actual retained boundary |
| --- | --- | --- |
| Setup and register | Calling worker; original Files reservation, fixed tables and mmap pages | Buffer pinning changes the affected mm through its existing checkpoint |
| Submission and completion | One metadata importer/progression engine, original OFD and SQ/CQ cursors | Actual unsupported owner resource defers before effects |
| Bytes and socket controls | Calling/source worker, original byte/socket algorithms and namespace | Owner-only task/proc/mount descriptions retain their original owner projection |
| Timers, links, cancellation, multishot and provided buffers | Canonical per-ring state; data operations outside its mutex | Original validation, in-flight cancellation and CQ backpressure rules remain |
| Outstanding work after enter returns | Existing source task's View and event subscriptions | A foreign task never executes another address space's pointers |
| Plain disk fsync | Existing bounded disk pool; direct readiness result to the source task | Mapped/overlay writeback, file growth/append, backing allocation/truncation and O_TRUNC still use the filesystem boundary |

An accepted enter retains its original ring, submission count and deadline across
cohort stop, FD close/reuse and unmap. No queue/table restoration or request replay
occurs. Source maintenance can run while that guest task sleeps, waits on a futex
or waits for an owner lifecycle event; it cannot finish the unrelated syscall.
A real owner fallback is remembered until owner progress, preventing empty cohort
loops. Owner-accepted SQEs acquire their original source execution context before
the first worker pump, without re-importing guest metadata.

Queue publication alone uses the protocol's acquire/release instructions. Ordinary
payload bytes keep their existing raw access and region/hardware ownership. Native
fixed buffers retain that same SharedPage and borrow gate; the former blanket HVF
rejection of mapped pins is removed. Pins survive remap without observing a new
allocation. Provided buffer group reuse retains the old in-flight generation, so
an old completion cannot advance a newly registered group with the same number.

Unix descriptor transfer and ring registrations share one ownership graph. Both
admission orders reject an actual strong cycle; graph handles and original OFDs
retire outside metadata locks. Worker-owned records contain only Send-capable
original resource handles. Original owner Rc projections stay in the owner ledger
and are released at the owner boundary. No Rc is made Send, and no new resource
thread, message bus, stronger ordinary memory barrier or native permission policy
is added. Disk work reuses the existing pool.

The source-bound [validation record](../validation/worker-uring-subsystem-20261103.json)
records concentrated checks and scoped performance comparisons. Physical iPad
validation remains skipped per owner; the native-linked iOS build checks product
compatibility. These results do not establish whole-application speed or eliminate
structural filesystem, VM and task boundaries.


Three counterbalanced samples per case, with setup outside the longest-worker
timer and 1,000 checked iterations per worker, give these four-worker medians:

| Complete workflow | Previous | Current | Native ARM Linux, same ELF | Improvement |
| --- | ---: | ---: | ---: | ---: |
| SQE NOP / enter / CQ consume | 85.38 ms | 2.31 ms | 0.32 ms | 36.95x |
| Fixed-file registration / unregister | 260.47 ms | 3.45 ms | 0.44 ms | 75.56x |
| SQE STATX / enter / checked output / CQ consume | 102.92 ms | 30.94 ms | 18.39 ms | 3.33x |

Process-wall medians are 94.40 → 10.80 ms, 268.93 → 12.22 ms and
110.54 → 39.14 ms. Both signed runners are invoked directly, avoiding the Mac
CLI's extra bundle re-exec on only one side. Separate diagnostic cohort counts
are 31 → 26, 7,414 → 26 and 1,492 → 26; whole-process service requests remain
three on both sides, including startup/teardown. The new paths complete all
4,000 enters or 8,000 registration controls on workers. NOP already used workers;
its improvement primarily removes repeated queue-memory/metadata access costs.

Two-worker improvements are 14.82x, 39.62x and 7.86x. Single-worker loop medians
regress by roughly 8–18%, while process-wall medians remain close; the shared
canonical metadata has a measurable local cost. Four-way STATX still contends on
filesystem metadata. Native Linux references use the identical static ARM64 ELF
on the existing six-vCPU VZ test machine after host builds finish. Linux is stopped
for Mac samples. There is no native macOS io_uring equivalent; no substitute score
or whole-application/native-speed claim is presented.

Concentrated Apple/Linux memory **159 each**, Linux ABI **216 each**, kernel
**284 each**, Linux VFS with hostfs **465**, Mac product **13**, all **66** actual
HVF io_uring cases, **13** production FFI environment checks, strict checks and
native-linked Mac/iPad builds pass. Apple ABI includes the failed-only retry after
fixing its new test fixture; unrelated passing suites are reused. The five real
Wayland stages also pass. Full evidence and limits are in the validation record.

## Thirty-third migration implemented — regular-file subsystem — 2026-11-03

Runtime `7739836e2302f8fb43f0a7b437f8038ae854ddd6` moves the complete regular-file
data/EOF/durability ownership boundary together, common to Mac and iPad. Scalar/vector/positioned I/O, append/growth,
seek/stat, truncate/fallocate, all file sync variants, O_TRUNC and file-to-file
copy/sendfile share the accepted original OFD and common FileWork contract.
io_uring uses that same implementation. Short data work runs on the source;
blocking native work releases the execution slot, uses the existing bounded disk
pool and wakes the source directly. The Session disk service and separate ring
disk executor are removed. Metadata/quota no longer need cohort scan/copyback.

| Resource | Original authority / ownership | Retained boundary |
| --- | --- | --- |
| Bytes, EOF, position and metadata | Original per-file locks, shared quota account and canonical mode/times | Actual mapped-layout change quiesces only affected mm views |
| Native file completion | Accepted OFD and owned inputs; result/readiness to original source | Source copyout and original task signal disposition |
| Truncating opens | Resolved inode and hidden Files reservation; publish after success | Excluded owner-only path controls |
| Durability and copy | Original filesystem/file snapshot; accepted copy bytes and offsets retained | Namespace mutations and cross-resource file/pipe/socket transfer remain separate |

Concentrated acceptance caught and fixed deferred-page writeback, staged-import
quota double charging, filtered byte-call dispatch and pathname-truncate FD
ownership. A measured single-core regression exposed temporary descriptor-table
allocation: data calls now use the accepted original descriptor directly, sharing
the existing VFS algorithm rather than constructing a parallel implementation.

Five counterbalanced unprofiled samples, 1,000 checked 32-byte writes per worker,
produce these four-worker longest-worker medians:

| Workflow | Previous | Current | Native macOS | Native ARM Linux, exact ELF | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: |
| Memory-backed shared-OFD append | 18.55 ms | 2.86 ms | 12.37 ms | 1.43 ms | 6.49x |
| Memory-backed independent-file growth | 18.53 ms | 2.06 ms | 5.66 ms | 0.29 ms | 9.00x |
| Private disk shared-OFD append | 24.07 ms | 27.97 ms | 12.37 ms | 1.43 ms | 0.86x |
| Private disk independent-file growth | 31.50 ms | 13.38 ms | 5.66 ms | 0.29 ms | 2.35x |

One-worker memory append/growth improve about 10%; private disk append improves
4%, while growth is about 4% slower. Four-way private append is about 16% slower:
contention on one actual disk/OFD still incurs native wait/resume overhead. This
remaining cost is recorded rather than hidden by the independent-file results.
Separate diagnostics show zero coordinator services for the append/growth loops;
the bulk-read fixture still has two residual compatibility services on two/four
CPUs, with all 128 file reads executing on workers. Their exact syscall identities
have not been instrumented. No whole-process zero-service claim follows.

File/thread creation, start-barrier setup, record verification, close and process
startup/teardown are outside the worker timer; process-wall measurements remain in
the validation record. Timings run with compilation finished and local Linux CI
stopped. Darwin uses native filesystem APIs, with no native RAM-filesystem score
substituted. The identical static ARM64 ELF runs on the existing six-vCPU VZ Linux
CI instance/ext4 for context. Kernel/filesystem/compiler differences prevent these
references from establishing generally native speed or whole-application gains.

Apple/Linux VFS **473/472**, ABI **222 each**, kernel **275 each**, memory **159**
plus **2/4** focused cases, strict checks, Mac product **13**, real HVF file workflows
on **1/2/4 CPUs**, **66** io_uring flows, **13** production interpreter environment
checks and **5** real Wayland stages pass. The iPad device-target static/native app
link passes; physical iPad testing remains skipped per owner. Eleven obsolete
coordinator-service/slot/copyback kernel cases are replaced by resource-lifetime
and ABI coverage, plus two affected-mm/signal kernel cases, rather than retained
as assertions that the removed mechanism still exists. Unchanged passing evidence
and guest artifacts are reused; only changed or failed checks are repeated.

The [source-bound validation record](../validation/worker-file-subsystem-20261103.json)
contains full timings and limitations. The [shared ownership contract](../../runtime/docs/architecture/execution-backends.md#regular-file-data-eof-and-durability-subsystem-2026-11-03)
describes scoped layout waits. Directory/namespace mutation, task/proc/mount
controls, cross-resource file/pipe/socket transfer and structural VM/lifecycle
remain separate complete-subsystem work. The overall goal remains active.

## Thirty-fourth migration implemented — namespace and metadata subsystem — 2026-11-03

Runtime `af0aa49e60127236b2bf7858f95757306238ddee` moves ordinary directory
create/remove, link/symlink/rename, mode/owner/times/xattrs, cwd/chdir/fchdir/chroot
and statfs onto one imported ABI and original shared filesystem resources. The
canonical fs_struct owns shared cwd/root and detached pins; private Rc mount
projections stay inside their control island. CLONE_FS shares the original state;
fork/unshare retains its original copy behavior. Send+Sync is checked by the
compiler without widening the existing sealed PathQueries boundary.

Short calls borrow the current worker island; memory-only joined calls use the
original VFS directly. Only retained/native work builds a private island. Native
namespace work reuses the existing bounded disk pool, accepted original FDs and
source completion. Owner-only resources defer before effects through a distinct
FileOutcome::Owner, retaining copied operands and original identities. The
per-filesystem transaction protects flat namespace/checkpoint/rollback invariants,
including PTY creation/final-close; inode metadata transactions cover their actual
multi-field changes. Ordinary bytes do not acquire these transaction locks. There
is no new service thread, polling bus, guest memory fence or production unsafe block.

Five counterbalanced unprofiled pairs give the following longest-worker medians.
Each memory worker performs 1,000 checked lifecycles; each private-disk worker
performs 16, retaining the original per-mutation durable manifest checkpoints.

| Backing / workers | Previous | Current | Native macOS filesystem | Native ARM Linux ext4, exact ELF | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: |
| Memory / 1 | 55.81 ms | 56.98 ms | 538.33 ms | 20.12 ms | 0.98x |
| Memory / 2 | 500.49 ms | 62.64 ms | 719.15 ms | 28.67 ms | 7.99x |
| Memory / 4 | 1,195.47 ms | 127.74 ms | 1,085.08 ms | 37.36 ms | 9.36x |
| Private disk / 1 | 1,224.32 ms | 1,408.31 ms | 11.29 ms | 0.35 ms | 0.87x |
| Private disk / 2 | 2,410.77 ms | 2,407.75 ms | 12.23 ms | 0.48 ms | 1.00x |
| Private disk / 4 | 5,014.08 ms | 4,915.74 ms | 17.74 ms | 0.61 ms | 1.02x |

Single-worker private-disk latency increases **15.0%** (throughput decreases 13.1%):
retained preparation/native queueing remains an added path cost; its contribution
versus persistence has not been separately profiled. Two/four-worker private results show no material speedup claim. An early
implementation copied a whole private island for every call and doubled single-core
memory latency; borrowing the current island removed that architectural regression
before final acceptance. Further persistence/offload optimization is not started.

Separate diagnostics reduce two/four-worker memory cohort rounds **16,787 → 9**
and **31,958 → 19**; stop time falls **20,909 → 0 us** and **66,944 → 140 us**.
Service requests are zero on both versions: the old namespace path joined the
cohort without a Service RPC. These counts include scheduling/lifecycle and are
not a claim of zero checkpoints for a whole application. Private-disk cohort rounds
fall **204 → 7** and **265 → 17**, while original persistence still dominates time.

Parent-directory/thread/barrier setup and process startup/teardown are outside the
worker timer. Child mkdir/open/create, bytes, metadata, links, rename, stat, unlink
and close are inside. Process-wall medians and separate diagnostics are retained in
the record. All builds finish and the Linux reference machine is stopped before
Mac timing. Native macOS uses its own filesystem APIs; no native RAM-filesystem
substitute is used. Native Linux runs the exact static musl ELF on the existing
six-vCPU VZ test machine. Different backings, kernels and compiler targets, and
especially the absence of equivalent per-operation durable manifest flushes in the
native references, prevent a pure coordinator-overhead or generally native-speed
interpretation. No Firefox or physical-iPad performance result is claimed.

Concentrated acceptance passes Apple/Linux VFS **477/476**, ABI **224 each**,
and kernel **276 unique cases each**, plus affected strict and format checks.
The Apple kernel total combines 275 passing cases with the corrected new filtered
harness's one-case retry; unchanged production code and passing tests are reused.
Actual Mac HVF passes **12** namespace workflows and **66** existing io_uring flows
on **1/2/4 CPUs**. The production interpreter's **13** environment checks pass
(including three reused rootfs-install checks), as do **5** real Wayland stages,
the iPad static library and device-target native app link. Unchanged memory,
product/guest artifacts and unrelated acceptance are reused. Physical iPad testing
remains skipped per owner.

The [source-bound validation record](../validation/worker-namespace-subsystem-20261103.json)
contains hashes, complete samples and the regression. The
[shared contract](../../runtime/docs/architecture/execution-backends.md#namespace-metadata-and-canonical-fs_struct-2026-11-03)
and [architecture-wide inventory](../../runtime/docs/architecture/coordination-boundaries.md)
separate owner services, full cohort joins, affected-domain checkpoints, resource
waits and local fallbacks. This closes the current implementation round; remaining
boundaries are recorded without starting another subsystem migration.


## Thirty-fifth migration implemented — cross-resource stream transfer — 2026-11-04

Runtime `7b850d2bb1954a0933a45708255e1f8adf1abe22` moves the admitted sendfile/splice subsystem onto calling workers: regular files, pipes, Unix streams and TCP retain their original OFD pairs, one imported ABI and the original resource readiness. Memory operations run directly; native file jobs reuse the existing bounded disk pool. Pipe prefix/capacity reservations protect the original resources without holding their mutex over I/O or a parked task. Backpressure releases only the caller’s execution slot.

The same algorithm serves joined execution. Immediately completing calls borrow the original VFS; only retained/native work allocates an island. The existing short native read/write policy applies below 16 KiB, preserving queued inode work and synchronous output. Bulk completion wakes the original execution target and parks on notification rather than fixed polling sleeps. Captured signals distinguish resource waits from accepted in-flight disk I/O, including EINTR and SA_RESTART.

Five counterbalanced unprofiled pairs, each worker performing 500 checked 4-KiB transfers, give these four-worker longest-worker medians:

| Backing / route, 4 workers | Release 1.5.0 | Current | Native macOS | Native ARM Linux, exact ELF | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: |
| memory / pipe-pipe | 77.58 ms | 5.07 ms | 3.59 ms | 1.04 ms | 15.31x |
| memory / file-pipe | 124.69 ms | 2.96 ms | 2.33 ms | 0.33 ms | 42.07x |
| memory / file-unix | 55.79 ms | 3.38 ms | 1.99 ms | 0.92 ms | 16.53x |
| memory / socket-pipe | 77.55 ms | 5.05 ms | 2.90 ms | 1.15 ms | 15.35x |
| private / pipe-pipe | 110.91 ms | 11.95 ms | 3.59 ms | 1.04 ms | 9.28x |
| private / file-pipe | 141.22 ms | 54.30 ms | 2.33 ms | 0.33 ms | 2.60x |
| private / file-unix | 80.64 ms | 29.33 ms | 1.99 ms | 0.92 ms | 2.75x |
| private / socket-pipe | 104.37 ms | 10.68 ms | 2.90 ms | 1.15 ms | 9.78x |

Single-worker memory results are 1.01–1.09x and three private-file/pipe routes are 1.04–1.23x. The private socket/pipe case initially measured 12.82 → 14.04 ms; an additional nine counterbalanced pairs measured 13.38 → 13.13 ms. Both sets remain in the record; no stable speedup is claimed for that case. The rejected first implementation increased private single-worker file/pipe latency to roughly 1.5 seconds by allocating retained state and sleeping for each small transfer. Removing those mechanisms restored the short-call path before integration.

Separate diagnostics reduce four-worker memory cohort rounds from 1,385–3,769 to 18, and two-worker rounds from 872–1,917 to 8–9. Service requests are zero before and after: the previous transfers joined cohorts without service RPCs. Lifecycle/scheduling and actual mapped-file layout work still exist; this is not an entirely coordinator-free application claim.

Setup, thread creation, final verification, close and process startup/teardown are outside the worker timer. All compilation finishes and the native Linux test machine stops before Mac timing. Darwin uses native sendfile for file/socket and checked read/write for pipe routes; Linux runs the identical ELF on ext4. Different API, backing and durability policies prevent generally native-speed or pure coordinator-overhead claims.

Affected Apple/Linux tests pass: kernel **281 each**, Linux ABI **228 each**, VFS **460 each**, with the existing ignored cases retained. Strict checks, shared boundaries and iPad device-target compilation pass. Mac product tests pass **20**, HVF unit checks **4**; real HVF executes all three fixture groups on **1/2/4 CPUs**, with memory and private backing. The production interpreter passes those groups on **1/4 CPUs**. Two existing virtual TCP tests use local larger debug stacks; no production stack changes. Physical iPad testing and unchanged CPU/GPU/media suites are reused or skipped as recorded.

The [source-bound record](../validation/worker-stream-transfer-20261104.json) retains every timing sample, native references, separate diagnostics, hashes and the first implementation’s regression. The [shared transfer contract](../../runtime/docs/architecture/execution-backends.md#cross-resource-stream-transfer-2026-11-04) and [remaining-boundary inventory](../../runtime/docs/architecture/coordination-boundaries.md) describe mapped-file layout checkpoints, owner/dynamic exclusions, lifecycle and mask/control paths. tee/vmsplice remain unsupported; no new coordinator/service thread, polling bus, global algorithm lock, guest memory fence or unsafe block is added.


Post-integration stability fix (2026-11-04): Debian Docker useradd exposed an
overlay checkpoint/stat lock inversion. Immutable mount device identity and
filesystem-before-inode directory/copy-up traversal remove that cycle without
adding a coordinator or global lock. The [scoped audit and regression record](../validation/overlay-checkpoint-lock-order-20261104.json)
includes three successful runs of the owner's unmodified repro in an isolated
Debian clone and two additional recursive-read corrections. The shared
[lock-order contract](../../runtime/docs/architecture/execution-backends.md#overlay-checkpoint-and-stat-lock-order-2026-11-04)
describes the preserved checkpoint and resource ownership boundaries.


## Thirty-sixth migration implemented — retained event waits — 2026-11-04

Runtime `64177108fb431bcd746d2cd1280c306f97af8fc3` moves admitted pselect6, ppoll, epoll_pwait and epoll_pwait2, including temporary signal masks, onto calling workers. A single imported wait keeps original OFDs/sets or the original epoll object, timeout/deadline and mask through parking and cohort handoff. Existing execution slots and resource notifications handle waiting. No coordinator/resource thread or polling bus is added.

Queued and arriving I/O signals interrupt on the source worker. The handler uses the temporary mask, ucontext saves the pre-wait mask and sigreturn restores it. EINTR remains nonrestartable for these waits under SA_RESTART. Actual unblocking synchronizes with the original signal inboxes; ordinary memory and unchanged/blocking-only masks gain no additional fence. Raw pselect6/ppoll remaining-time copyout follows native Linux best-effort behavior. Seccomp decides once before inputs and keeps that decision through parking or a typed owner fallback.

Five counterbalanced unprofiled pairs, each worker running 500 checked write/wait/read iterations, give these four-worker longest-worker medians:

| Wait family, 4 workers | Mac 1.5.1 | Current | Native macOS | Native ARM Linux, exact ELF | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: |
| pselect6, no mask | 59.25 ms | 3.32 ms | 2.39 ms | 0.39 ms | 17.82x |
| ppoll, temporary mask | 59.06 ms | 3.10 ms | — | 0.52 ms | 19.06x |
| epoll_pwait, temporary mask | 58.76 ms | 3.79 ms | — | 0.53 ms | 15.50x |
| epoll_pwait2, temporary mask | 58.59 ms | 4.27 ms | — | 0.51 ms | 13.73x |

Masked pselect6 at four workers is 59.15 → 3.29 ms (17.99x). Newly migrated two-worker paths improve 8.7–11.7x. Separate diagnostics reduce four-worker migrated-wait cohort rounds from 1,523–1,795 to 18–24, and retire all 2,000 waits on workers. Epoll setup retains four existing service requests outside the timed loop; counting only service RPCs would have missed the previous per-wait joins.

Nine-pair rechecks with 5,000 loops confirm the pre-existing unmasked ppoll four-worker path is within 1.9% (28.36 → 28.92 ms), and unmasked epoll variants improve about 7% (37.14 → 34.74 and 36.81 → 34.46 ms). Longer single-worker tests remain about 2–4% slower, roughly 0.07–0.21 µs per write/wait/read iteration; no single-core speedup is claimed. The first intermediate implementation added avoidable epoll output buffers and a ready-poll Arc. Both were removed before integration. The record retains those timings rather than silently replacing them.

Concentrated Apple/Linux acceptance passes 283 kernel and 229 ABI cases each, plus signal-kernel tests, strict affected clippy/format checks, paired project boundaries and the iPad device-target FFI check. The same static ARM64 fixture passes nine cases per run on native Linux, Mac HVF with 1/2/4 CPUs and the production interpreter with 1/4 CPUs. It covers raw timeout updates/read-only copyout, pending and arriving FASYNC signals and exact handler/sigreturn masks. Unit regressions also cover original epoll close/reuse and one-time ALLOW/ERRNO seccomp evaluation across parking. Unchanged release acceptance is reused; physical iPad and Firefox performance are not claimed.

The [source-bound record](../validation/worker-event-waits-20261104.json) contains every sample, binary/source hashes, native references and separate diagnostics. The [shared event-wait contract](../../runtime/docs/architecture/execution-backends.md#retained-event-wait-subsystem-2026-11-04) and [remaining boundaries](../../runtime/docs/architecture/coordination-boundaries.md) retain tracing, excluded owner/dynamic FDs, unavailable source pumps, actual legacy signal/task effects, VM/layout and lifecycle work. Mac Dev is updated for this development batch; stable release 1.5.1 is kept separate.


## Thirty-seventh migration implemented — POSIX and real interval timers — 2026-11-04

Runtime `2a5d554fba9c7bd6c17ef1e313828a15ecd95010` moves all seven previously supported timer APIs—timer_create/gettime/getoverrun/settime/delete and getitimer/setitimer—onto calling workers. The common Mac/iPad implementation owns original timer objects and signal-target incarnations. Queries lock only their object after namespace lookup; creation/deletion protect the timer namespace. A short shared deadline-index transaction publishes expiry hints. Existing executors process bounded due batches outside the execution-queue mutex; an expiry alone no longer joins the whole cohort.

SIGEV_NONE installs no asynchronous target/deadline: queries calculate its current periodic phase lazily and overrun remains zero. Ordinary joined short timer controls also keep the CPU quantum. Native Linux comparison corrected the prior timer_settime flag rejection and preserves ITIMER_REAL across exec while deleting POSIX timers. Fork inherits neither; final process exit cleans up the remaining objects.

Five counterbalanced unprofiled pairs, 500 iterations per worker, give these four-worker longest-worker medians. The baseline is the previous Mac Dev/runtime `6417710`, including the event-wait migration:

| Timer operation, 4 workers | Previous Dev | Current | Native macOS | Native ARM Linux, exact ELF | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: |
| POSIX gettime, SIGEV_NONE | 56.72 ms | 0.91 ms | — | 0.11 ms | 62.16x |
| POSIX settime, SIGEV_NONE | 56.21 ms | 0.91 ms | — | 0.13 ms | 61.63x |
| POSIX create/delete, SIGEV_NONE | 111.59 ms | 1.68 ms | — | 0.80 ms | 66.30x |
| getitimer, ITIMER_REAL | 56.70 ms | 0.93 ms | 0.15 ms | 0.51 ms | 60.93x |
| POSIX settime, SIGEV_THREAD_ID | 56.12 ms | 1.11 ms | — | 0.21 ms | 50.51x |

Separate diagnostics reduce four-worker cohort rounds from 1,810–3,628 to 18–20. Service requests are zero on both sides: prior timer calls joined cohorts without RPCs. Nine-pair single-worker rechecks with 5,000 iterations improve 1.10–1.15x. The existing no-timer ppoll path is within 1% on one/four workers. These results isolate API/index costs; the notified settime case resets future deadlines, rather than claiming a native interrupt-latency or whole-Firefox score.

Concentrated Apple/Linux acceptance passes 285 kernel and 15 signal/timer-kernel cases each; the unchanged Linux ABI suite reuses its 229 passing cases each. The same final static ARM64 fixture passes five cases on native Linux, Mac HVF with 1/2/4 CPUs and the production interpreter with 1/4 CPUs: relative/absolute clocks, ownership/deletion, lazy notification-free periods, payload/overrun, exact thread targets, masked waits and fork/exec. Existing libc SIGEV_THREAD callbacks and all 25 signalfd workflows additionally pass HVF on one/four CPUs. A source-worker regression completes create/arm, expiry, caught handler/sigreturn and delete on one existing slot with zero service requests; seven API cases check one-time ALLOW/ERRNO seccomp. Strict affected clippy, format, paired boundaries, Mac product tests and iPad device-target FFI compilation pass. Physical iPad/Firefox acceptance is not claimed.

The [source-bound validation record](../validation/worker-posix-timers-20261104.json) retains full timings, native references, binary/source hashes and separate diagnostics. The [shared timer contract](../../runtime/docs/architecture/execution-backends.md#shared-posix-and-real-interval-timers-2026-11-04) and [remaining-boundary inventory](../../runtime/docs/architecture/coordination-boundaries.md) preserve real lifecycle/default signal effects, legacy owner waits and unsupported CPU-clock timers. No new timer/coordinator/service thread, polling bus, ordinary-memory fence, unsafe block or global algorithm lock is added.

## Thirty-eighth migration implemented — shared signal queues, controls and reader-scoped waits — 2026-11-04

Runtime `2b388148c192b6c4511c528bde21b21ac6d03dcf` closes the normal signal queue/control/wait dependency chain. The migration commit `40ff2f7` is preserved; a following focused commit removes redundant owner publication for read-only action queries. Mac and iPad use the same shared implementation.

Owner-generated, ordinary guest, FASYNC and timer signals now share one task-private inbox and one process inbox. There is no duplicate Rc pending queue or stale owner-pending snapshot. Disposition changes and guest sends synchronize on the original sighand/queue objects; callbacks run after resource locks are released. Consumption, discard and a pending timer's overrun update do not manufacture new readiness edges. A weak presence token distinguishes a zombie from a reaped task even if a reader retains its old inbox.

The source worker handles altstack/actions/masks/pending, ordinary kill/tkill/tgkill, sigtimedwait/sigsuspend, and signalfd read/readv. Reader authority belongs to the current caller, including fork/SCM_RIGHTS and shared epoll. Accepted waits retain their copied operands, original OFDs and deadlines through close/reuse and cohort handoff. io_uring read/readv/fixed-read/poll uses its submitting task and the same record-copy algorithm; fixed files and original pinned bytes survive closing, unregistering and remapping. Ordinary owner queries use the same decoder without redundant context handoffs. Signal-control ERRNO and unsupported TRACE/USER_NOTIF decisions complete locally; TRAP/KILL still retain task-policy authority.

Five counterbalanced unprofiled pairs, 1,000 iterations per worker, produce these four-worker longest-worker medians:

| Operation, 4 workers | Previous Dev | Current | Native macOS | Native ARM Linux, exact ELF | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: |
| Mask query | 113.59 ms | 1.40 ms | 0.16 ms | 0.14 ms | 81.23x |
| Mask mutation | 113.39 ms | 1.38 ms | 0.62 ms | 0.19 ms | 82.32x |
| Action query | 1.52 ms | 1.46 ms | 0.16 ms | 0.19 ms | 1.04x |
| Action mutation | 112.40 ms | 5.41 ms | 0.73 ms | 0.61 ms | 20.79x |
| Thread signal + sigwait | 454.58 ms | 5.85 ms | — | 2.50 ms | 77.76x |
| Thread signal + signalfd read | 456.33 ms | 6.07 ms | — | 2.47 ms | 75.15x |

The send/signalfd diagnostic drops from 14,380 cohort rounds to 7; the remaining returned SVCs are thread creation/exit, not the timed signal calls. Service requests are zero on both sides, so the improvement cannot be attributed to RPC counts alone. Nine longer counterbalanced single-worker pairs, 10,000 loops each, range from 0.997x to 1.018x: no sustained single-core regression is observed in those samples. Short mask-query samples were noisy (including 1.46–3.52 ms after); the record keeps those results and the longer recheck rather than silently replacing them.

The Linux reference uses the exact static ARM64 ELF in the existing local ARM Linux VM, stopped before Mac timing; it is not a bare-metal Darwin score or a change to the product architecture. Darwin lacks signalfd. The native self-send/sigwait multiworker fixture timed out on this host, so those cells are omitted; all Linux reference counts pass.

Local acceptance passes 287 Apple kernel cases, 17 signal-kernel cases, two public signal API cases, 230 ABI cases and 493 VFS cases. Linux passes the affected ABI/VFS suites and final 29/13 signal subsets, reusing the already passing unrelated kernel cases. Mac product tests, strict affected clippy/format, paired boundaries and iPad device-target FFI compilation pass. The same existing ELF fixtures pass all 25 signalfd workflows and five POSIX timer workflows on final HVF and interpreter builds at 1/4 CPUs. New regressions cover six wait APIs with one worker slot and zero owner services, mask restoration across handoff, reaped PID/reader lifetime, and async fixed-file/buffer identity.

The [source-bound validation record](../validation/worker-signals-20261104.json) contains native comparisons, all source/binary/log hashes, longer solo results and separate path diagnostics. The [shared signal contract](../../runtime/docs/architecture/execution-backends.md#shared-signal-queues-controls-and-reader-scoped-waits-2026-11-04) and [remaining dependencies](../../runtime/docs/architecture/coordination-boundaries.md#shared-signals-and-remaining-lifecycle-dependency) explicitly retain the connected lifecycle unit: STOP/CONT/default termination, parent wait/SIGCHLD, robust/clear-tid cleanup, resource release and reaping. This batch does not claim that chain is coordinator-free. Mac Dev is installed; stable 1.5.1 and its running Docker environment are unchanged. No new coordinator/service thread, polling bus, unsafe block or ordinary guest-memory fence is added.


## Futex retirement correction after shared signals — 2026-11-04

Runtime `d9c4a811faf316a51b1a8f33f055247943a18855` fixes logical wait
cancellation independently of retained owner-state lifetime. A caught signal on
a worker used to drop its wait while an owner checkpoint still retained an Arc.
The old WAITING ticket could then consume a later FUTEX_WAKE intended for the
same task's new wait, stranding Go GC and Docker operations. Worker/owner caught
signal entry and active/stopped exit now explicitly cancel and unlink it using
the original keyed protocol. Drop remains the final cleanup backstop.

This correction adds no coordinator/service request, global algorithm lock,
ordinary-memory fence or Go preemption workaround. Deterministic tests retain
owner clones across interruption and exit; normal-preemption stock Go stress and
eight isolated Docker lifecycle cycles pass. The existing lifecycle boundaries
remain unchanged. [Source-bound evidence](../validation/reader-runtime-20261104.json).


## Thirty-ninth migration — shared resource-limit subsystem — 2026-11-05

Runtime `f361bde2824f6b629a85e336774f8eceee3492e7` retains migration commit
`33b00d5` and closes its file-size error completion boundary. All supported
getrlimit/setrlimit/prlimit64 calls, including non-self PID/TID targets and
ordinary errors, use original shared process limits on source workers. Thread
clones share them; forks copy them and exec retains them. Host-authorized exec
binds its replacement VFS, while FD unshare keeps the process soft-limit atomic. Credentials, PID
membership and weak task presence keep their original identity and custody.
A retained limit cannot keep a reaped PID visible or retarget reuse.

The actual FD soft atomic and live file-size atomic enforce updates. Original
OFD metadata remains shared, with the caller's limit bound separately. Both fast
file capabilities and accepted/native work observe reductions. The old stable
binary fails a real threaded short-write case after a limit reduction; the new
shared implementation passes. Inline/native error completion and io_uring
accepted contexts route SIGXFSZ to the original task, including a closed fixed
file and pumping from another context. AIO already uses its source channel.
Ignored dispositions and retired targets are respected. Solo clone and actual
credential changes publish signal identity at their custody boundary. Caught signals retain
source handler/sigreturn; default task termination still has its real boundary.

Nine counterbalanced pairs measure the longest thread body, with 1,000 calls per
thread. Times below are four-thread medians. Linux is the same static ARM64 ELF
on the local native ARM Linux reference instance, not bare-metal Darwin.

| Work | Old HVF | New HVF | Native ARM Linux | Improvement |
| --- | ---: | ---: | ---: | ---: |
| getrlimit | 151.95 ms | 1.42 ms | 0.196 ms | 107.22x |
| setrlimit | 150.63 ms | 1.47 ms | 0.518 ms | 102.73x |
| Cross-thread prlimit64 query | 151.40 ms | 1.42 ms | 0.264 ms | 106.25x |

Final single-worker medians are within 0.9% of the old binary. A slower pilot
self-TID query exposed unnecessary full-directory publication; it now borrows
the original object without authority copies or repeated translation. Ordinary
file-growth controls show no regression; all pilot and final samples are retained. The diagnostics are separate from timing: 4,000 limit owner returns
fall to zero and cohort rounds fall from 3,700 to 7. Both sides have zero service
requests, so RPC counts alone would miss the change. This is a subsystem result,
not a Firefox or whole-system speedup claim.

Apple and ARM Linux pass 293 process/kernel-conformance cases, 21 kernel unit
cases and 233 ABI cases; the existing affected VFS results are reused (496 Apple,
495 Linux, one ignored each). Strict checks and the iPad FFI target compile pass.
Targeted rechecks correct CPU-allowance assumptions and join both concurrent
transfer attempts before retry; no production transfer algorithm changes.

[Source-bound evidence and all samples](../validation/worker-limits-20261105.json)
· [Shared contract](../../runtime/docs/architecture/execution-backends.md#worker-owned-process-resource-limits-2026-11-05)
· [Remaining boundaries](../../runtime/docs/architecture/coordination-boundaries.md).
The connected lifecycle, credentials, namespaces and VM checkpoints remain
explicit. This batch adds no service/coordinator thread, polling bus, global
algorithm lock or ordinary-memory fence.

The final pinned Dev binary additionally passes 30 normal-preemption stock Go
GC stress starts and eight isolated Docker start/health/exec/inspect/stop/remove
cycles at twelve CPUs. Public Debian tools and Alpine content are used; no
application data, frontend activation or paid model requests are involved.
Mac Dev is installed. The physical iPad execution is not claimed; its shared
runtime pin and device-target compile are verified.

## 40. Original child waits and lifecycle event ownership (2026-11-05)

The task-lifecycle migration now has an original shared parent/child event
model. A process owns its `Children` family; threads share that family and the
child group's wait event, while fork creates new objects. Supported wait4 and
waitid calls execute over those objects on their source workers, including
blocking waits, WNOHANG/WNOWAIT, stopped/continued events, PID namespaces,
competing reapers, output faults and caught-signal EINTR/SA_RESTART. Accepted
waits subscribe before checking and relinquish their execution slot. They do
not scan the owner's Process table, send an owner request or copy out guest
bytes while holding resource locks.

Reaping updates one shared PID-visibility bit immediately. Signal/limit
lookups and proc observations stop exposing that task even before its owner
projection is collected. Projection collection now precedes returned owner
PID queries, so an immediate getpgid after source reaping returns ESRCH. A
retained resource handle owns neither the task lifetime nor a replacement PID.
Structural relationships publish only on custody changes; solo wait polling
also avoids rebuilding the full directory on every call.

Nine counterbalanced before/after pairs use the same static ARM64 musl fixture.
The four-thread workload performs 4,000 wait queries; the solo control performs
1,000. Native reference measurements use the local native ARM Linux instance,
not bare-metal Darwin.

| Work | CPUs | Old HVF | New HVF | Relative speed |
| --- | ---: | ---: | ---: | ---: |
| wait4 poll | 1 | 1.487 ms | 1.458 ms | 1.020x |
| waitid poll | 1 | 1.579 ms | 1.620 ms | 0.975x |
| wait4 poll | 4 | 172.905 ms | 4.214 ms | 41.031x |
| waitid poll | 4 | 172.291 ms | 4.192 ms | 41.098x |

The solo controls vary within about 3%; the waitid solo result is a small
regression rather than a claimed improvement. Diagnostic runs separately
verify that wait syscalls no longer return their cohorts to the owner. Both
old and new paths have zero service requests, so RPC counts alone would miss
the removed coordination. This is a wait-subsystem result, not a whole-system
or browser speedup claim.

The isolated twelve-CPU Docker run exposed a real integration error: an
owner-blocked child wait acquired both its typed child-wait request and a
legacy I/O retry envelope. After the child event completed, it was replayed as
unsupported I/O. Child custody now excludes that duplicate envelope; the real
Kernel regression imports a blocked wait, completes it and follows it with an
owner PID query. Native Linux also corrected previous pointer-fault semantics:
no-result WNOHANG does not preflight output pointers, while failed copyout after
consumption does not resurrect the child. wait4/waitid retain Linux's differing
rusage copy order. Temporary event tracing is removed from the final build.

[Source-bound evidence, samples and acceptance](../validation/worker-child-waits-20261105.json)
· [Shared lifecycle boundary contract](../../runtime/docs/architecture/coordination-boundaries.md).
Apple and ARM Linux pass 296 process conformance cases (three ignored); the
unchanged final kernel sources retain their 32 passing unit cases. Strict
checks and the iPad device-target FFI compile pass. Real binary acceptance uses
HVF at one/four/twelve CPUs and the production interpreter at one/four CPUs;
this does not claim physical-iPad execution. Unchanged ABI/VFS results from
stage 39 are reused.

**The full lifecycle migration remains open.** Default STOP/CONT/termination,
exit/group-exit cleanup, clear_tid, parent-death/reparenting policy,
namespace-init teardown and resource finalizers still retain their documented
boundaries. In particular, `Epoch::finish` still stops the cohort. This batch
adds no coordinator/service thread, polling bus, global algorithm lock or
ordinary-memory barrier. Mac Dev is the review target; the installed stable
1.5.3 environment is kept separate.

The final normal build additionally passes 30 stock Go GC stress starts with
normal asynchronous preemption and eight isolated Docker lifecycle rounds at
twelve CPUs. The successful eight-round run contains no temporary event tracing.
Mac Dev is installed from this paired runtime pin; the stable binary hash is
unchanged. Failed pilot logs, both custody fixes and every timing sample remain
in the source-bound evidence.

## 41. Original thread-group STOP/CONT and wake custody (2026-11-05)

STOP/CONT now runs on the source executors over one original shared thread-group
control object. It replaces owner-private stopped/saved-state wrappers with a
stop overlay over the original ready, futex, sleep, child/signal and I/O/GPU
waits. Generation clears opposing pending signals under the group lock;
callbacks execute after unlocking. Guest send APIs, timers, resource/FASYNC
signals and parent-death generation use the same path. Masked or ignored CONT
still resumes the group. Same-session permissions and orphaned job-control
groups retain Linux behavior.

An assignment holds a group execution lease. STOP interrupts only this group's
active CPUs; WUNTRACED/SIGCHLD waits for every live member and accepted
uninterruptible operation. An owner VM execution batch also participates, so it
cannot acknowledge stopped and then execute guest instructions. Pending KILL
suppresses a late stop acknowledgement. Original waits retain operands, OFDs,
subscriptions, deadlines and eventual results without replaying accepted work.
Stopped user poll/socket/signal deadlines do not keep executors busy.

Real multi-round acceptance exposed a lost wake hidden by syscall tracing:
a STOP notification remained fired when a stopped job parked, so later CONT
and thread-directed TSTP notifications coalesced into that old wake. The queue
now rearms the original task signal under its existing mutex before checking
level state and parking. Four consecutive stop/wait/continue/wait cycles pass
with one and four execution slots and zero owner requests. This uses the
existing queue and notification, with no new control thread or polling bus.

Seven counterbalanced pairs measure 100 STOP/wait/CONT/wait rounds with a
two-thread target group, parent controller and unrelated spinning peer:

| CPUs | Previous HVF | New HVF | Relative speed | Native ARM Linux |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 1734.902 ms | 1735.042 ms | 1.000x | 3.306 ms |
| 4 | 404.773 ms | 68.187 ms | 5.936x | 2.960 ms |
| 12 | 404.726 ms | 64.167 ms | 6.307x | — |

The single-CPU owner scheduling path is unchanged; this is not a claim that it
approaches native latency. Native measurements use the existing ARM Linux Lima
instance with one/four-CPU taskset affinity, not bare-metal Darwin. Multi-CPU
latency also remains substantially above native. This is a job-control result,
not an extrapolation to Firefox or all workloads.

A separate four-CPU diagnostic reduces cohort rounds from 205 to 6 and signal
syscall returns from 200 to zero. Cohort stop time drops from 398.863 ms to
2.019 ms; both paths have zero service requests. Fork, setpgid and actual exits
still appear as the expected structural returns. The final normal build has no
temporary lifecycle tracing.

[Source-bound measurements and acceptance](../validation/worker-job-control-20261105.json)
include 297 process cases and 40 kernel cases on both Apple and ARM Linux
(three unchanged ignored process cases), strict affected checks, project/fmt
checks and the iPad device-target compile. Real compatibility passes HVF at
one/four/twelve CPUs and the production interpreter at one/four CPUs, each with
ignored, blocked and caught SIGCONT. Native reference compatibility passes all
three dispositions. The clean Mac binary also passes 30 normal-preemption Go
GC stress starts and eight isolated twelve-CPU Docker lifecycle rounds.
Physical-iPad execution is not claimed. Unchanged CPU/ABI/VFS/graphics results
are reused rather than rerunning unrelated suites.

**Lifecycle removal remains open.** `Epoch::finish`, default termination,
exit/group-exit cleanup, clear_tid/robust-futex cleanup, final resource release,
structural fork/exec/reparenting and namespace-init teardown still retain their
actual dependencies. Tracing, vfork and accepted mapped-file/legacy-owner work
are explicit boundaries; a stop does not discard those operations to obtain
an early acknowledgement. The common implementation is pinned by both products.
Mac Dev is the installation target; the stable 1.5.3 binary/environment remains
separate and unchanged by this development batch.

## 42. Original cgroup hierarchy, freezer and notifications (2026-11-05)

Cgroup paths, control files and membership now use one original shared resource
graph. Source mkdir/open/metadata, read/write/readv/writev, pread/pwrite, seek,
poll/epoll and inotify can use it without returning the cohort. OFDs keep inode
identity through deletion, controller removal and pathname reuse. Operations
use the current caller's PID namespace and credentials. A retained device
factory follows actual membership changes instead of retaining its old cgroup.

The freezer uses original task capabilities and existing executor wakes. Its
request flag is separate from completed freeze: active guest execution and
accepted uninterruptible operations must relinquish the affected task before
`cgroup.events` reports frozen. Nested freeze, inherited freeze, migration,
thaw and SIGKILL keep the original wait custody. The owner serial quantum and
VM batch participate in the same execution lease. A regression also covers a
non-leader exec that rekeys a lease before its completion.

Population/frozen counters update at actual membership transitions; ordinary
control reads do not rescan the task table. Event generations belong to the
original control inode and wake existing epoll/inotify waiters after unlocking.
Linux reference checks cover initial ERR/PRI, read acknowledgement, seek and
retained deleted files. The real two-thread target fixture additionally checks
both counters stop, ancestor/local freeze, migration, pids EAGAIN notification
and killing a frozen thread group while an unrelated peer remains runnable.

Seven counterbalanced pairs time 100 freeze/observe/thaw/resume rounds:

| CPUs | Previous HVF | New HVF | Relative speed | Native ARM Linux |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 1202.384 ms | 1202.507 ms | 1.000x | 598.979 ms |
| 4 | 810.354 ms | 12.730 ms | 63.657x | 17.956 ms |
| 12 | 810.356 ms | 12.981 ms | 62.426x | — |

Both versions use the same pread-based observation and 50 us retry/resume sleep,
because the old runtime rejects cgroup epoll registration. Separate functional
acceptance exercises real epoll on the new runtime and Linux. The native
reference is the existing six-vCPU ARM Linux Lima instance with one/four-CPU
affinity, not bare-metal Darwin. Timers and scheduling affect these short
measurements; the four-CPU result does not establish general superiority over
native Linux. The unchanged one-CPU owner scheduling cost remains substantial.

A separate four-CPU diagnostic reduces cohort rounds from 409 to 6. Returned
write/pread syscalls drop from 404 to zero; service requests are zero before and
after. Cohort stop time drops from about 811 ms to 5.9 ms. Remaining structural
fork/open/exit returns stay visible. This is a cgroup workload result, not a
Firefox or whole-system speedup claim.

[Source-bound acceptance and measurements](../validation/worker-cgroup-20261105.json)
and the [shared contract](../../runtime/docs/architecture/coordination-boundaries.md#shared-cgroup-hierarchy-freezer-and-control-events-2026-11-05)
record the precise remaining boundaries. Structural fork/exec/exit, final
Files/mm/native ownership and cancellation, clear_tid, reparenting, namespace
topology and BPF installation still require their actual lifecycle authority.
No coordinator/service thread, polling bus or ordinary-memory barrier is added.
The shared implementation supports both products; the stable 1.5.3 environment
is separate from this Dev batch.

Apple ARM64 and native ARM Linux pass 298 process cases (three ignored), 40
kernel cases, 233 ABI cases (one ignored) and 470 VFS cases (one ignored).
The all-features standalone VFS suite, strict affected Clippy, project/fmt
checks and iPad device-target FFI compilation pass. Real acceptance covers HVF
at one/four/twelve CPUs and the production interpreter at one/four CPUs. The
four existing cgroup fixtures also pass all twelve HVF combinations, including
non-leader exec, namespaces, stopped/frozen signals, pids limits and live BPF
device policy. The final source passes 30 normal-preemption stock Go GC stress
starts and eight isolated twelve-CPU Docker lifecycle rounds. No new physical
iPad run is claimed. Unchanged CPU, graphics and other subsystem results are
reused.


## 43. Original task addresses, Files lifetime and cancellation (2026-11-05)

`set_tid_address` now updates the original task's relaxed atomic address on its
executing worker, including allowed/denied filter handling. It performs no guest
memory access at registration. This fixes a pre-existing ABI error where a
read-only address could fail to replace the registration, despite Linux accepting
it. Fork/exec and owner exit share the same per-task state. A real musl thread
fixture verifies read-only registration, later protection change and clear_tid
wake, as well as a rejected registration preserving the birth address.

Files lifetime no longer depends on owner Rc projections. Each real task owns a
logical Files reference; its executor can retire that reference and final-close
complete OFDs before owner collection. Accepted operations retain their originals.
Late factories/publication and a cloned closed view cannot resurrect membership.
POSIX process locks and io_uring cancellation expose the original resource
capabilities, and existing owner cleanup uses the common algorithms. Cancellation
preserves another submitter and never retargets a reused ring FD.

Complete OFDs now share lifetime independently of whether their body is directly
admitted. This includes stdio and volatile proc nodes. Actual excluded epoll,
namespace, OOM/network proc and mount-watch controls remain explicit owner
destructors, reported rather than hidden. Registered dynamic files still defer
before body effects and keep their original identity across close/reuse. Owner
and source cgroup/volatile reads and seeks also use one canonical OFD position;
positioned writes do not change it.

Seven counterbalanced pairs time four ready threads, each registering 1,000
addresses, then restoring its clear_tid address and exiting:

| CPUs | Previous HVF | New HVF | Relative speed | Native ARM Linux |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 5.527 ms | 5.518 ms | 1.002x | 0.545 ms |
| 4 | 134.630 ms | 1.483 ms | 90.762x | 0.287 ms |
| 12 | 132.607 ms | 1.516 ms | 87.476x | — |

Thread creation, initial barrier and final stack unmap are outside the interval;
release, registration, clear_tid joins and thread exit are inside. Single-CPU
scheduling shows no meaningful improvement. Native uses the same static musl ELF
on the existing six-vCPU ARM Linux Lima instance with one/four-CPU affinity,
not bare-metal Darwin. Unchanged native fixture samples are reused. This is a
focused syscall/dependency measurement, not a Firefox or whole-system speedup.

[Source-bound evidence](../validation/worker-task-resources-20261105.json) includes
Apple and native ARM Linux process/kernel/ABI/VFS acceptance, standalone VFS
features, strict affected checks, iPad device-target compile, production
interpreter and HVF compatibility at one/four/twelve CPUs. The four existing
cgroup fixtures cover all twelve HVF combinations. The isolated Go/Docker store
is separate from business environments. Physical-iPad execution is not claimed.

**Lifecycle migration remains open.** Actual exit/group-exit still joins, and
full process relationships, robust futex, final mm/native ownership, owner control
destructors, tracing and vfork still need their real dependencies removed.
`Epoch::finish` still stops the cohort. The APIs in this batch are not reported
as complete worker exit. The shared implementation supports both products;
only Mac Dev is the installation target. Stable Trames 1.5.3 and the business
Debian/Techo deployment remain unchanged by this development batch.

The separate four-CPU diagnostic reduces cohort rounds **3,780 → 7** and
returned set_tid_address calls **4,004 → 0**. Actual thread exits and fork still
return at their structural boundary. Three service requests remain in both
versions; this is not a zero-service whole-application claim. Recorded cohort
stop time is 3,058 → 24 us, distinct from total elapsed syscall/scheduling time.

Apple and native ARM Linux pass 299 process cases (three ignored), 40 kernel
cases, 235 ABI cases (one ignored), then the added registered-proc-file regression
on both platforms, and 477 VFS cases (one ignored). Standalone VFS all-features
passes 507 cases (one ignored). These affected results are reused after metadata,
documentation and test-only refinements; unrelated CPU/GPU acceptance is unchanged.


## 44. Source individual thread exit and original parent-thread relationships (2026-11-05)

Individual exit(93) now completes on its calling worker when another original
group member is alive and its Files contain complete shared descriptions. The
group arbitrates this transition; concurrent exits leave one last member for
actual process cleanup. Cancellation, cgroup membership, clear_tid/futex and
Files retirement use their original resources. Non-leader presence disappears
from shared PID queries before owner projection collection. A leader keeps its
waitable process identity and live-group signal target.

Parent-thread links and death-signal registration now belong to one original
shared graph/task object. Source PR_SET/GET_PDEATHSIG obeys existing ALLOW/ERRNO
filter handling. Retirement updates actual children, not a process-wide scan;
callbacks follow the graph transaction. Exec keeps the actual surviving thread,
including its future TID. Childless retirement skips unused survivor selection.
Owner group exit/exec also retire all original task identities and preserve
parent-death/cancellation work that the older group-drop path omitted.

A source exit drops its View and returns its CPU without ending the cohort or
making a service request. Cold remaining projections are collected only after
Views return, before processing another owner syscall or the final group exit.
This is not a claim that all final CPU/mm/native resources already retire on the
source. Actual excluded owner controls defer before effects, and actual legacy
futex adoption keeps its existing boundary.

The removal exposed a real lost notification: the final exiting View of an mm
could leave a pending mapping checkpoint asleep. This was captured with all
workers waiting, the requester held and its futex already woken. The queue now
publishes the original domain's VmReady event. A deterministic regression fails
without that one notification and passes with it while keeping peers running.
The final code has no diagnostic environment switch or extra retry/polling path.

Final clean acceptance includes 200 interpreter and 100 twelve-CPU HVF runs of
the complete PID-namespace fixture with profiling disabled. All pass after the
fix. Additional acceptance covers parent death, fork/exec, credentials,
subreaper, namespace teardown, clear_tid, filtering and all four cgroup fixtures.
The common code passes the physical-iPad FFI target compile; new physical-iPad
execution is not claimed. Stable 1.5.3 and business environments stay separate.

[Source-bound measurements and acceptance](../validation/worker-thread-exit-20261105.json)
and the [shared boundary contract](../../runtime/docs/architecture/coordination-boundaries.md#source-individual-thread-exit-and-original-parent-thread-links-2026-11-05)
record the precise limits. The last member, exit_group/default fatal exit,
full reparenting/namespace-init policy, robust futex, final mm/native ownership,
tracing/vfork and actual owner control graphs remain open. The goal remains active.


Seven counterbalanced pairs use four already-ready threads. The short case
performs one address registration, restores clear_tid and exits each thread:

| CPUs | Previous HVF | New HVF | Relative speed | Native ARM Linux |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 43.084 us | 43.417 us | 0.992x | 12.543 us |
| 4 | 264.417 us | 133.292 us | 1.984x | 31.460 us |
| 12 | 296.750 us | 153.000 us | 1.940x | — |

The 1,000-registration control case changes from **1.476 to 1.318 ms** at four
CPUs and **1.529 to 1.313 ms** at twelve CPUs; the single-CPU **5.495/5.511 ms**
results show no meaningful gain. Removing unused childless survivor selection
also removes the initial roughly 4 us single-CPU regression: the final short
single-CPU result is **43.084/43.417 us**, with no meaningful improvement claimed.

The interval includes barrier release, registration, clear_tid joins and thread
exit. Clone/setup and final stack unmap are outside it; remaining cold owner
projection collection is not claimed as included before join. Native uses the
exact ELF on the existing six-vCPU ARM Linux Lima instance with one/four-CPU
affinity, not bare-metal Darwin. The unchanged native 1,000-registration samples
are reused; the short case has seven fresh native samples per affinity. These
are focused join/exit latency results, not Firefox or whole-application scores.

Apple and native ARM Linux pass **301** process cases (three ignored) and **46**
kernel cases. The final source passes strict checks and iPad compilation. The
unchanged VFS/ABI/CPU/GPU results are reused. Thirty stock Go GC-stress starts and
eight isolated twelve-CPU Docker lifecycle rounds pass with normal preemption.
The final childless optimization skips only unused relationship selection;
integration results are reused after that semantic-preserving change.

The unmodified native PID-namespace fixture does not fully pass: its existing
negative ioprio_get assertion expects EINVAL, whereas the native probe returns
ESRCH. That difference predates this migration and is retained in the evidence;
no assertion is weakened or silently counted as native success. The unmodified
native parent-death program and exact thread benchmark both pass. Trames passes
the complete namespace fixture, including the 300 final unprofiled stress runs.


A separate four-CPU short-case diagnostic has **6 → 4** cohort rounds and
**4 → 0** returned exit(93) calls. Three service requests remain in both versions,
including startup work; remaining returns include clone and console ioctl.
Measured stop time is 147 us versus below the timer's 1 us reporting resolution,
not a claim that synchronization costs are literally zero.


## 45. Original process families and parent completion (2026-11-05)

Parent IDs and subreaper state now belong to the original shared Family. A
thread's short prctl/getppid entry does not consult the coordinator or replay
cached parentage. Fork starts a new subreaper state; threads share the original.
Original structural relationships decide adoption, namespace-init victims,
parent wait membership and parent signal targeting. Adoption of a zombie also
uses the actual new parent's disposition and can retire its PID immediately.
The owner no longer runs a second parent/subreaper/reparenting algorithm.

The full namespace fixture found an exec handover bug: a nonleader survivor had
new presence but the same PID/group/UID, so publication skipped its old family
record and pruning erased the wait relationship. Registration now tracks the
original presence identity and keeps live family membership through handover.
A deterministic regression fails without the fix and passes with it. No extra
retry, polling loop or all-CPU stop substitutes for the missing relationship.

The final process boundary is still owned: CPU/VFS/mm/native teardown and actual
namespace-victim projection collection have not moved to source workers. This
batch supplies their actual shared dependency APIs. Last-member/group/default
fatal exit, robust futex, final mm/native ownership and actual owner control
resources remain open. New tests cover adoption, ignored/NOCLDWAIT zombies,
leader-thread death, PID reuse, namespace boundaries and nonleader exec.

[Source-bound acceptance and measurements](../validation/worker-family-20261105.json)
· [Shared boundary contract](../../runtime/docs/architecture/coordination-boundaries.md#original-process-families-and-completion-notifications-2026-11-05).


Seven counterbalanced pairs use four already-ready threads, each performing a
subreaper set, checked subreaper get and getppid per round. The timed interval
includes barrier release and thread joins; clone/setup and final stack unmap are
outside it. Native runs the exact ELF in the six-vCPU ARM Linux Lima reference,
with one/four-CPU affinity; it is not a bare-metal Darwin comparison.

| Rounds/thread | CPUs | Previous HVF | New HVF | Relative speed | Native ARM Linux |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 1 | 80.083 us | 81.500 us | 0.983x | 17.664 us |
| 1 | 4 | 521.250 us | 133.500 us | 3.904x | 27.662 us |
| 1 | 12 | 606.750 us | 156.084 us | 3.887x | — |
| 1,000 | 1 | 16.750 ms | 16.719 ms | 1.002x | 1.866 ms |
| 1,000 | 4 | 272.096 ms | 3.704 ms | 73.466x | 0.962 ms |
| 1,000 | 12 | 259.220 ms | 3.757 ms | 69.000x | — |

The long-case diagnostic changes **6,858 to 4** cohort rounds and **8,004 to
zero** returned prctl entries. Both versions still have three service requests,
including startup work. Remaining returned entries include clone and console
ioctl. The single-CPU path shows no meaningful improvement. This deliberately
stresses these control APIs; it does not estimate browser or Docker throughput.

Apple and native ARM Linux pass **302** process cases (three ignored) and **52**
kernel cases. Strict affected checks, formatting and the physical-iPad FFI target
compile pass. A new physical-iPad execution test is not claimed. Initial cross-
executor acceptance includes all four cgroup fixtures, filtering, clear_tid,
parent death and 100 interpreter/100 HVF namespace repetitions. After the final
namespace full-group cleanup correction, another 50 namespace runs span HVF
one/four/twelve CPUs and interpreter one/four CPUs. Thirty normal-preemption Go
GC starts and eight isolated twelve-CPU Docker lifecycle rounds also pass.

Namespace shutdown now retains each original leader through full-group cleanup,
including an already exited leader with live peers. Its regression fails against
the per-thread projection loop and passes with the full original-group path.
The native full namespace fixture's preexisting negative ioprio expectation
difference remains recorded in stage 44; no native full-fixture pass is claimed.

## 46. Original native mm ownership and completion retirement (2026-11-05)

GPU, video and audio roots now follow the actual shared address space. Real
CLONE_VM tasks count once each; worker snapshots retain a capability, not another
live task. Fork isolates native contexts, exec replaces the original mm reference,
and source/owner exits retire the same reference once. Weak context caches keep
ordinary lookups out of the mm membership mutex. Captured source native bodies
keep their existing context/stream leases and gain no new lock or permission test.

The Kernel's three native lifetime maps, native-liveness ProcessMap scans and
abandoned-GPU drive-loop poll are removed. Pending compilation and final GPU work
retain their original context until existing completion events arrive. Busy
context callbacks return to their existing unlock notification, rather than
blocking a driver callback or headless work. The event handshake rearms repeated
signals and publishes its own context unlock; it creates no service thread, bus,
guest-memory fence or new unsafe declaration.

Two guarded regressions distinguish this from moving an owner request: a headless
program finishes while a different native context is locked, and final release
returns while two native jobs remain gated. Restoring owner polling or disabling
completion retirement makes the corresponding test fail. Additional cases cover
actual fork/CLONE_VM composition, stale source exit capabilities, concurrent last
reference release, accepted native leases and destructor reentry after unlocking.

The last process/group entry and final CPU/Memory custody remain open. Actual
native idle/destruction remains its original algorithm; non-notifying GPU
backends and native audio/decoder stop APIs retain their real driver behavior.
There is no claim that the full exit chain has become source-only.

[Source-bound acceptance and regression controls](../validation/native-space-20261105.json)
· [Shared boundary contract](../../runtime/docs/architecture/coordination-boundaries.md#original-native-address-space-lifetime-and-completion-retirement-2026-11-05).


Apple and native ARM Linux each pass **309** process cases (three ignored) and
**61** Vulkan and **15** shared-resource cases; audio unit coverage also passes. Strict affected
checks, formatting and the physical-iPad FFI target compile pass. Fresh acceptance
runs real GPU/device and video controls on one/four/twelve HVF CPUs, parent death,
fork/exec, namespace teardown and clear_tid on both executors. Hardware H.264,
export-before-sync, fork/SCM_RIGHTS and native Vulkan plane pixel comparison pass.
Thirty normal-preemption Go GC starts and eight isolated twelve-CPU Docker
lifecycle rounds pass. No new physical-iPad or real microphone test is claimed.

Seven counterbalanced pairs retain the existing control workload and a concurrent
CPU peer. These measure ordinary throughput as a regression control; pending-
compilation exit is proved by gated regressions rather than represented as a GPU
throughput improvement. All builds and the native Linux test VM are stopped
before timing. The native macOS reference uses the unchanged native control
artifact from stage 31 with seven fresh samples. Native Linux has no comparable
GPU in this local instance; video surface metadata has no equivalent native API.

| Workload | CPUs | Previous | Current | Native macOS |
| --- | ---: | ---: | ---: | ---: |
| 20 GPU device cycles | 1 | 25.048 ms | 25.096 ms | 12.689 ms |
| 20 GPU device cycles | 4 | 13.236 ms | 13.246 ms | 12.689 ms |
| 20 GPU device cycles | 12 | 13.370 ms | 13.368 ms | 12.689 ms |
| 2,000 video surface pairs | 1 | 255.573 ms | 255.549 ms | — |
| 2,000 video surface pairs | 4 | 4.012 ms | 3.984 ms | — |
| 2,000 video surface pairs | 12 | 4.048 ms | 4.000 ms | — |

Ordinary throughput is essentially unchanged. The mechanism removes blocking
owner polling and slow-job lifetime coupling, rather than claiming faster GPU
driver execution. Existing CPU/ABI/VFS acceptance is reused where unchanged.

## 47. Original thread-group exit completion — 2026-11-05

The original group distinguishes stop-executing from cleanup-complete. Source
non-last exits retain membership through original resource retirement and their
final guest View; owner exits use the same completion phases. The last ordinary
begin-exit fixes status, while an earlier explicit group exit fixes it once.
Member completion order cannot change that status. Original liveness is maintained
under the existing group lock, replacing whole-ProcessMap checks for group final
status, family publication and process-lock release.

This exposed two projection hazards: late stop/hold updates could re-register a
dead original member, and a structural refresh could mistake an owner-held caller
absent from ProcessMap for a completed group. Both now use original lifetime. A
real Linux program also demonstrates the old incorrect leader status (11) versus
the actual last-thread status (37). See Linux's
[synchronize_group_exit and wait implementation](https://github.com/torvalds/linux/blob/v6.18/kernel/exit.c).

Source CPU return now collects only local scheduling, Memory and VFS projections,
without repeating original native/FD retirement, uring cancellation, clear_tid or
parent-death effects. Original ancillary-rights and detached-mount projection GC
is preserved; owner retirement collects it once. Occupying the original native
context cannot block source projection collection. Injecting legacy native
cleanup makes that guarded regression fail, and omitting directory projection GC
makes the actual detached-submount case fail.

Apple validation: **311 process**, **55 shared kernel**, **507 VFS with hostfs**
checks pass, with three process and one VFS test intentionally ignored. Native
ARM Linux passes the complete group semantic batch plus the final collector's
three affected regressions. The final binary passes 20 lifecycle cases over HVF
1/4/12 and interpreter 1/4, native GPU/video controls, hardware H.264 interoperation,
30 Go GC starts with normal async preemption, and eight isolated Docker cycles.
Strict affected Clippy, formatting and iPad-device-target compilation pass. No
physical iPad or new microphone test is claimed.

Seven counterbalanced pairs use the same ELF for 128 two-thread groups, each with
fork, clone, leader exit, final exit and parent wait. Half retain an actual owner
epoll control; benchmark mode exits both threads with zero so the old binary does
identical valid work. Builds and the Linux instance are stopped during Mac timing.
The native ARM Linux reference is the same ELF in the existing 6-CPU/8-GB test
instance, rather than a native macOS syscall equivalent.

| Workload | CPUs | Previous Dev | Current Dev | Native ARM Linux (6 CPUs) |
| --- | ---: | ---: | ---: | ---: |
| 128 two-thread groups | 1 | 16.704 ms | 16.472 ms | 7.661 ms |
| 128 two-thread groups | 4 | 29.559 ms | 29.732 ms | 7.661 ms |
| 128 two-thread groups | 12 | 29.212 ms | 29.275 ms | 7.661 ms |

This is a lifecycle regression control, not a general CPU or Firefox speedup.
Final last-member/group/default-fatal **entry remains joined**. The next connected
work includes final CPU/Memory custody, original vfork release, namespace-init
victims and their completion ordering, robust futex support and actual owner
controls. No service thread, polling bus or extra source admission gate is added.

The [machine-readable evidence](../validation/thread-retirement-20261105.json)
records exact revisions, source/binary hashes, controls and samples. Only Dev is
installed; stable 1.5.3 and business services retain their previous binary.

## 48. Original vfork release and cold projection completion — 2026-11-05

Runtime `7b91b6dc664606ef4c046f218df7fe5001052073` follows the original vfork child through mm release.
Its parent parks on actual readiness in the existing queue. Source exit drops
its View and publishes release directly; it does not await an occupied owner,
unrelated child peer or numeric-PID scan. A sole runnable producer now admits
its actual waiting parent. Owner exit/exec finish the same object once. Pending
vfork preserves STOP custody, and ring CQ progress cannot replace its clone
return value. Linux ties this to
[mm_release](https://github.com/torvalds/linux/blob/v6.18/kernel/fork.c), before full FD/process destruction.

Non-last source exit now accepts Files containing opaque owner controls. The
caller retires original logical descriptors; an event on the existing completion
channel collects private Rc controls and returned Memory wrappers on their owner
without stopping peer CPUs. Original Group cleanup membership finishes after
that collection. CPU/metrics still return once at cohort drain. Weak original
mm roots are saved only when a wrapper actually retires; they retain no RAM and
preserve backing publication through an undispatched CLONE_VM owner.

Original Families now publish status and optional host completion after their
actual namespace victim dependencies. A nested init still records those victims
when its ancestor already marked the namespace dead. This removes cleanup-order
assumptions, not the current joined execution of namespace shutdown.

Four bounded negative controls cover direct release with an occupied owner,
ancestor-dead namespace dependencies, missing real backing publication and the
sole-runnable vfork admission. All fail when the corresponding mechanism is
removed. Cold-control tests retain actual source captures while checking the
private Rc destructor. No new service thread, polling bus, guest-memory fence,
unsafe or source refusal gate is introduced.

Final Apple and native ARM Linux process suites pass **314 checks** each (three
ignored). Unchanged validated code retains 56 kernel, 159 memory, 507 Apple VFS
and 506 Linux VFS passes; platform/hostfs counts are explicit in the evidence.
Strict affected Rust checks and iPad device-target FFI compilation pass. The
final paired binaries pass 25 lifecycle cases (HVF 1/4/12 CPUs and shared
interpreter 1/4), native GPU/video, real H.264/Vulkan transfer, 30 Go GC starts
and eight isolated 12-CPU Docker rounds. No physical iPad run is claimed.

The unmodified previous Dev times out at the fixture's two-second guard on four
and twelve CPUs: the vfork parent cannot release the still-spinning child peer.
The new candidate completes, including a unique old epoll control. Seven final
samples (eight rounds per sample) give these median per-return means:

| CPUs | Ordinary Files | Unique owner epoll control |
| --- | ---: | ---: |
| 1 | 2025.9 µs | 4028.7 µs |
| 4 | 124.2 µs | 97.0 µs |
| 12 | 121.0 µs | 96.8 µs |

Native ARM Linux's same ELF reference is about 39 µs / 37 µs in the existing
6-CPU/8-GB test instance. This is not native macOS Linux-syscall equivalence or
a finite speedup ratio against a timed-out old run. The one-CPU coordinated
path remains about 2/4 ms, essentially unchanged from the old Dev.

Ordinary thread churn is **not faster**. Seven counterbalanced pairs for 128
two-thread groups include fork/clone, exit and final parent wait:

| CPUs | Previous Dev | Final Dev | Change in elapsed time |
| --- | ---: | ---: | ---: |
| 1 | 15.558 ms | 15.983 ms | +2.7% |
| 4 | 29.126 ms | 31.220 ms | +7.2% |
| 12 | 29.806 ms | 31.618 ms | +6.1% |

The added asynchronous cleanup and remaining final CPU/mm work still have a
cost. A stopped-cohort notification guard showed no consistent benefit and was
removed. Lazy weak roots only changed the first candidate by about 1%; no broad
CPU/Firefox gain is claimed. Timings exclude builds and the Linux instance.

The full goal remains open: final CPU/Memory custody, last/group/default-fatal
entry, source namespace victim termination, robust futex and excluded control
operations need their connected migration. The next step is completion custody,
not another service thread or a new source-admission workaround.

[Exact revisions, sources, controls and samples](../validation/source-completion-20261105.json).
Only Dev is installed after acceptance. Stable 1.5.3 and business services keep
their original binary; no Techo frontend is activated.

## 49. Source process exit and CPU/mm return — 2026-11-05

The final member, exit_group, default-fatal signal and synchronous fatal fault
now use original group/resources rather than forcing a whole-cohort return.
Actual namespace-init victim groups use the same termination path. Running and
parked peers return their own resources; cold peers are completed on their actual
owner. Source completion returns the full CPU and metrics immediately, allowing
parent wait to finish within the same cohort while unrelated CPUs continue.

Owner-local Rc destruction remains asynchronous work on the existing owner.
A cold peer sharing an executing mm uses a byte-only checkpoint of that mm and
restores its unchanged snapshot. An external guarded borrow keeps physical mm
custody after logical exit, independently of parent notification and PID reuse.
Dirty metadata remains available for its later writes. No new service thread,
polling bus, syscall RPC, ordinary guest barrier or unsafe block is added.

The remaining joined lifecycle work is structural creation/exec, explicit tracing
and non-ALLOW task policy, actual excluded owner controls, and legacy unadmitted
waiter adoption. This batch does not add robust-futex owner-death support.

[Validation and measured samples](../validation/process-completion-20261105.json)
record the passing checks and unchanged short-process throughput. Scheduling
switches fall by 23% at 12 CPUs, but structural clone/fork replaces the removed
exit stops and cohort rounds remain unchanged. Multicore vfork latency rises
slightly; no general speedup is claimed.


## 50. Task creation with creator-mm custody — 2026-11-05

Runtime `27b3eba9e6423345e1381f3318f5da08aeee19d8` removes the full-cohort return from all currently accepted
fork/clone/vfork combinations. The original owner still constructs the Rc task
and VFS graph under creator-mm custody; unrelated address spaces keep executing.
New children enter the live queue with the same resource capture as initial jobs.
Actual retired task/domain rows are removed, and existing CPU workers expand
participation on demand. This is scoped owner work, not fully source-thread
construction.

Birth shares the original family relationship transaction with namespace shutdown.
Current cgroup limits and membership commit together; accepted kill delivery
marks actual member identities before notification, including newborn binding.
Record-lock PID translation uses the live original namespace. TID copyout faults
follow Linux post-birth semantics. An external guarded memory borrower no longer
forces a creation-only full join: fork isolates the child while retaining the
parent snapshot and dirty journal. Child-first writable COW preparation preserves
parent physical storage/hardware mappings. Parked parents do not reserve busy
CPUs; warm continuations prefer an idle original executor without starving new
work. No new service thread, polling bus, unsafe block or ordinary-byte barrier
is added.

Apple and native ARM Linux each pass 318 process, 58 kernel and 162 memory
checks; full-feature VFS passes 510/509 respectively. Strict affected checks and
iPad target compilation pass. Real HVF/interpreter runs pass 25 lifecycle cases,
GPU/video teardown, 30 Go GC starts, eight isolated Docker rounds and H.264/Vulkan
import. No physical iPad run is claimed. Existing unchanged checks are reused.

Two untimed warmups per binary/CPU count, then seven counterbalanced pairs,
128 two-thread groups with ordinary files/private epoll controls:

| CPUs | Previous Dev | Candidate | Elapsed change |
| --- | ---: | ---: | ---: |
| 1 | 15.517 ms | 15.086 ms | -2.8% |
| 4 | 30.890 ms | 30.262 ms | -2.0% |
| 12 | 31.091 ms | 29.973 ms | -3.6% |

At 12 CPUs the same workload retains 838 context switches and 128 futex waits;
full-cohort rounds fall from 256 to one and clone(220) cohort returns from 255
to zero. It still performs 255 scoped mm checkpoints, so the construction and
communication costs have not disappeared. Plain vfork round-trip falls from
129.0 to 103.7 microseconds; the variant with an owner operation is 102.7 versus
107.2 microseconds. The unchanged ELF's earlier native ARM Linux reference is
reused and retained in the evidence; this is a process-lifecycle microbenchmark,
not a Firefox or general CPU score.

Separate opt-in HVF diagnostics located 381 unnecessary parent mapping retirements
in the first candidate; preparing child COW storage first removes all 381.
Diagnostics are not used for timing. Initial regressing and build-overlapped
trials were not used as acceptance results. Negative controls prove the retained
backing journal and parked-CPU regressions, rather than relying on code shape.

Exec, tracing, non-ALLOW policy, namespace administration and actual excluded
backing/native controls retain their documented boundaries. Stable 1.5.3 and
business environments are unchanged. Details and raw paired samples are in
[the validation evidence](../validation/task-creation-20261105.json).


## 51. Exec image replacement with creator-mm custody — 2026-11-05

Runtime `a116d834318d59ea51f3de119fc7e20ed44c58d8` moves execve/execveat into the existing creator-mm checkpoint.
The original Rc owner still prepares and commits the image, but unrelated mms
continue. Actual peer CPUs/statistics return before de-thread cleanup, old queue
members are removed, and the replacement enters the live queue before running.
Preparation failures preserve the original image, Files and background work.
Detached old memory retains its dirty journal and original last-borrow readiness,
independently of the new image and numeric PID. No new service thread, polling
bus, unsafe block or ordinary guest-memory barrier is added.

Nonleader exec preserves process visibility until its new task identity is
published. Cgroup/parent/wait relationships, process age and returned peer CPU
accounting follow the survivor. Old retry/open/AIO state, deadlines and temporary
wait-mask restoration are cleared while preserving the current mask. Accepted
cgroup termination targets the original process inbox/group even after its
captured leader retires; it cannot follow a reused TID. Pure custody-policy errors
finish on untraced continuous workers before mm quiescence, and allowed/deferred
policy tokens are consumed once. Tracing and lifecycle policy effects retain
their explicit boundary.

Apple and ARM Linux each pass 320 process and 59 kernel checks. Full-feature VFS
passes 510/509 respectively; the unchanged 162 memory results are reused. Strict
affected checks and iPad target compilation pass. The 39 real acceptance cases
include HVF/interpreter lifecycle and exec, FD/script/sealed-file execution,
30 Go GC starts, eight isolated Docker rounds, GPU/video teardown and H.264/Vulkan
import. No physical iPad run is claimed. One Linux socket case failed in an
earlier concurrent-build run; the final full suite and ten focused repetitions
passed. Its cause was not established, and no test was weakened.

Two untimed warmups, then seven counterbalanced pairs; each row measures 32
replacements/attempts with a busy sibling. The following are 12-CPU medians:

| Workload | Previous Dev | Candidate | Elapsed change |
| --- | ---: | ---: | ---: |
| execve | 87.228 ms | 22.775 ms | -73.9% |
| execveat with proc executable open | 159.794 ms | 74.945 ms | -53.1% |
| Exec from a nonleader thread | 86.043 ms | 31.297 ms | -63.6% |
| Failed exec attempts | 72.904 ms | 5.011 ms | -93.1% |

The 12-CPU execve and nonleader cases fall from 33 full-cohort rounds to one;
all 32 exec returns disappear. The execveat case falls from 65 rounds to 33:
its 32 openat calls on /proc/self/exe still refresh dynamic proc state through
the joined path. Failed attempts retain their actual proc-open/control boundary.
Single-CPU results remain within 0.8% of baseline; that caller path is unchanged.
These are process-lifecycle results, not Firefox or general CPU speedups.

The same-source OS controls are retained for comparison (32 operations):

| Workload | Native macOS control | Native ARM Linux, local filesystem |
| --- | ---: | ---: |
| execve | 111.713 ms | 0.672 ms |
| execveat with proc executable open | Not available | 2.637 ms |
| Exec from a nonleader thread | 92.836 ms | 2.598 ms |
| Failed exec attempts | 2.050 ms | 2.165 ms |

Darwin uses Mach-O/system libc, Linux uses the same static ELF as Trames, and the
Linux reference runs in the local six-vCPU ARM CI instance. They are separate OS
controls; filesystem location and scheduler conditions matter. An earlier Linux
shared-mount reference is retained as diagnostic data and is not used above.

The prior 128-group fork/exit workload changes -5.1%, +1.6% and -1.6% at 1/4/12
CPUs in the paired run. A noisy eight-round vfork sample prompted a longer
128-round confirmation: plain vfork is 103.3→104.2 microseconds at four CPUs and
107.8→110.1 at twelve; the owner-operation variant is 110.2→112.5 and
112.7→114.9. Both sample sets are retained. No broad throughput regression is
claimed from short latency samples or different absolute host conditions.

Remaining boundaries include original-owner scoped construction, dynamic proc
opens, tracing/security lifecycle effects, namespace administration and actual
excluded backing/native controls. Stable 1.5.3 and business environments are not
updated. Raw samples, hashes, negative controls and check scope are in
[the validation evidence](../validation/exec-custody-20261105.json).

To reproduce the fixture, compile `tests/fixtures/worker-exec.c` with
`musl-gcc -O2 -static -pthread` on ARM Linux. Import the resulting ELF as an
executable and run `worker-exec MODE 32`, choosing plain, at, thread or failed; use the same binary
and warmup/order for both runtime versions. The Darwin conditional fixture can
be compiled with `clang -O2 -pthread`; its `at` mode is not a Darwin reference.


## 52. Original scheduling and on-demand CPU slots — 2026-11-06

Runtime `e6d700c3147a4fb5a2a772dfcbb28eb636ac53fc` removes the implemented scheduling subsystem's owner
dependency as one batch. Affinity, nice and policy belong to each original IoTask;
queries and updates use the same ABI from the owner and source workers. Cross-task,
process-group and UID operations preserve original identities, namespaces,
permissions/capabilities and partial-update errors. Fork inherits values into an
independent task; exec keeps its actual original task. Credentials and structural
group/session changes publish at their real write boundary, so ordinary queries
do not rebuild every task's metadata.

An accepted mask change waits only for its original target's running lease to
depart. The caller releases its execution slot, retains completion across a
cohort handoff and resumes on existing readiness. Reaping/rekeying cannot redirect
that operation to a reused PID. Self migration and callbacks share the existing
group/queue synchronization, with no guest data barriers or algorithm/native locks.

The original CPU pool also had to change: previously only owner VM admission could
start a cold execution slot. Real twelve-CPU acceptance caught a thread remaining
ready after being pinned to CPU 11. Shared per-slot startup authority now lets the
live queue start that needed original executor directly. Startup/control locks do
not cover execution. Unneeded slots stay unstarted; no coordinator/service thread,
polling bus or eager all-CPU allocation is introduced. Actual startup failure,
cancel, return collection and teardown include these dynamic slots.

The complete validated matrix contains 56 HVF/interpreter runs (1/2/4/12 CPUs,
raw/ALLOW-filtered calls, cross-thread control, actual migration, fork/exec),
324 process cases plus 65 kernel cases on Apple and native ARM Linux, strict
checks, both iPad compile targets, 47 signed HVF cases and one pressure case.
Final exact-package regression also passes 30 Go-GC rounds, eight isolated Docker
rounds, 20 native GPU lifecycles and 2,000 video lifecycles. Existing unchanged
memory/VFS/driver acceptance is reused. No physical iPad or Firefox/Geekbench
performance claim is made by these scheduling fixtures.

The existing unimplemented RR interval and sched-attribute calls keep ENOSYS on
the source; realtime/deadline functionality is not newly advertised. Traced
calls and actual security lifecycle effects keep their original boundary.
Dynamic proc, namespace/credential administration, structural memory publication,
legacy nonmember scheduling and generic unhandled SVC/HLT dispatch remain listed
above. See the [shared contract](https://github.com/kb5000/trames-runtime/blob/e6d700c3147a4fb5a2a772dfcbb28eb636ac53fc/docs/architecture/coordination-boundaries.md#original-task-scheduling-and-on-demand-execution-slots--2026-11-06)
and [filtered evidence](../validation/worker-scheduling-20261106.json).

## 53. Original proc/sys views and namespace descriptors — 2026-11-06

Runtime `b26b2ad732663e6aedb665a3e3bc61b676b9746e` moves the dynamic process/resource view subsystem as
one batch, including pathname lookup, opened directories, magic links, ordinary
reopen, stat/getdents/readlink, dynamic bytes, control files, namespace descriptors
and asynchronous read consumers. A weak original PID binding survives nonleader
exec without retaining the task's Files table. Maps/environ keep the opened mm;
cmdline follows that PID's current mm; auxv keeps saved original bytes. Live image
bytes use the same guest memory model, including modified argv and setproctitle.

The original mount namespace graph is now shared. Worker-opened nsfs descriptors,
SCM_RIGHTS and bind aliases retain real setns authority, including mounts added
after opening. Metadata projections do not keep overlay layers or unrelated
mount uses alive. Self/older namespace binds fail and unshare omits namespace-bind
subtrees, matching the native Linux reference and avoiding ownership cycles.
Mount, unshare and setns task administration themselves retain their owner entry.

Remote cmdline/environ reads park on their own original page dependencies. Only
the target mm quiesces for first export or actual mapping changes; unrelated mms
continue. External execution Views wait on the original idle notification. The
same FileWork lifetime reaches scalar/vector reads and io_uring fixed descriptors,
registered buffers and retries. Last-mm retirement revokes virtual read authority
and releases pins. No new coordinator/resource thread or normal-byte barrier is
introduced. Mac host-managed residency uses its actual host callback.

Real acceptance found two integration omissions and fixed them in this batch:
solo/traced directory-relative queries still used a stale joined snapshot; and
worker O_TRUNC opens of sysctl/OOM/cgroup controls attempted regular-file truncate.
The latter broke runc's ping_group_range setup and is covered by control-open and
real Docker lifecycle regression. The ordinary owner/source paths now agree.

The shared implementation retains inherited compatibility limits for deleted
path-name history and anonymous endpoint O_PATH/pipe reopening. Those are not
reported as coordinator migrations. Legacy synthetic providers, unavailable host
accounting, tracing/policy effects, actual owner-only control graphs and general
unhandled dispatch still have explicit fallbacks. See the
[shared contract](https://github.com/kb5000/trames-runtime/blob/b26b2ad732663e6aedb665a3e3bc61b676b9746e/docs/architecture/coordination-boundaries.md#original-procsys-resources-and-complete-namespace-descriptors--2026-11-06).


## 54. Original mount transactions and root replacement — 2026-11-06

Runtime `b06c97a3e83e4ed81a08400cfe2991f224810dae` moves the supported mount topology subsystem as one
batch: ordinary filesystem mounts, remount, bind/recursive bind, move, existing
private/slave flags, ordinary/lazy unmount and pivot_root. Calling executors use
the original synchronized Table and Mount graph. Weak lookup-index observations
refresh only on actual index mutation; parent topology and flags remain original
resources, not copied authority. Ordinary pathname consumers see live changes
without an owner round trip. Cached weak entries do not keep detached mounts or
overlay layers alive.

A transaction mutex belongs to each original mount namespace. Contention retains
the imported operation in FileWork, subscribes to original readiness and releases
the guest execution slot. Slow native filesystem work uses the existing disk pool.
Cancellation has no uncommitted topology effects. No coordinator/resource service
thread or polling bus is introduced, and no ordinary guest-memory barrier is added.

Pivot_root now replaces matching root/cwd references in original fs_structs;
shared and independent contexts update directly. Open directory FDs and cwd below
the old root keep their original meaning. Fork/context creation and reference
replacement synchronize through a weak context registry, with amortized pruning
instead of a full scan on every fork. A detached/foreign directory FD remains
readable but cannot become a namespace root. The former owner-side process loop
and duplicate mount ABI dispatch are removed.

Concentrated acceptance covers 534 Apple VFS cases (533 on Linux), 237 Linux ABI
cases, 337 process cases, strict checks, both iPad compile targets, native ARM Linux
and HVF/interpreter guest fixtures, and the existing isolated eight-round Docker
lifecycle. Unchanged CPU/MMU/graphics acceptance is reused. No physical iPad
installation or global coordinator completion is claimed. Detailed results are
in the [validation record](../validation/worker-mount-20261106.json).

Unshare/setns still change task membership on their owner; the next whole subsystem
must update Files/fs, UTS, network/IPC/cgroup, pid-for-children, descriptor factories,
signal/readiness and proc consumers together. Credentials/capabilities, unsupported
newer mount APIs/propagation, excluded path controls, tracing/security and general
unhandled dispatch keep their documented boundaries. See the
[shared contract](https://github.com/kb5000/trames-runtime/blob/b06c97a3e83e4ed81a08400cfe2991f224810dae/docs/architecture/coordination-boundaries.md#original-mount-transactions-and-root-replacement--2026-11-06).


## 55. Original task namespace membership — 2026-11-06

Runtime `a5c3b6510e69c5c491411318695a6c4ef24157c7` moves supported unshare/setns and close_range as one complete
subsystem. Files/fs, UTS, mount, network/IPC/cgroup and pid-for-children membership
commit on the caller. Source resource refresh covers pathname/descriptor factories,
UTS/umask, native GPU image/event and audio notifications, exit ownership, proc
observers and future async submissions. Already accepted io_uring/AIO/native work
retains its original resources and wake target; changing namespaces does not
redirect an accepted operation to a reused FD or a new cwd.

Complete cold control OFDs now belong to original Files membership, using their
own synchronized objects. Ordinary Shared OFD I/O acquires no new lock. Private
projection caches no longer determine control lifetime. Fs membership counts actual
tasks separately from observers; source exit and exec retire it before cold owner
collection. The owner imports committed per-task bindings before service/custody,
without replaying effects. Failed transactions preserve old membership; waiting
setns retains the accepted namespace across close/reuse and can resume on another
existing worker. No new coordinator/service thread or polling bus is introduced.

Local validation passes 547 Apple VFS cases (546 on native Linux), 238 Linux ABI
cases and 339 process cases, with targeted retries after updating the old
close_range route test. Ten namespace transaction cases include serial rollback
and busy owner-to-worker resumption. The native Linux/HVF/interpreter matrix has
24 passing runs at 1/2/4/12 CPUs, raw and ALLOW-filtered. Strict checks, both iPad
compile targets, Mac product tests, Go-GC and eight isolated Docker lifecycle
rounds pass. Unchanged CPU/MMU/graphics acceptance is reused. Physical iPad testing
and a stable release are outside this batch.

Remaining boundaries include credentials/capabilities, original-owner task/image
construction, structural memory publication, tracing/security, excluded control
bodies and generic unhandled dispatch. Source-owned control lifetime alone does
not migrate every control body. See the [shared contract](https://github.com/kb5000/trames-runtime/blob/a5c3b6510e69c5c491411318695a6c4ef24157c7/docs/architecture/coordination-boundaries.md#original-task-namespace-membership-and-complete-filesfs-lifetime--2026-11-06)
and [validation record](../validation/worker-task-namespace-20261106.json).


## 56. Original credentials and permission consumers — 2026-11-06

Runtime `cb2b6209446be2b032c8c9ec759df0226e66ee0b` moves the supported credential subsystem as a whole.
UID/GID/res-ID/fs-ID/groups, capget/capset and credential prctl share one source and
serial ABI. A successful mutation publishes one original task credential record,
clears sensitive dumpability/parent-death state in order, and refreshes future
path/FD/socket/UTS/kernel-log/native/async and local sender/limit consumers. Existing
async requests retain their accepted authority. Cold owner collection imports
committed bindings rather than replaying changes. Ordinary identity getters keep
their existing fast path and ordinary file permissions take no new lock.

Remote signal, scheduler, prlimit/capget and proc observers use the original record.
Child wait and SIGCHLD identity follow the same record without reconstructing the
family graph. Shared-mm dumpability is visible immediately; fork/exec isolate it.
Native comparison also fixes invalid fs-ID queries, saved-ID permission rules,
setres-ID no-op behavior, group validation/sorting/count ordering and remote
capget error ordering.

Validation passes 547 Apple VFS cases (546 native Linux), 243 Linux ABI cases,
67 kernel unit/integration cases, 342 process cases, 24 Mac product cases, both
compile targets for iPad, and 24 native ARM Linux/HVF/interpreter fixture runs.
The complete legacy capability/exec fixture passes at 1/4 CPUs. Targeted source
tests prove immediate original observers before owner collection, future permission
changes, retained async authority and exactly one seccomp decision. Strict checks,
Go-GC and final Docker acceptance pass; the initial intermittent Docker cleanup
timeout remains explicitly recorded above. No physical iPad or stable deployment
is claimed. See [validation](../validation/worker-credentials-20261106.json) and the
[shared contract](https://github.com/kb5000/trames-runtime/blob/cb2b6209446be2b032c8c9ec759df0226e66ee0b/docs/architecture/coordination-boundaries.md#original-credentials-and-complete-permission-consumers--2026-11-06).

## 57. Original task/image lifecycle and mapping dependencies — 2026-11-06

This batch moves the complete supported construction/replacement subsystem and its
original-resource dependencies together. The current inventory and measurements
at the top of this document supersede stage-56 lifecycle boundaries. See the
[shared implementation audit](https://github.com/kb5000/trames-runtime/blob/4962de8d33d15b97e31d6ff61ba31ca099eec536/docs/architecture/coordination-boundaries.md#57-original-taskimage-lifecycle-and-mapping-dependencies--2026-11-06)
and [exact validation record](../validation/worker-task-lifecycle-20261106.json).


## 58. Original scoped-memory activation, completion and file layouts — 2026-11-06

The entire supported scoped-memory batch is integrated together, including original
proc-byte publication, cold/returned CPU completion and mapped-file mutations with
new aliases, io_uring, cancellation and external borrowers. The current inventory
supersedes the proc/cold-mm/layout boundaries recorded in earlier stages. See the
[shared implementation and defect analysis](https://github.com/kb5000/trames-runtime/blob/aeca5a59a6d53549ed39478081480fc4b4ed9f29/docs/architecture/coordination-boundaries.md#58-original-scoped-memory-activation-completion-and-file-layouts--2026-11-06)
and [exact validation record](../validation/worker-scoped-memory-20261106.json).


## 59. Original seccomp policy and complete lifecycle consumers — 2026-11-07

Supported strict/filter/prctl installation, TSYNC, entry results, source SIGSYS/
termination, proc visibility and fork/exec inheritance are integrated as one batch.
Accepted policy and guest bytes survive resource contention and cancellation;
late NNP publication refreshes a prepared exec image under original group custody.
Ordinary syscall entry remains lock-free. See the [shared contract](https://github.com/kb5000/trames-runtime/blob/38cdbba7a1967863a77fb0e432ec59cfa46e153c/docs/architecture/coordination-boundaries.md#59-original-seccomp-policy-tsync-and-lifecycle-consumers--2026-11-07)
and [validation record](../validation/worker-seccomp-20261107.json).


## 60. Original BPF programs and cgroup device-policy administration — 2026-11-07

The supported policy subsystem and its target-path dependencies now use original
shared resources on execution workers. This includes registry/FD publication,
attachment lifetime, atomic replacement and permission/device consumers; it does
not introduce host BPF execution or new policy features. See the
[shared implementation contract](https://github.com/kb5000/trames-runtime/blob/a1b8759dff09e2b8e1fa85607cde25e73b71544c/docs/architecture/coordination-boundaries.md#60-original-bpf-programs-and-cgroup-device-policy-administration--2026-11-07)
and [validation record](../validation/worker-bpf-20261107.json).


## 61. Generated dispatch recognition and source unimplemented completion — 2026-11-07

Unknown syscall numbers now finish without an owner rendezvous, with one original
policy decision and the same bounded diagnostic log. Recognition comes from the
actual dispatch arms; AIO's pre-dispatch family reuses its existing predicate.
Known resource/subcommand handlers remain conservative. See the
[shared contract](https://github.com/kb5000/trames-runtime/blob/c2d8643c924def8ff813b2d31de3dfc54cd64c26/docs/architecture/coordination-boundaries.md#61-generated-syscall-entry-recognition-and-unimplemented-completion--2026-11-07)
and [validation record](../validation/worker-dispatch-20261107.json).


## 62. Original execution-lease memory barriers — 2026-11-07

The supported barrier subsystem and its native/scoped execution dependencies now
run on existing workers, retaining original mm registration, accepted waits and
actual acknowledgement fences. See the [shared contract](https://github.com/kb5000/trames-runtime/blob/a4c1f20115757f72c6a023b81b9b13aa791db90f/docs/architecture/coordination-boundaries.md#62-original-execution-lease-memory-barriers--2026-11-07)
and [validation record](../validation/worker-membarrier-20261107.json).


## 63. Source entropy and original random-device controls — 2026-11-07

getrandom now executes through the original memory interface on the caller;
random-device control errors preserve the accepted OFD and policy without owner
service. Flags consume their Linux 32-bit argument. Device and UUID resource
consumers are validated with the complete subsystem. See the
[shared contract](https://github.com/kb5000/trames-runtime/blob/4d6926ba47d574250557314ea9335cb5ac4d8f1a/docs/architecture/coordination-boundaries.md#63-source-entropy-and-original-random-device-controls--2026-11-07)
and [validation record](../validation/worker-entropy-20261107.json).


## 64. FD/network compatibility entry retired — 2026-11-07

The current inventory and [runtime stage 64 audit](https://github.com/kb5000/trames-runtime/blob/4d6926ba47d574250557314ea9335cb5ac4d8f1a/docs/architecture/coordination-boundaries.md#64-retirement-of-the-fdnetwork-compatibility-rpc--2026-11-07)
record the closed production entry, original-console and complete-epoll resources,
source byte SIGPIPE, tests and remaining explicit boundaries. Validation is in
[fd-compatibility-closure-20261107.json](../validation/fd-compatibility-closure-20261107.json).


## 65. Unix ancillary lifetime and accepted-message closure — 2026-11-07

The complete rights lifetime, opaque io_uring retention and accepted-message retry
batch is integrated together. Full original OFDs survive close/reuse, import and
source-thread final release without an owner ledger. Stream/datagram/seqpacket
SIGPIPE behavior is checked against native Linux. Solo execution borrows its
original path context; ready calls avoid reconstructing a private VFS.
See the [shared implementation audit](https://github.com/kb5000/trames-runtime/blob/2c9228d2e55d588f637125699e72e88ef88d6a42/docs/architecture/coordination-boundaries.md#65-unix-ancillary-lifetime-and-accepted-message-closure--2026-11-07)
and [exact validation](../validation/unix-ancillary-closure-20261107.json).


### Stable 1.5.4 release gate — 2026-11-07

The release gate corrected two original-resource completion/observation defects:
joined idle detection omitted accepted ELF disk jobs after a cohort returned,
and a proc fdinfo read could reenter its own original offset transaction. Image
jobs now reuse their existing owner unpark token. File-position mutations retain
their original mutex/pending invariant and publish a relaxed committed scalar for
nonblocking metadata observation. No new coordinator/service thread, ordinary
syscall check or guest memory barrier is added. The original one/four-CPU systemd
and eight-cycle Docker deployment checks are part of the release record, alongside
actual artifact/source/signing provenance. The source-bound record is
[release-1.5.4.json](../validation/release-1.5.4.json).


## 66. Original io_uring Files and source-context closure — 2026-11-09

Registration and accepted-request import now retain complete original OFDs,
including opaque descriptions. The source resolver's temporary alias retains the
exact original object; it neither publishes a guest FD nor promotes metadata to
body authority. Close/reuse, registration removal, cancellation and final release
keep the original lifetime. All callbacks execute outside Files/ring metadata locks.
Solo acceptance and worker admission use the same request execution context.
Foreign CQ observation no longer asks the coordinator to recover another task.

Corrupt CQ indices return EINVAL on enter/resume while retaining accepted requests
and completion overflow for a corrected queue. Failed subscription setup gets one
source progress/rearm attempt: ready native requests can finish without a needless
owner transition. Still-unpumpable live requests retain the real capability
boundary. No new resource thread, busy message bus, memory fence or global lock is
added. Ordinary shared-FD lookup and native file/layout resources retain their
existing implementations.

The shared runtime's stage 66 audit records the exact remaining cases. Both full
runtime jobs and 384 process assertions per host pass; iPad device/simulator compile
checks pass. The 108 real backend cases cover mapped queues, IPv4/IPv6, CQ poll/epoll,
fixed/selected buffers, multishot pressure, close/reuse and cancellation/exit on
1/4/12 CPUs. Exact 707-entry async and 129-entry registered workflows remain on the
workers. Paired NOP/register/statx timing stays within about ±1.5%; no Firefox
speedup or zero-fallback claim is inferred. Native Linux uses the same static
benchmark but different filesystem backing for statx. The source-bound record is
[uring-originals-20261109.json](../validation/uring-originals-20261109.json).


## 67. Original io_uring completion dependencies — 2026-11-09

The unconsumed owner layout queue, its exported work/ID record and take/restore
API, and the owner signal-message queue are deleted together. Accepted file work
now retains a private resource dependency and original backing ticket. It waits
on that backing, resumes its original command without FD/path/input reimport,
and returns native work to the existing pool. Native completion retains the ticket
even after cancellation; unissued cancellation drops the dependency normally.
Standard signal fallback uses the original task actor's coalescing atomic flag,
not a queued owner/PID message. Production captured mailboxes are unchanged.

Tests cover queued/inline execution without a layout driver, cancellation before
and after native acceptance, close/reuse, real publication wakes, original signal
targeting, coalescing and fork noninheritance. The normal file actor no longer
embeds FileWork; dependency storage is allocated only during a real wait and kept
through retries. Existing mapping admission and native I/O synchronization remain.
No new thread, message bus or guest memory fence is introduced.

Both runtime CI jobs and 384 process assertions per host pass. Both iPad compile
targets and the 108 actual HVF/interpreter io_uring scenarios pass. Five alternating
pairs at 10,000 operations per worker keep single-CPU read/write/truncate between
-1.13% and +0.59% of the baseline; the measured multi-core cases have no regression.
This is a bounded whole-workflow result, not a browser/general speedup claim.
Native Linux uses the same static ELF in the six-vCPU CI environment, with different
filesystem backing. Exact short/long data and limitations are in
[uring-completion-20261109.json](../validation/uring-completion-20261109.json).


## 68. Original guest terminal/session/job-control closure — 2026-11-09

The former mixed private/snapshot state is replaced by one original process
PGID/SID and controlling-terminal binding. Threads share it; fork copies current
identity and binding in the existing family transaction, exec preserves it and
last-thread exit retires it. Existing family and PID/group indexes commit against
the originals. Source setpgid/setsid updates wait selection, proc and signal
consumers without copying fields into stopped CPUs. PID namespace references keep
PGID/SID identity after its leader retires. Native Linux verifies post-exec and
cross-session error precedence.

Supported PTY controls and read/write job-control signals now execute on the
caller using the same ABI as solo execution. Mounted and compatibility devpts,
automatic controlling opens and /dev/tty use original synchronized registries and
inodes. The old real-PTY Kernel body is deleted. The terminal retains its original
signal route, so input, resize, steal/detach, final master close and group exit do
not depend on an opening task's lifetime or a coordinator scan. Process-directed
signals use original shared queues and account for blocked siblings.

Two dependencies were found by whole-flow testing and closed together: a
background tty ioctl had been returning EAGAIN through its short-control route,
and solo write/ioctl waits lacked a CONT event source. Actual stops now retain the
accepted SVC/OFD and lazily subscribe/recheck original group readiness. FD reuse,
filters, cohort adoption, ignored CONT and one-slot execution are covered. No
ordinary guest memory barrier, periodic retry, global byte lock or service thread
is introduced. Read-only controls do not build an async envelope or read unrelated
job policy; self PGID/SID queries read the original IDs directly.

The four-thread mixed query workflow has 4,000 rounds/thread. Measured medians are:

| Mac HVF CPUs | Baseline | Final | Remaining coordination |
| --- | ---: | ---: | --- |
| 1 | 169.5 ms | 165.8 ms | No cohort in either version; approximately flat |
| 4 | 3266.3 ms | 62.2 ms | About 48,000 coordinator returns become 2 remaining cohort rounds |
| 12 | 3346.3 ms | 61.3 ms | About 49,000 coordinator returns become 2 remaining cohort rounds |

The same measured C query branch is about 43.3 ms on native macOS. The identical
Linux ELF is about 5.9 ms in the six-vCPU native ARM64 Linux CI environment. Darwin's
unused sigwaitinfo compatibility branch is adapted to sigwait; its measured query
branch is unchanged. These are bounded whole-workflow comparisons across different
host ABIs/compilers, not a Firefox or general application speedup. The initial
single-CPU regression, its readonly-path fix, sample order and reused unchanged
baseline are preserved in the [validation record](../validation/job-control-20261109.json).

Legacy GuestOutput terminal ID 0, host launch/control, explicit tracing and genuine
missing/excluded providers remain distinct boundaries. Existing unsupported tty
controls/termios modes and tty io_uring are not implemented by this batch. This
closes one complete supported subsystem; the overall coordinator goal stays active.


## 69. Original native mapping/entry checkpoint closure — 2026-11-09

The complete admitted native checkpoint path now uses the original CPU/job,
original task/mm/backing custody and existing execution slots. It covers live/
private native memory and Vulkan host allocation imports, with common entry
validation and native/device bodies. Resource waits release scope permits; accepted
GPU work retains its original Pending state, and unissued work preserves operands
through contention or cancellation. Bounded unstable-entry faults return through
source signal delivery. SVC/HLT classification prevents stale x8 from selecting an
unrelated structural operation.

Tests cover exact retirement, COW, entry instability for pure/GPU/video/audio,
original task/mm contention, cancellation and unrelated-mm progress with one slot.
The same static ELF runs private/shared file and GPU import workflows on HVF and
the interpreter at 1/4/12 CPUs. HVF confirms exact shared-map counts and no native
cohort returns. Five counterbalanced whole-workflow pairs for 32 imports change
4/12-CPU medians from 12.182/12.188 ms to 10.426/10.400 ms, with 32 → 0 native
cohort returns. Single CPU is 9.379 → 9.118 ms; file controls stay within -1.2%
to +0.5%. These bounded results do not imply a Firefox/general speedup.

The [validation record](../validation/native-checkpoint-20261109.json) includes
native macOS/Linux file controls, different backing/durability limits, all hashes,
source-bound CI and delta reuse. Actual affected bytes still need their original
checkpoint; host/UI lifecycle, tracing/capability-less embeddings and other generic
fallbacks remain. The overall coordinator-removal goal stays active.


## 70. Host process and native terminal lifecycle closure — 2026-11-09

Host exec transport prepares original stdio/OFD resources while CPUs run. Its
accepted launch is retained by the session, rather than an epoch or a numeric PID
message. Existing executors nonblockingly acquire original metadata custody,
construct an unpublished task and submit image/PATH loading to the existing disk
pool. Completion admits that original task and View into the live execution queue,
starting only required existing CPU slots. Disk/resource waits return the slot;
cohort replacement preserves accepted operands and image completion.

Client disconnect arbitrates with PID publication on one atomic lifetime. A
published PID also publishes its original group-control route; acquire/release is
limited to that actual host-event publication. Signals and final PTY master close
use original task/group resources. Native terminal creation/close no longer asks
either product adapter to stop all executors. No service/resource thread, polling
bus or ordinary guest memory fence is introduced.

Whole-flow validation found and closed two dependencies: source-born actors could
be misclassified as foreign Ready tasks during admission or deferred cold exit;
non-terminal TCGETS/other read-only tty probes still joined just to return ENOTTY.
The former retains source ownership; the latter completes against the original
OFD, preserving O_PATH/invalid-pointer ordering and descriptor reuse. Native Linux
confirms ENOTTY/EBADF and EIO when acquiring a slave after its master closes.

Tests hold image/task/mm custody, replace cohorts, disconnect before publication,
return late images, race route binding/master close and run busy unrelated jobs
with one executor. Real HVF workflows cover binary streams, literal arguments,
concurrent host exec, group signals, native terminal sizes/resize, early queued
open/close, surviving terminals and persistent restart. See the
[validation record](../validation/host-lifecycle-20261109.json) for bounded paired
measurements and native reference limits. Some whole-flow samples still contain
one extra execve(221) joined return; the generic fallback remains an audit target.
Product administration, legacy terminal
ID 0, tracing, excluded providers and the generic joined fallback remain; the
overall coordinator-removal goal stays active.


## 71. Cold process observation/source ownership closure — 2026-11-09

Stopped-cohort metrics alone initially suggested a remaining execve handoff.
Temporary stop-reason diagnostics instead captured a cold Ready task that had
already exited between the owner's liveness and queue-membership reads. Membership
now precedes the exit read, ordered by the existing queue mutex. No new barrier or
periodic check is required. Temporary diagnostic logging was removed before final
validation and integration.

The full observer dependency is closed across cold lookup/mutable lookup,
checkout, body enumeration, retention and removal. Each original body is admitted
before its current PID identity is validated. Stale snapshots skip removed,
rekeyed, replaced or consumed originals; a retention callback cannot delete a new
occupant based on its old snapshot. Waiting for a body never holds the original
directory. Accepted source Handles, Memory roots and guest result semantics remain
unchanged, and ordinary source execution has no extra validation/fence.

Four added regression tests cover all observation forms and original custody;
two stale-snapshot regressions fail on the baseline. Both complete local runtime
workflows, 401 process cases per host and dual iPad compilation pass. Eighty real
HVF terminal/exec workflows require exactly one shutdown cohort at 4/12 CPUs. Three
alternating single-CPU controls with one million getpid calls change median wall
time from 1340.7 to 1334.3 ms, approximately flat; native controls and cold-start
outliers are preserved in the
[validation record](../validation/process-observation-20261109.json).

This closes a source-ownership dependency, not every remaining joined execution
category. Actual host administration, legacy terminal ID 0, solo dispatch,
tracing/excluded providers and the final generic fallback remain audit targets.
Only Mac Dev is deployed, and the overall coordinator-removal goal stays active.


## 76. Original host filesystem command closure — 2026-11-09

Streaming imports, exports, conditional FileProvider saves and timezone commands
now submit accepted original-resource work to the existing disk pool. The adapter
retains FIFO completion without pausing unrelated CPUs. Original inode exports and
conditional edits share the existing mapped-backing driver; the original task's
pending namespace publication is observed without consuming/retiring its task
ownership. Anonymous staging avoids guest-visible temporary input that another
guest task could modify. Original filesystem/inode/byte transactions retain the
conditional content check through namespace replacement, including failed-manifest
rollback and retry with no spare staging quota. Ordinary guest byte access gains
no synchronization.

Explicit user pause remains intentional, while disk and backing completions
continue on the existing driver. Root teardown drains accepted work. Actual
cancellation/final backend teardown and genuinely capability-less embeddings keep
their own boundary. The owner-selected policy still submits FileProvider saves on
returning to Trames. All applicable local checks pass; only Mac Dev deployment is
included. See the [validation record](../validation/host-files-20261109.json).
The overall coordinator-removal goal remains active.

## 77. Original native-device namespace and Bridge resource closure — 2026-11-09

Supported native device opens no longer depend on terminal-only capability or a
joined VFS body. `/dev/dri`, virtual block nodes/aliases, kmsg and Bridge resolve
through the same shared pseudo-node cache and original Files/credentials. Cached
pseudo-node identity is resolved before the missing-identity escalation. Video
device availability is an original shared atomic flag rather than a frozen
worker projection. Guest device nodes never open a host device.

Stateless/error-only device bytes call the existing device implementation.
Sequential empty reads preserve the original VFS early completion, while
positioned calls retain their original device/offset errors. The regression
matrix compares scalar, vector and positioned read/write and seek against the
original Linux ABI with read-only/write-only/read-write/O_PATH opens, empty
buffers and invalid offsets. Existing driver functionality is unchanged.

Bridge DirectFd snapshots use a weak original connection. Accepted DirectFd/OFD
work retains that connection; closing and reusing its descriptor number cannot
redirect a pending reply. The original broker lock protects framing, bounded
input/output, pending requests, cancellation and consent-based host replies.
A fixed notification array collects changed original connections under that lock
and wakes subscribers only after unlocking. No new lock serializes unrelated
native calls, and notifications allocate no per-frame registry.

Readiness observation never expires requests or emits callbacks while the run
queue holds its metadata lock. Due requests are lazily readable; actual body/host
operations materialize the same timeout response. Only future deadlines are
armed, preventing a past-deadline retry loop under output/CQ backpressure.
Dynamic external readiness keeps a host-reply waiter alive. Draining the original
output wakes blocked writers; read/write epochs follow that original stream.
Passive snapshots and subscriptions cannot retain the final connection.

Conformance covers source SQE opens, pending Bridge CQ completion after FD
close/reuse, reentrant host-reply wake after unlock, backpressure, timeout,
cancellation, original permissions and live video availability. A real guest
opens the Bridge and blocks on a read while peers execute. The 4-CPU, 12-CPU and
4-CPU/one-execution-slot cases require one cohort, zero returned syscalls and
zero coordinator service requests.

The two io_uring maintenance escalation sites are deliberately unchanged.
Genuinely absent typed/wait capabilities, excluded resources/bodies, actual host
administration, final dispatch and initial/legacy orchestration remain open.
This batch does not claim the overall coordinator-removal goal is complete.

Both local runtime workflows pass all seven steps. The complete process suite
passes 408 tests with two intentionally ignored on macOS and ARM64 Linux; all
nine FFI tests pass on both hosts. Strict core/FFI lint and iPad device/simulator
SDK checks pass. macOS and Linux reuse unchanged results after the two obsolete
device-deferral test expectations were corrected; a later source-comment/doc
correction changes no executable behavior. Product hardware and installed
acceptance are recorded by the consumer's
[validation record](https://github.com/kb5000/trames-ipad/blob/main/docs/validation/native-devices-20261109.json).

The expiry follow-up marks only future native requests as external producers.
Accepted wait retries capture their existing resource deadline before the final
level check, so crossing a lazy expiry before parking cannot discard that wake.
io_uring enter, CQ polling and background pumps retain pre-pump deadlines as
well as newly established ones. Due requests do not create a repeated past-time
wake after the worker has observed them. Ready ordinary byte calls gain no extra
resource scan, and ordinary guest memory access is unchanged.

## 78. Original live host pause/resume — 2026-11-09

A live embedding pause no longer requests a stopped control hook. The iPad FFI
adapter continues original host queue, notification and transfer processing while
paused. Actual cancellation still requests the original stopped hook and reports
its existing result; pause cannot swallow cancellation or errors. Legacy adapters
may still request their explicitly stopped callback.

One original relaxed atomic pause flag is retained by the cohort and every
original/new execution domain. The existing queue mutex publishes suspension and
resume. On pause, existing domain interrupt registries mark their original active
leases and kick their backend. An entry before publication is found and kicked;
an entry after the original domain lock sees the published flag and cannot run.
Interpreter/native loops use their existing lease pending bit; no ordinary byte
check, native-call lock, new fence or per-instruction atomic is added. A native
body already entered may finish before the pause settles.

Guest dispatch stops without destroying jobs, CPU/translation state, Views, wait
subscriptions or accepted I/O. New tasks can be admitted while paused and inherit
the same original flag. Actual mapped-backing checkpoints and accepted scoped
work can progress on existing workers; their optional guest instruction batches
start with a pending lease and cannot execute. Host pause does not alter guest
STOP/CONT, cgroup freezing or the mapping-pause bit. Resume cannot clear those
independent causes. Suspended cohorts are not classified as idle completion.

Resume wakes the original queue and starts only CPU slots required by its actual
ready affinity/load. A resource wake during pause cannot start a new execution
slot. Cancellation wakes sleeping dispatchers and uses normal teardown/drain.
There is no resource service, new pool, polling message bus or coordinator RPC.

Conformance covers three pause/resume cycles with 4/12 logical CPUs and a single
physical execution slot, original job/View identity, pre-entry/late interrupt
binding, new-task/cold-affinity admission, cancellation, idle pipe reads retained
through FD close/reuse and a wake arriving while paused. Mapped host exports still
complete on 4/12 CPUs while guest execution is suspended. The signed Mac hardware
fixture uses native CLONE_THREAD peers and raw aligned ARM64 counter stores, so
its pause check does not depend on guest syscalls. The baseline runtime keeps
executing after the live pause request and fails to settle.

Actual cancel/deadline/shutdown, final teardown, missing/excluded resources,
known-resource/HLT/fault fallback and initial/legacy/solo orchestration remain.
No overall coordinator-removal completion or browser/throughput speedup is claimed.

Both complete local runtime workflows pass. macOS and ARM64 Linux each pass 412
process tests (two intentionally ignored) and all 10 FFI tests. Strict core/FFI
lint and iPad device/simulator SDK compiles pass. A removed unnecessary `mut` in a
FFI test binding changes no production behavior; only affected FFI checks are
repeated. Product hardware and installed evidence are recorded in the consumer's
[validation record](https://github.com/kb5000/trames-ipad/blob/main/docs/validation/host-pause-20261109.json).

A focused regression reproduced an ignored paused Idle notification preventing
idle completion after resume. Resume now clears that original notification latch
before waking dispatchers. The full affected process suite and strict core lint
are rerun on both hosts; unchanged CPU/Linux/VFS/runtime checks are reused.

## 79. Original system statistics and live execution accounting — 2026-11-09

Supported sysinfo no longer returns a running cohort to read owner-private
statistics. One typed capability reuses the original proc Source; source/solo
share the complete AArch64 formatter and preflight. Original live mm reports
are deduplicated, untouched VA consumes no resident budget, and host-managed
sysinfo directly samples the original host callback without scanning irrelevant
mm residency. Original task membership supplies process count.

Task and physical execution-slot time include the active assignment through one
relaxed atomic word. Original actors own begin/finish; cold observations cannot
overwrite a running total. Existing timestamps cover source, native/scoped and
solo execution without a new clock or atomic on each ordinary instruction,
syscall or native fast call. Idle derives from configured CPU capacity minus
live execution time. The original shared load sample advances only for aggregate
observers; drive-loop load/idle projection copies are deleted.

This preserves approximate wall-assignment CPU accounting. Five-second load
sampling uses current runnable state when observed and does not reconstruct
unobserved history. No new statistics service, resource thread, polling bus,
whole-cohort refresh, ordinary memory check or fence is introduced.

Conformance covers full destination/error ordering, ABI padding, missing
providers, changing host reports, shared-mm residency, unused mappings, task
lifetime, live CPU time/migration, load decay and exactly one seccomp decision.
4/12 logical CPUs and a single physical slot retain one cohort and zero returned
sysinfo/service work. Both complete local runtime workflows, 418 process tests
per host, all 10 FFI tests, strict lint and both iPad SDK compile targets pass.
Consumer hardware and installed acceptance are recorded in the
[validation record](../validation/system-statistics-20261109.json).

This closes the connected supported statistics subsystem. Actual cancellation,
deadline/shutdown/final teardown, missing/excluded resources, other final
SVC/HLT/fault dispatch and initial/legacy/solo orchestration remain. Only Mac Dev
is deployed, and the overall coordinator-removal goal stays active.

## 80. Original guest power control and group completion — 2026-11-09

Supported halt/poweroff no longer returns to the owner syscall handler before
requesting process exit. Common policy preserves live SYS_BOOT permission,
int-width magic/command validation, unsupported commands and nested namespace
rejection. One original root group is captured at root publication and retained
across later task/exec admission, retirement and numeric replacement. Source
execution requests root and caller group exit on those original controls.

Existing execution leases, dormant wait subscriptions, first-status arbitration,
original Files/namespace/native cleanup and final CPU return are reused. Accepted
native/disk work retains its actual drain requirements. The caller never executes
the instruction following successful reboot(142). This is a guest-only operation;
no host power API is invoked.

Errors complete locally before absent capabilities can defer. Seccomp ALLOW/ERRNO
is evaluated once before group effects. Valid legacy embeddings without original
shutdown/exit capabilities retain their explicit before-effects boundary. No
coordinator/service thread, polling bus, ordinary memory check/fence or new
per-instruction check is introduced.

The full shared fixture exercises halt/poweroff from root, root nonleader,
independent child and child nonleader at 4/12 CPUs and with one physical slot.
Additional coverage checks route lifetime/replacement, changing live credentials,
error ordering, namespace isolation, capability absence and seccomp. A signed
native CLONE_THREAD fixture reproduces reboot(142)'s joined return on the exact
previous runtime. Source-bound final checks are in the
[validation record](../validation/guest-power-20261109.json).

Both complete local runtime workflows pass, with 422 process cases (two
intentionally ignored) and all 10 FFI cases on each host, strict lint and both
iPad SDK compiles. Mac product checks, 52 signed HVF and five signed product
hardware tests pass. The installed Dev also passes disposable Alpine BusyBox
poweroff on 1/4/12 CPUs, plus init, native host terminals/exec and tracing.
Single-CPU execution keeps its existing solo caller path; those results do not
claim it has moved to worker orchestration.

Actual final teardown still drains CPU actors; this removes the preceding guest
syscall handoff. Host cancellation/deadline/shutdown/final teardown, other
missing/excluded capabilities, other final SVC/HLT/fault dispatch and
initial/legacy/solo orchestration remain. Only Mac Dev is updated, and the overall
coordinator-removal goal remains active.
