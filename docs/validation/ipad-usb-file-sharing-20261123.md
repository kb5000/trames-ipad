# iPad USB file sharing — 2026-11-23

## Behavior

The containing app now explicitly enables `UIFileSharingEnabled` and
`LSSupportsOpeningDocumentsInPlace`. Apple Devices/Finder can expose its
`Documents` transfer directory. The Files panel offers a document picker starting
in that directory and an export button that publishes a completed Linux file
under its original filename. Import uses the existing coordinated, streamed
staging and Linux transfer path. Existing shared files are not overwritten.
Ordinary Files exports also retain the guest filename instead of a random UUID.

The Linux store remains in its existing App Group/private storage; the existing
File Provider remains the separate way to browse Linux files in the iPad Files
app. USB sharing does not expose the raw environment store.

## Validation

- macOS: all three `USBFileSharingTests` passed against the production helper:
  Unicode filename and complete content, existing-file preservation, and dangling
  symlink preservation without following its target.
- Both localization catalogs have the same 281 keys and matching `%@` arguments.
- Signed iPhoneOS Release build passed; the built app contains both sharing keys
  set to true. `codesign --verify --deep --strict` passed.
- Physical iPad Air 11-inch (M3), iPadOS 27.0.1: three USB tests and two
  `LocalizationTests` passed, with zero failures/skips/runtime warnings.
  Result: `target/usb-sharing-check/ipad-final.xcresult`.
- Development candidate: Trames Dev 1.5.4 (66), using its existing bundle
  identifier and App Group. Installation preserves the existing app container.
- Clean signed candidate: `target/usb-sharing-check/Trames-USB-Dev66.ipa`.
  XCTest components were removed from the distribution copy before re-signing.
  Every Mach-O section's content matches the tested app; code-signature metadata
  differs. IPA sharing flags and test-component exclusion were checked.

## Build scope and limits

XcodeGen is installed locally at `target/tools/xcodegen/bin/xcodegen` (2.46.0),
outside the current PATH. The development project was regenerated from the
current canonical `apps/ios/project.yml`, with local signing and build-number
overrides. The Release test target included only the five selected tests and
enabled testability; unrelated DEBUG-only probes were not compiled into it.

This change does not modify Runtime or its repository pin. The development build
reuses the existing iOS static library rather than incorporating the workspace's
independent, unfinished Runtime changes. The reused archive SHA-256 is
`b2e17ad10e08e3c7dd087a13d808ca287221722dd0bc25efa012fe9c9e93552f`.
Its recorded hash and paired native Vulkan inputs were checked, and all guest
graphics/video/Bionic/glibc payload checks remained enabled. Only the generated
local development spec omitted the current-Rust-source freshness comparison;
the canonical build spec and its checks were not changed. This is a frontend
candidate using a pre-existing runtime binary, not a rebuild of current Runtime.

Initial physical execution waited for the owner to unlock the iPad. An
independent installation during that wait invalidated the waiting runner's
LaunchServices record; a fresh test-without-building run passed after unlock.

Windows Apple Devices visibility and a transfer through that Windows client
remain to be checked on the owner's computer. The Mac used for development did
not show the iPad in Finder's device sidebar during this session.
