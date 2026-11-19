# Shared core API integration — 2026-11-19

Mac and iPad adopt runtime `5f750931671f68996eb7c01b7bba4d9a4f643ab5` on their
existing main development lines. The [shared validation record](https://github.com/kb5000/trames-runtime/blob/5f750931671f68996eb7c01b7bba4d9a4f643ab5/docs/validation/gvisor-core-apis-20261119.md)
contains the implementation scope, original gVisor binary identities, equivalent
fixture baseline, full/final-group provenance and remaining compatibility gaps.

| Local gate | Result |
|---|---|
| Shared runtime, macOS | 8/8 workflow steps passed |
| Shared runtime, native ARM64 Linux | 8/8 workflow steps passed |
| iPad interpreter | 500 passed, 3 existing manual benchmark ignores |
| iPad FFI | 24 passed |
| iPad interpreter Clippy | Passed with warnings denied |
| iOS aarch64 core build check | Passed |
| Product execution boundaries and layout | Passed; 3 layout tests passed |
| Mac product workflow | 14/14 steps passed |

The Mac workflow includes native terminal/session and window-scene acceptance,
manager controls, clipboard/input-method fixtures, both channel builds and both
unnotarized packaging checks. Its only workflow adaptation redirects package
output into the ignored task evidence directory; no check is skipped. Final
local bundles are restored to embedded Vulkan linkage after the workflow's
external-linkage packaging fixtures.

The selected original gVisor inventory improves from 2,211 to 2,312 passing
cases with zero old-pass regressions. The 101 additional passes exclude the
82 cases recovered by preparing the baseline's missing ACL tmpfs fixture.
Remaining failures, skips, timeouts and absent results stay recorded.

This is a source/pin integration, not a product deployment or release. Installed
Mac Stable/Dev apps and running environments are unchanged. The physical iPad
is not contacted, captured, woken or installed. Raw evidence and packages remain
under the ignored `target/gvisor-core-apis-20261119` directory. The native Linux
validation VM is stopped after its final checks. No new controlled throughput
or Firefox-latency result is claimed.
