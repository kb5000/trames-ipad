# iPad Debian iproute2 dump delivery fix — 2026-11-24

The owner reported `ip a` failing with `Cannot send dump request: Invalid
argument` in an iPad Debian + systemd environment. This is an older runtime's
NETLINK_ROUTE datagram framing behavior, independent of systemd configuration.

## Diagnosis

The unchanged Debian iproute2 6.15.0 binary reproduced the error in an existing,
older Mac Debian process. `ip link` succeeded; address and route queries failed.
A standard Python netlink probe succeeded with an exact 24-byte RTM_GETADDR
request but failed with EINVAL when 4, 16, or 128 bytes were appended.

[Original iproute2 6.15.0 libnetlink.c](https://github.com/iproute2/iproute2/blob/v6.15.0/lib/libnetlink.c#L293)
sets nlmsg_len to the meaningful request length, then sends sizeof(req),
including a 128-byte buffer, for address and route dumps. The older runtime
rejected the remaining bytes as an invalid additional netlink message.

Runtime `9246f9ed486792438e44520b910f0ebd8d0af3ba` already fixed route netlink
trailing-byte handling on November 22. iPad frontend builds 66/67 deliberately
reused an older native runtime archive (documented in the USB/deletion
validation records), so installing those frontend updates did not deliver that
runtime fix. No new production syscall patch was required here.

## Build and verification

- Built from clean iPad source `e93f62b9` and its committed Runtime pin
  `42e58346041335df8ef995119abc7f6cfab555ca`. Independent unfinished memory and
  storage changes in the working directories were excluded.
- Fresh iPhoneOS Release Rust archive built and passed source/native Vulkan
  provenance checks; SHA-256:
  `57a21632ca5979f0520f748b975e3628658bbaec6e8f8d92cdf1b17b2d3e7965`.
  Unchanged verified guest resource payloads were reused.
- Existing `route_dump_ignores_trailing_bytes_and_preserves_sequence_and_peek`
  regression passed on the host.
- Physical iPad Air 11-inch (M3), iPadOS 27.0.1: **1 test passed, zero failures
  or skips**. An isolated disposable guest ran the original Debian iproute2
  6.15.0 executable and original glibc/libraries, exported read-only from Debian.
  `ip a`, `ip -4 a`, `ip -6 a`, `ip route`, and `ip -6 route` all succeeded;
  loopback IPv4 and IPv6 addresses were present. This uses the same iPad-linked
  runtime; it does not claim a new systemd boot suite or inspection of the
  owner's live Debian shell.
- The first device test had successful command exits but false text assertions
  because ANSI colors appeared inside addresses. Stripping ANSI display codes
  from the assertion text produced the passing run; no guest command or runtime
  behavior was changed to obtain that pass.
- Dev and Stable **1.5.4 (68)** were signed, deep/strict verified, and installed
  under the existing identifiers and App Groups. The Dev installation copy
  excludes injected XCTest plugins. Installed-app inventory confirms both 68;
  Stable launched successfully. Existing environments were not reset or rebuilt.
- The Mac Debian production environment was not restarted. Temporary diagnostic
  files created under its /tmp were removed after read-only inspection.

Local evidence: `target/ip-a-netlink-20261124/`, including
`ipad-iproute-final.xcresult`, `ipad-iproute-final-summary.json`,
`netlink-regression.log`, `build-record.json`, original executable/library
hashes, build logs, install receipts, and the scoped local XCTest source.
This is an installed-runtime refresh and scoped query regression, not a full
runtime compatibility or graphics acceptance claim.
