# Coherent backing access and complete bulk NativeCall

Pure calls run on the executing guest worker, with joined memory only for
backing cases that require it. No JIT, pointer-as-guest-address cast, worker-side Rc/VFS transfer,
or retained host slice is introduced.

PageBacking has optional object-safe scoped read/write callbacks. A supported
backing invokes exactly once while retaining its borrow; unsupported backing
returns false without effects and Memory retains its staged contract. Memory
checks the reached page, permission and live EOF before borrowing. Shared
anonymous backing updates its revision/global invalidation before mutable access;
file backing additionally marks dirty and updates inode mtime. Normal msync,
fd access, aliases, truncation and parallel snapshot refresh keep using those hooks.
Private file pages still detach before writes and retain live EOF checks.
Already detached private pages can copy directly; live backed moves retain a
complete bounded source snapshot to preserve non-virtual aliases.

Bulk memory and string operations process the complete request in one entry,
without an input-size policy limit. Copy/fill preflight complete ranges; scans
stop at reached mismatch/NUL. Actual page and host-allocation boundaries remain
internal transport details. Sole-executor private mappings can coalesce as raw
pointers; shared pages retain the existing CPU guards. Unknown backing I/O
failures may retain earlier effects, consistent with the existing contract.
Non-virtual aliases select original code before effects.

AArch64 baseline NEON searches and compares 16-byte checked fragments with
scalar exact-byte resolution and bounded tails. Unaligned loads stay inside the
slice. Non-AArch64 development hosts retain the existing word routines. Original
Arm memcpy/memset assembly remains in use, with musl/Arm notices retained.

A separate one-shot output preflight permits resident shared, undetached private
file and write-only outputs without relaxing the private/readable contract for
resumable sort/callback state. Unknown I/O backing remains original fallback.
Integer/floating conversion result frames use the same one-shot preflight.

Pure IDs 18/19 add strcat/strncat, and IDs 20–32 add shared search/set/case,
bounded BSD copies, plain integer and FP conversion. The feature bitmap is
`0xffffffff`; IDs 1–19, activation key and profile stay unchanged. Wrappers
keep short work local and check their individual feature bit. Original upstream
functions remain privately renamed; old hosts and ordinary Linux use them.
Concatenation scans destination/source, preflights output, preserves tagged
return pointers and writes one final NUL without strncpy padding. Zero bounds
never access the source. Unknown I/O output retains original fallback before
writes. Pattern/numeric scratch grows from valid reached pages without the old
64 KiB ceiling. Callback-sort state retains its separate 64 KiB budget.
Per-function fallback counters distinguish these admission decisions.

[Validation and occasional measurement](../validation/bulk-native.md).
