# Complete hybrid musl — standard libc and loader paths

Version 0.10.0/build 11 integrates the initial [NativeCall pure profile](accelerator-v1.md)
into a complete musl 1.2.5 shared ELF built from the SHA-256-pinned upstream archive.
The guest uses `/lib/ld-musl-aarch64.so.1` and Alpine's existing
`/lib/libc.musl-aarch64.so.1` symlink. No LD_PRELOAD or separately named library
is required. The complete public export set is retained; private original
functions and NativeCall stubs are hidden. The loader still identifies itself
as musl 1.2.5.

## Startup and implementation

Original musl source supplies the allocator, TLS, pthreads, loader, I/O, math,
locale and all other libc functions. The 0.10.0 release wraps
memcpy/memmove/memset/memcmp/strlen. The development follow-up also wraps
memchr/memrchr/strnlen/strcmp/strncmp, with a function capability bitmap and
[direct guest-page access](../roadmap/musl-optimization.md).
Original fallback source bytes stay intact: selected object files
are privately renamed through a compilation header. The original Arm assembly
remains the memcpy/memset fallback.

Two small checked startup hooks call a hidden initializer: static __init_libc
after locating auxv, and dynamic __dls2b after self-relocation and before TLS
copying. The initializer scans raw auxv for private profile key 0x54520001.
It performs no libc calls, allocations, constructors, syscalls or errno/TLS
access. Early dynamic stage-2 calls see the zero-initialized flag and execute
the original implementation. The wrappers/initializer have no stack protector.

Known-size operations below 128 bytes stay in the original guest implementation.
Enabled strlen first scans a bounded 64-byte aligned-word prefix using musl's
HASZERO algorithm; an early NUL returns without host entry. Aligned words cannot
cross a page. Larger work uses one NativeCall v1 entry for the complete request,
without an input-size ceiling or guest continuation loop. Actual mapping boundaries
remain inside the native memory transport; unsupported backing uses typed original-code fallback. Disabled Trames sessions and ordinary Linux/QEMU
use original routines and never execute the virtual HLT.

This is a complete hybrid libc/loader. Current development adapts 35 public
memory/string/plain-integer functions using 32 pure entry IDs (bitmap `0xffffffff`),
and adds [resumable sort/search callbacks](native-callbacks.md).
The extended set includes substring/set/case search, BSD bounded copies,
`mempcpy`, locale aliases and `atoi`/`atol`/`atoll`. `strsep`/`strtok_r` reuse
the accelerated spans through their unchanged original algorithms. Stateful
native TLS/I/O and allocator cold paths remain future work requiring exact layout
and ownership contracts. The [original-source mallocng profile](native-allocator.md)
implements existing-group allocation and atomic fast free while preserving the
guest heap, original lock boundaries and source-reuse principle.
Statically linked existing programs retain their
embedded libc; replacing a shared library cannot accelerate them automatically.

## Installation and iPad integration

`trames-distro::install_musl` bounds input, validates a loadable AArch64 ET_DYN ELF
and the standard libc alias, and compares the current loader before writing.
New bytes are copied/synced into a sibling inode, then one atomic namespace
rename replaces the loader. Invalid input and staging-path conflicts preserve
the existing library. Identical bytes are a no-op. User files need no reset.
The library is replaced before guest startup, under the existing exclusive
environment-store ownership. This is library replacement, not a whole-rootfs
transaction or snapshot recovery mechanism.

CLI activation is explicit:

```sh
bash scripts/build-musl-hybrid.sh
target/release/trames-run --storage /path/to/environment \
  --rootfs target/alpine-download/alpine-minirootfs-3.22.6-aarch64.tar.gz \
  --musl-libc target/musl-hybrid/libc.so --accelerator pure --guest /bin/ash
# On later launches omit --rootfs. Use --accelerator off for original fallback.
```

Embedding ABI 3 accepts optional `musl_libc` and existing `pure_accelerator`
JSON options. Without these options its existing behavior is retained.
The iPad application bundles the full Linux ELF as a resource and supplies both
options for installation/session startup. Xcode does not link or execute that
guest ELF. Apple CI first cross-builds it in a Linux prerequisite job, transfers
the same-workflow artifact, verifies its manifest digest, and copies it into
GuestMusl resources. The real Apple DNS/HTTPS simulator probe uses this path.

Each application launch ensures the bundled library is installed, preserving
the standard alias and user data. An apk musl upgrade can replace the loader
during a session; already loaded processes keep their mapped image and ordinary
new libc code remains supported. On the next application launch the supplied
bundled version is installed again. No APK database claims are rewritten.

## License, source and evidence

Trames adapters and integration are GPL-3.0-or-later; original musl/Arm MIT notices
are preserved. The library artifact includes the complete original source archive,
all adaptation/build files, GPL text, original COPYRIGHT, checked startup hashes,
compiler identity and the resulting ELF SHA-256. Pinned-source re-extraction on
each build avoids mixing stale patched/configured trees. [Validation and measured
limits](../validation/musl-hybrid.md) describe the actual tested scope.

[Native floating conversion](native-numeric.md) adds a separate gated module over checked guest bytes; atof and locale aliases retain their original forwarding code.

[Native integer parsing and string search/copy](native-stdlib.md) extends the
function bitmap and uses a separately negotiated integer module, preserving
original fallback on earlier hosts.

[Coherent backing and bulk memory](bulk-native.md) adds gated strcat/strncat,
resident shared page borrows and bounded NEON fragment scans/comparisons.

## Complete requests (2026-11-18)

Bionic and musl share the same search/set/case implementation and the CPU's
existing memory admission. Copy/fill coalesce adjacent private mappings only
within one retained host allocation under the existing sole-executor proof.
Raw pointers do not manufacture cross-page Rust slices. Shared pages keep the
CPU's existing guards; no function-wide lock, coordinator or service thread is added.
Virtual overlap uses directional native moves. Non-virtual physical aliases
select original execution before effects, rather than snapshotting arbitrary
input into a host allocation. Unrelated aliases do not disable ordinary copies.

Search-pattern and floating-input scratch grows only from reached valid pages;
allocation failure selects the original routine before guest writes. There is
no 1 MiB memory/string ceiling or 64 KiB pattern/numeric ceiling. The separate
resumable callback-sort state budget remains unchanged.

New musl floating wrappers use pure ID 32 with a 24-byte result including FPSR.
They preserve host FP/errno state, propagate guest exception flags, and select
original code for nonzero guest FPCR. Existing numeric module `0x10003` keeps
its 16-byte ABI for old libraries. See the paired measurement record in
[complete-request validation](../validation/libc-complete-20261118.json).

Three-round medians against the frozen previous product on the same ARM64 Mac
showed musl 2 MiB memcpy at about 106 us (40x faster), strlen at 52 us (106x),
and the 64 KiB one-character strspn at 1.9 us (510x). The large ratios include
removing the previous interpreter fallback; they are not comparisons against
native host libc. Bionic strspn improved about 12x and long strlen about 1.4x.
Bionic 2 MiB overlapping memmove remains a regression in this fixture: about
93 us versus 63 us previously. Complete-request dispatch does not remove all
page transport/metadata cost, and these results do not claim a universal speedup.
