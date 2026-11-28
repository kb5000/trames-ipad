# iPad optimization and complete workspace integration — 2026-11-28

Runtime pin: `8fee46855ab3e092f27fbc654b9f0d9ef413275a` (includes shared optimization commit `03d21f4`).
CLI pin: `bd22dbf1b6f9875d2298633467bf81f1516b6f5b`. The owner additionally requested committing all existing
workspace changes, then building and installing on the paired iPad.

## Optimization scope

Private-store incremental WAL/checkpoints and the bounded pathname cache apply to
Apple and Linux hosts. Computation follows existing defaults: the iPad interpreter
can use the approved compression/checksum/SHA/AES adapters; Mac HVF and Linux KVM
keep original hardware execution by default. No optimization setting or UI was
added. The pre-existing product UI/audio/share/browser changes were included on
the owner's subsequent all-workspace instruction.

Details, APIs, exact approved-library hashes and limitations are in the
[runtime validation](../../runtime/docs/validation/storage-compute-cache-20261128.md).
Compression stream APIs and EVP/TLS contexts retain original execution. Selection
occurs at cold session creation, with original fallback for unapproved images.

## Completed checks

- Shared runtime storage: Apple 87 passing; Linux hostfs 90 passing, plus the
  pathname-cache semantic test and the earlier affected host-edit/overlay checks.
- Computation unit tests: 33 passing on Apple and Linux. Three approved library
  sets × one/two CPUs × disabled/enabled: all 12 interpreter cases passed.
  Per enabled case, 96 worker dispatches included 92 native completions and four
  intentional empty-update fallbacks. Native Linux original fallback and exact
  restoration were checked for the Debian and Alpine derivatives.
- Rust-generated 41-record WAL was replayed by the actual Swift reader, including
  torn tail, stale checkpoint, changed revision/cache signature, preserved hard
  links and complete-frame corruption. No writer lock was acquired by the reader.
- Complete current `trames-distro` unit suite: 47 passed on Apple and 47 on Linux.
- Complete current `trames-ffi` unit suite: 35 passed on Linux. Import/edit tests
  count actual WAL bytes and retain their original I/O ownership/cancellation checks.
- Current iPad Rust cross-check and Release archive build: passed for
  `aarch64-apple-ios`, using Xcode 27.2 and the existing validated MoltenVK archive.
  The unchanged guest CLI/graphics/video payloads passed their source/artifact checks.
- Clean Mac and Linux product compile checks passed, including the final shared
  login/ALSA/network accessor integration. Architecture checks passed; iPad's
  dependency closure contains the interpreter and no JIT.
- Dependency notices: 139 reviewed Rust packages / 117 original notice texts.
  All 14 changed/new Python scripts passed syntax compilation.
- The existing unsigned-app finalizer now selects the File Provider by extension
  point, so the new Share extension does not break packaging. Actual built plist
  fixtures cover both extensions, a legacy single extension, version mismatch and
  signed-extension rejection. Existing upstream audit/license evidence is retained.

The small-file fixture reduced manifest publication bytes by approximately 93.3%
(about 137 KB instead of 2.06 MB for 200 files). This excludes object data and fsync
latency. No apt/apk/git/curl/code-server end-to-end timing claim is made.

## Device installation

The current complete Xcode Release build succeeded with development signing.
`codesign --verify --deep --strict` passed with normal keychain access. Installed
in place on the connected physical iPad Air 11-inch M3 using `devicectl`:

- App: `org.trames.Trames`, Trames 1.5.4 (49), stable variant.
- File Provider and Share extensions: both version 49, matching app group.
- Installation succeeded, launch succeeded, and the Trames process was observed.
- The existing app was not uninstalled; its app identity/data container was retained.

Installation/startup is confirmed; no physical-device apt or code-server benchmark
or full guest-workload acceptance is claimed here. Raw build/install logs and the
signed bundle remain under local temporary/build artifact directories.
