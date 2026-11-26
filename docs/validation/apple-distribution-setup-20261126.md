# Apple distribution setup — 2026-11-26

## Registered resources

| Resource | Value |
| --- | --- |
| Developer team | `T73WQBSU73` |
| Main application | `org.trames.Trames` |
| File Provider extension | `org.trames.Trames.FileProvider` |
| Shared App Group | `group.org.trames.Trames` |
| App Store Connect name | Trames |
| Apple ID | `6839642990` |
| SKU | `trames-ipad` |
| Primary language | English (U.S.) |

Both App IDs have the shared App Group assigned. Increased Memory Limit is
enabled for the main application. These settings were saved and checked in
the Apple Developer portal.

The [App Store Connect record](https://appstoreconnect.apple.com/apps/6839642990/distribution)
was created and its identity checked on the App Information page. The draft
iOS version is `1.5.4`, with manual release selected and saved. Its status is
Prepare for Submission; no build has been uploaded or submitted for review.

## Local configuration and checks

`apps/ios/project.yml` now selects team `T73WQBSU73` with automatic signing.
The previous generated Xcode project was backed up locally under
`target/apple-signing-20261126/previous-Trames.xcodeproj` before regeneration
with the existing XcodeGen 2.46.0 binary.

Checks completed:

- Generated project inspection verified Debug and Release team settings,
  bundle identifiers, and entitlement file references for both targets.
- Xcode 27.2 `xcodebuild -showBuildSettings` succeeded for the Release main
  application and File Provider targets using the existing
  `target/ios-swift-packages` cache. Both resolve the registered team,
  identifiers, automatic signing, and shared App Group. The selected device
  SDK is iOS 27.2.
- `python3 scripts/prepare-third-party-notices.py --check` passed after
  refreshing the project configuration fingerprint.
- `git diff --check` passed.

Build-setting output is retained locally in `target/apple-signing-20261126/`.
An initial sandboxed settings query could not write Xcode caches; a query
using the existing package cache outside the sandbox completed successfully.

This check does not validate a signed archive, certificate availability,
provisioning profiles, or App Store upload acceptance. Legal drafts, store
descriptions, screenshots, privacy and compliance declarations still require
their respective release preparation and review.
