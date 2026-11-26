# Dependency notices and source-package verification

Date: 2026-11-26.

## Changes verified

- English dependency records, original license texts, selected license branches,
  source checksums and offline notice generation/checking in all five repositories.
- Complete main-lock Rust closures: iPad 132, Runtime 132, Mac 132, Linux 136,
  CLI 12. Runtime's standalone vendored-backend lockfile brings its combined
  reviewed closure to 171; other auxiliary locks add no distinct coordinates.
- iPad Wayland/protocol/keymap notices regenerated against the current Cargo.lock.
- Rust notices added as an iPad resource and copied into the Mac bundle before signing.
- Mac distributions export product, Runtime and CLI sources and record the CLI revision.

## Executed checks

1. `python3 scripts/prepare-third-party-notices.py --check` passed in each repository.
   Every referenced original license text hash was checked, including vendored
   source notices. Documentation links resolved.
2. The Wayland generator regenerated 522,675 bytes of original notices and its
   `--check` passed. Apple's Python 3.9 was used with the audit's temporary tomli
   compatibility module; the Wayland generator itself requires Python 3.11+.
3. Negative checks rejected a changed dependency version and modified original
   license text. The three previously missing copyright holders were found in
   each relevant new notice bundle.
4. The real IPA packager rejected a fixture missing Rust notices, then included
   the exact notice bytes when supplied. This used a synthetic arm64/iOS Mach-O
   header fixture; it did not execute an app or claim device verification.
5. The real Mac packager rejected a locally compiled, ad-hoc-signed fixture app
   without notices. After adding notices and signing again, it produced a valid
   ZIP containing all three source archives, matching `BUILD.json` CLI revision,
   and exact notices both inside the app and at the distribution root. Each
   source archive was opened and checked for Cargo.toml and LICENSE.
6. Python source parsing, shell syntax and `git diff --check` passed for the changes.

No runtime algorithms, guest binaries or application executables were changed.
The packaging fixtures are verification artifacts, not product releases. Existing
published Release assets were not replaced by this documentation/package update.
