# Trames iPad App (`apps/ios`)

This directory contains the SwiftUI iPadOS application, the `TramesKit` Swift package, the `TramesFileProvider` extension, and the XcodeGen project definition (`project.yml`).

---

## Overview

- **Target Platform**: iPadOS 17.0+ (`arm64`)
- **Bundle Identifiers**:
  - Main App: `org.trames.Trames`
  - File Provider Extension: `org.trames.Trames.FileProvider`
- **App Group**: `group.org.trames.Trames`
- **Entitlements**:
  - App Group (`group.org.trames.Trames`) — shared strictly between the app and `TramesFileProvider`
  - Increased Memory Limit (`com.apple.developer.kernel.increased-memory-limit`) — optional headroom on supported iPads; falls back gracefully where unavailable
  - **No JIT or dynamic-codesigning entitlements**

### Key Subsystems
- **Workspace & Terminal**: SwiftUI multi-tab workspace built on SwiftTerm (Unicode, VT sequences, true color, touch selection, hardware keyboard shortcuts, editable extra-key row, floating windows, Picture-in-Picture preview, and Light/Dark appearance).
- **Local Linux Environments**: Runs unmodified ARM64 Linux distributions (Alpine by default, plus Debian 13, experimental Android, or custom `tar`/`tar.gz` rootfs) via the embedded Rust runtime (`libtrames_ffi.a`) using the non-JIT shared AArch64 interpreter. One local environment runs at a time; entering the background pauses the interpreter (unless the user explicitly keeps a window active in Picture in Picture) and returning to the foreground resumes it.
- **Files & USB Integration**:
  - **File Provider Extension (`TramesFileProvider`)**: Exposes installed environments in the iPadOS **Files** app with deferred, inode/SHA-256-verified writeback for in-place edits of regular files (see [`docs/guides/rootfs-files.md`](../../docs/guides/rootfs-files.md)).
  - **In-App Files Panel (`+ → Files`) & USB File Sharing**: Streamed import/export without silently overwriting existing files; `UIFileSharingEnabled` exposes the app's `Documents` transfer directory in Finder / Apple Devices.
- **Wayland & Metal Graphics**: Optional built-in Wayland compositor, Xwayland bridge, and Vulkan-to-Metal presentation (via static MoltenVK) displayed inside floating or docked iPadOS workspace windows.
- **Note on `TramesRemote`**: `TramesKit` contains a separate `TramesRemote` module (SwiftNIO SSH / Citadel), which is not currently exposed in the main `Trames` app UI.

---

## Project Layout

| Path | Description |
| :--- | :--- |
| `project.yml` | XcodeGen specification generating `Trames.xcodeproj` |
| `Trames/` | Main SwiftUI app sources, Metal shaders, localization (`en`, `zh-Hans`), and staged guest resource folders |
| `TramesKit/` | Shared Swift package (environment configuration, FFI session wrapper, SSH/SFTP, bridge, scoped files) |
| `TramesFileProvider/` | iPadOS File Provider app extension |
| `TramesFileSystem/` | Shared read-only rootfs manifest/object reader and deferred-edit journal used by both targets |
| `TramesTests/` & `TramesUITests/` | Unit, FFI, and simulator UI test suites |

---

## Building from Source

### Prerequisites
- macOS with **Xcode 16+** and the iOS SDK
- [XcodeGen](https://github.com/yonaskolb/XcodeGen) (`brew install xcodegen`)
- Rust nightly toolchain (configured via [`rust-toolchain.toml`](../../rust-toolchain.toml))
- Python 3

### Build Commands

Run from the repository root:

```sh
# Build Rust staticlib + generate Trames.xcodeproj + build iPad Simulator app
bash scripts/build-ios.sh simulator

# Build unsigned arm64 device app
bash scripts/build-ios.sh device
```

To package a built device `.app` into an unsigned `.ipa`:

```sh
python3 scripts/package-ios.py \
  apps/ios/build/device/Build/Products/Release-iphoneos/Trames.app \
  apps/ios/build/sideload/Trames-unsigned.ipa
```

> **Guest Payloads**: Generated guest directories (`GuestMusl`, `GuestGlibc`, `GuestVulkan`, `GuestVideo`, `GuestBionic`) are validated during the Xcode pre-build script (`scripts/check-ios-guest.py`). On a fresh clone, prepare them following the staging steps in [`.github/workflows/ios.yml`](../../.github/workflows/ios.yml).

---

## Signing & Running on a Physical iPad

### Developer Signing in Xcode
1. Build the device Rust static library first (`bash scripts/build-ios.sh device` or the Rust target step).
2. Open the generated `apps/ios/Trames.xcodeproj` in Xcode.
3. For **both** the `Trames` and `TramesFileProvider` targets:
   - Select your Development Team.
   - Set a unique bundle identifier (keeping `TramesFileProvider` prefixed by the main app's bundle ID, e.g., `<your.bundle.id>.FileProvider`).
   - Register and set a matching App Group (`TRAMES_APP_GROUP`) so File Provider shared storage works.
4. Select your connected iPad and build & run.

If the signing setup does not include a matching App Group container, Trames automatically falls back to its app-private store (in-app Files import/export and USB transfer remain available, while the iPadOS Files app provider will be unavailable).

### Sideloading Unsigned IPAs
Unsigned release `.ipa` files are published on [GitHub Releases](https://github.com/project-trames/trames-ipad/releases/latest) and can be signed locally with tools such as AltStore or Sideloadly:

1. Download the `.ipa` directly from GitHub Releases.
2. Sign and install it to your iPad (iPadOS 17+). No JIT activation is required.
3. To preserve existing Linux environments across updates, always re-sign with the **same Apple ID and effective bundle identifier**.
4. Deleting the app removes its local Linux data—export important files before uninstalling.

---

## Testing

- **Simulator & Unit Tests**: Run `TramesTests` and `TramesUITests` from Xcode or via CI (`.github/workflows/ios.yml`).
- **Large-File Streaming Memory Probes** *(opt-in)*: Set `TRAMES_FILEPROVIDER_LARGE_MIB=256` when running `RootfsEditStoreTests` and `BridgeTests.testLargeFilesStagingUsesBoundedMemory` to verify bounded chunked-copy RSS under large File Provider edits.
