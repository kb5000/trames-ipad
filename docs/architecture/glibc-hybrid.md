# Debian 13 glibc native calls

The cached interpreter now has a cold-selected glibc adapter for Debian 13
ARM64 `2.41-12+deb13u4`. It covers the same 44 memory, string and numeric
interfaces as the Bionic batch, plus GNU locale, C23 and internal aliases:
68 exported names in total. This is a small precompiled addition to the exact
installed libc, selected before execution. It does not replace the loader,
allocator or TLS implementation, use LD_PRELOAD, or patch a running process.

## Pinned input and replacement

The current Debian rootfs supplies these files:

| File | SHA-256 |
| --- | --- |
| `/usr/lib/aarch64-linux-gnu/libc.so.6` | `70fa4c4cf57eabeb96097cce47582d02379c4cd1de3db3e98ade4dd7089a1b13` |
| `/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1` | `1b882eddacd494b060413a8aa4f0a4754ee3f84775e68cecbf8a86092b59cca6` |

The original libc build ID is `5f3926d9963ff284b8173bf3601bbb00c3abad16`.
[Debian's libc6 package](https://packages.debian.org/trixie/libc6) and the
[official source archive directory](https://deb.debian.org/debian/pool/main/g/glibc/)
are the upstream inputs. `accelerator/glibc/layout.json` records their digests,
the matched debug symbols, exact instruction prefixes and symbol aliases.
`scripts/prepare-glibc-layout.py` is the maintainer-only layout generator.

`scripts/build-glibc-hybrid.py` builds about 15 KiB of ARM64 code and a reversible
recipe. Clang and Rust's ELF linker are sufficient; an ordinary resource build
needs no downloaded rootfs or full glibc build. A new RX segment carries the
wrappers and relocated original entry prefixes. Original dynamic symbols,
GNU symbol versions, relocations, TLS data, initializers and entry point remain
unchanged. IFUNC symbol types remain intact; selected resolvers return a fixed
wrapper address, without per-call architecture or capability detection.

The shared distro installer validates both the libc and loader before staging
changes. Installation uses the existing exclusive environment-store ownership
and per-inode atomic rename. Stored provenance supports idempotent reopen,
recipe upgrades/downgrades, disabling acceleration, and recovery from interrupted
installation. It is not a whole-rootfs transaction. Unsupported custom images
remain original in the app; explicit CLI installation reports an unsupported
pair. A changed or missing loader restores a recognized owned derivative.

The iPad app bundles `GuestGlibc` and supplies it for Debian and custom Linux
profiles. A recognized pair is selected before the first process starts.
Turning pure acceleration off restores the original library at cold startup.
An apt libc upgrade is not overwritten with an older package: its new, unknown
bytes remain original until a matching adapter is supplied. Statically linked
programs retain their own embedded libc.

Mac's HVF path keeps running the original ARM64 libc directly on hardware, with
`pure_accelerator: false`. It does not need these interpreter native-call exits.
There is no product restart, rootfs package upgrade or device installation as
part of this change.

## Function coverage

| Family | Interfaces |
| --- | --- |
| Memory | `memcpy`, `memmove`, `mempcpy`, `memset`, `memcmp`, `memchr`, `memrchr`, `memmem` |
| Strings | `strlen`, `strnlen`, `strcmp`, `strncmp`, `strchr`, `strrchr`, `strchrnul`, `strcpy`, `stpcpy`, `strncpy`, `stpncpy`, `strcat`, `strncat`, `strlcpy`, `strlcat` |
| Search and case | `strstr`, `strcasestr`, `strspn`, `strcspn`, `strpbrk`, `strcasecmp`, `strncasecmp`, their `_l` case aliases, `strsep`, `strtok_r` |
| Integer | `strtol`, `strtoll`, `strtoul`, `strtoull`, `strtoimax`, `strtoumax`, `atoi`, `atol`, `atoll`; applicable `_l`, `__isoc23_*` and `__*_internal` aliases |
| Floating | `strtod`, `strtof`, `atof`; applicable `_l` and `__*_internal` aliases |

Pure profile `0x10001`, IDs 1–30, reuses the shared memory/string implementation.
Glibc profile `0x1000a` has two entries: integer conversion and floating
conversion. The complete static library and runtime must be deployed together.
The original 32-entry pure bitmap and older musl/Bionic protocols are unchanged.

Native work runs on the executing worker through the same guest-memory access
model as the CPU. There are no native-specific permission rules, global locks,
service/coordinator threads, byte quotas, or guest continuation loops. Actual
mapping boundaries remain inside the shared transport. Unsupported backing
selects original code before native writes. Short operations stay local;
tokenizer cursor state remains caller-owned. Original ptmalloc, qsort/bsearch,
I/O, wide-character functions, locale objects and their locks remain guest code.

## Equivalent implementations and original-code selection

Glibc integer semantics differ from musl/Bionic. The Rust scanner implements
the pinned glibc C-locale grammar and saturation/errno behavior, including C23
binary prefixes. `atoi`/`atol`/`atoll` retain glibc's `strtol` behavior rather than
musl's wrapping conversion. Invalid bases and grouped conversions use the
original implementation.

Finite floating conversion uses the existing pinned gdtoa **strtodg** algorithm
with the target IEEE precision, not Darwin strtod or musl's intermediate
precision parser. Glibc's decimal/octal/hex NaN payload grammar and AArch64
tininess-before-rounding rule are implemented explicitly. Exception flags come
from gdtoa's exact rounding metadata, independently of incidental host FP flags.
Guest end pointer, errno and FPSR are committed by the guest wrapper. Host FP
control, status and errno are restored; scratch and gdtoa caches are call-owned.

These are semantic equivalents, not compiled copies of glibc's finite floating
algorithm. Source references, GNU LGPL text and provenance are retained under
`runtime/crates/trames-native/upstream/glibc-2.41/`; gdtoa's original notices and
source remain alongside the shared native parser. To replace an equivalent
later, keep the same result/end/errno/FPSR contract and rerun the differential
suite against this exact binary pair.

Only C/POSIX/C.UTF-8 byte semantics take these locale-sensitive native paths.
Other locales, nonzero guest FPCR, and grouped conversions keep the original
guest algorithm. Real Turkish case mapping and French decimal punctuation are
covered by regression tests. There is no native locale or allocator shadow.

## Reproduction and measured scope

On Linux with AArch64 GCC, Clang, Rust and qemu-user:

```sh
python3 scripts/prepare-glibc.py
python3 scripts/build-glibc-fixtures.py
cargo build -p trames-cli --release --locked
qemu-aarch64 -cpu max -L target/glibc-hybrid/sysroot \
  -E LOCPATH="$PWD/target/glibc-hybrid/sysroot/usr/lib/locale" \
  target/glibc-hybrid/fixtures/glibc-probe > target/glibc-hybrid/oracle.stdout
python3 tests/integration/glibc.py --oracle target/glibc-hybrid/oracle.stdout
```

For explicit isolated CLI selection, pass `--glibc-bundle DIR` together with
`--accelerator pure` to `trames-run`. `DIR` contains the manifest and
`glibc-nativecall.bin` produced by the builder. Pass the same bundle with
`--accelerator off` to restore original bytes. `--libc HOST_LIBC` on the builder
also exports complete original/derived libraries for validation; the ordinary
app resource contains only the adapter and recipe, not Debian's libc binary.

[The validation record](../validation/glibc-hybrid-20261118.json) includes the
exact binary/fixture hashes and three-round medians. Original interpreter,
native adapter and independent QEMU produce identical functional output,
including 4,096 randomized decimal pairs over all four rounding modes, NaNs,
errno, end pointers and exception flags. The matrix covers 1/4 CPUs, memory/disk
stores, GNU symbol versions, real locales, preload interposition and cold
upgrade/restore/error recovery. It is a regression suite, not exhaustive proof
for every libc input or an application benchmark.

On the ARM64 Mac cached-interpreter host, 64 KiB substring/set searches improve
by hundreds of times, `strtoll` about 2.6x and `strtod` about 3.4x versus the
same original guest library. These large string ratios remove interpreter work;
they are not comparisons with host-native libc. Short calls have wrapper cost;
4 KiB memcpy/memset do not improve in this fixture, and large memcpy is near
parity. Original ptmalloc throughput is approximately unchanged. No end-to-end
Android/Firefox or physical-iPad performance claim is made from these results.
