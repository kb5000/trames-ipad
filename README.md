<p align="center">
  <img src="resources/branding/trames-app-icon-ios.png" alt="Trames icon" width="108">
</p>

<h1 align="center">Trames iPad</h1>

<p align="center">
  <strong>A local Linux terminal and graphical workspace for iPadOS.</strong>
</p>

<p align="center">
  Run unmodified ARM64 Linux binaries inside the standard iPadOS sandbox using a shared Rust userspace kernel and a non-JIT AArch64 interpreter.<br>
  No jailbreak, JIT entitlement, private API, or hardware virtualization required.
</p>

<p align="center">
  <a href="https://github.com/project-trames/trames-ipad/releases/latest"><img src="https://img.shields.io/github/v/release/project-trames/trames-ipad?style=flat-square&amp;color=f06b45" alt="Latest release"></a>
  <a href="https://github.com/project-trames/trames-ipad/actions/workflows/ios.yml"><img src="https://img.shields.io/github/actions/workflow/status/project-trames/trames-ipad/ios.yml?branch=main&amp;style=flat-square&amp;label=iPad%20CI" alt="iPad CI"></a>
  <a href="https://github.com/project-trames/trames-ipad/actions/workflows/linux.yml"><img src="https://img.shields.io/github/actions/workflow/status/project-trames/trames-ipad/linux.yml?branch=main&amp;style=flat-square&amp;label=Linux%20CI" alt="Linux CI"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0--or--later-7c5269?style=flat-square" alt="GPL-3.0-or-later"></a>
</p>

<p align="center">
  <a href="#what-trames-does">Features</a> ·
  <a href="#architecture">Architecture</a> ·
  <a href="#getting-trames">Installation</a> ·
  <a href="#building-from-source">Building</a> ·
  <a href="#documentation">Docs</a> ·
  <a href="CONTRIBUTING.md">Contributing</a>
</p>

---

## What Trames Does

Trames turns your iPad into a local ARM64 Linux terminal and graphical workspace inside the standard iPadOS sandbox:

- **Multiple Local Linux Environments**: Start immediately with **Alpine Linux**, add **Debian 13** (shell or systemd), or import a custom ARM64 `tar`/`tar.gz` rootfs—each with persistent storage, independent package state, and a per-environment TOML profile.
- **Standard Developer Toolchains**: Run unmodified ARM64 Linux tools including Git, OpenSSH, Vim/Neovim, Python, Node.js, Clang, GCC, Make, and CMake. Works offline once an environment is initialized.
- **Multi-Tab Terminal & Floating Workspace**: SwiftTerm-based terminal with Unicode, true color, touch selection, hardware keyboard support, customizable extra keys, detachable floating windows, and Picture-in-Picture window preview.
- **iPadOS Files & USB Integration**: Browse and edit regular files inside your Linux environments directly from the iPadOS **Files** app via the bundled File Provider extension, or transfer files through the in-app **Files** panel (`+ → Files`) and USB File Sharing.
- **Optional Wayland & Vulkan Graphics**: Built-in Wayland compositor and Xwayland bridge that renders graphical Linux windows directly into iPadOS tabs or floating windows via Metal (MoltenVK), with hardware keyboard, pointer, and text clipboard support.
- **Local-First & Private**: No user accounts, no ads, and no developer analytics or automatic log uploaders in production builds.

### Realistic Performance & Scope Expectations

Standard iPadOS app sandboxes do not allow JIT compilation. Trames executes guest ARM64 instructions using an optimized software interpreter (`trames-interpreter`) with decoded-block caching, soft-TLB, and signed static native helpers for selected hot paths. Interactive shells, editing, scripting, SSH, and everyday CLI tools run smoothly, while heavy compilation and compute-bound workloads run slower than bare-metal hardware.

Trames is a userspace compatibility layer inside the iPadOS app sandbox, not a hardware VM. Guest `root` (UID 0) is strictly virtual and grants no host privileges.

---

## Architecture

Trames separates **Linux OS semantics**, **instruction execution**, and **host presentation** across shared repositories:

```text
┌──────────────────────────────────────────────────────────┐
│                 iPadOS Frontend (SwiftUI)                │
│     Multi-Tab Terminal · Files · PiP · Metal Wayland     │
└────────────────────────────┬─────────────────────────────┘
                             │ Versioned C ABI (trames-ffi)
┌────────────────────────────┴─────────────────────────────┐
│                trames-runtime (submodule)                │
│   Pure-Rust Userspace Linux Kernel (Syscalls, VFS, Net,  │
│      Tasks, Memory, Wayland/Vulkan, Shared Interpreter)  │
└────────────────────────────┬─────────────────────────────┘
                             │ Execution Contract
┌────────────────────────────┴─────────────────────────────┐
│              trames-cpu (Non-JIT Interpreter)            │
│    AArch64 software interpreter + static native helpers  │
└──────────────────────────────────────────────────────────┘
```

| Component | Location | Responsibility |
| :--- | :--- | :--- |
| **iPadOS App & File Provider** | [`apps/ios/`](apps/ios/) | SwiftUI workspace, SwiftTerm terminal, native SSH/SFTP, Metal Wayland presenter, and `TramesFileProvider` extension. |
| **iPad Runtime Facade & FFI** | [`crates/`](crates/) | Thin composition crates (`trames-core`, `trames-cpu`, `trames-compute`, `trames-ffi`, `trames-cli`) binding the shared runtime without linking JIT. |
| **Shared Userspace Kernel** | [`runtime/`](runtime/) ([`trames-runtime`](https://github.com/project-trames/trames-runtime)) | Platform-neutral Linux kernel semantics shared by `trames-ipad`, `trames-mac` (HVF), and `trames-linux` (KVM). |
| **Shared Host CLI Engine** | [`cli/`](cli/) ([`trames-cli`](https://github.com/project-trames/trames-cli)) | Argument parsing, raw PTY lifecycle, signal forwarding, and `TRMEXEC1`/`TRAMES02` wire protocols. |
| **Guest Accelerators** | [`accelerator/`](accelerator/) | Hybrid musl/glibc/Vulkan/Xwayland/Android guest adapters bridging selected hot paths and graphics calls. |

---

## Getting Trames

### Requirements
- iPad running **iPadOS 17.0 or later** (Apple Silicon M-series or A12X/A12Z+ recommended).

### App Store & TestFlight (In Preparation)
App Store and TestFlight distribution is currently being prepared (see [`docs/legal/app-store-drafts/`](docs/legal/app-store-drafts/) for the licensing, privacy, and review drafts).

### Sideloading Pre-Built IPAs
Until the App Store release is live, unsigned `.ipa` builds are published on [GitHub Releases](https://github.com/project-trames/trames-ipad/releases/latest):

1. Download the unsigned `.ipa` from the latest release.
2. Sign and install it with Xcode, AltStore, Sideloadly, or your preferred signing tool.
   - To enable **Files** app integration, both the main app and `TramesFileProvider` must be signed with the same team and matching App Group (`group.org.trames.Trames` or your team's equivalent).
3. Launch Trames and tap **+ → Linux Environments → New Environment** to initialize an Alpine or Debian environment.

---

## Building from Source

### Prerequisites
- macOS with **Xcode 16+** and the iOS SDK
- [XcodeGen](https://github.com/yonaskolb/XcodeGen) (`brew install xcodegen`)
- Rust nightly toolchain (pinned automatically via [`rust-toolchain.toml`](rust-toolchain.toml))
- Python 3

### Clone & Build

```sh
git clone --recurse-submodules https://github.com/project-trames/trames-ipad.git
cd trames-ipad

# Build for iPad Simulator
bash scripts/build-ios.sh simulator

# Build unsigned bundle for physical iPadOS devices
bash scripts/build-ios.sh device
```

> **Note on guest payloads**: Pre-built guest libraries and graphics/video payloads (`GuestMusl`, `GuestGlibc`, `GuestVulkan`, `GuestVideo`, `GuestBionic`) are generated artifacts not committed to Git. See [`.github/workflows/ios.yml`](.github/workflows/ios.yml) and [`apps/ios/README.md`](apps/ios/README.md) for guest staging and Xcode signing instructions.

---

## Documentation

### User Guides
- [Linux Environments & Configuration](docs/guides/ipad-environments.md) — Alpine, Debian, custom rootfs, TOML profiles, and appearance
- [Browsing & Editing Files in iPadOS Files](docs/guides/rootfs-files.md) — File Provider behavior, deferred writeback, and USB sharing
- [Wayland & Graphics Windows](docs/guides/wayland.md) — Running graphical Linux and Vulkan/EGL apps in Trames
- [iPad App Build & Signing Guide](apps/ios/README.md) — XcodeGen, entitlements, File Provider setup, and sideloading

### Architecture & Security
- [Non-JIT Interpreter Design](docs/architecture/nonjit.md) · [Execution Backends](docs/architecture/execution-backends.md)
- [Region Memory Model](docs/architecture/region-memory.md) · [Hybrid Libc](docs/architecture/musl-hybrid.md) · [Native Allocator](docs/architecture/native-allocator.md)
- [Wayland Compositor](docs/architecture/wayland.md) · [Xwayland Bridge](docs/architecture/xwayland.md)
- [Security Boundaries & Resource Limits](docs/security/README.md) · [Third-Party Dependencies](docs/security/dependencies.md)

---

## Contributing & Security

- **Contributing**: Read [`CONTRIBUTING.md`](CONTRIBUTING.md) before opening a pull request or issue. Please note that public issues and pull requests are normally analyzed with AI tools—do not post secrets or personal data in public threads.
- **Security**: To report a vulnerability privately, see [`SECURITY.md`](SECURITY.md).
- **Support**: Best-effort support is available via [GitHub Issues](https://github.com/project-trames/trames-ipad/issues) or email (`a123edcrfv@gmail.com`).

---

## License

Trames iPad is licensed under the **GNU General Public License v3.0 or later** ([`LICENSE`](LICENSE)).

Third-party licenses and notices are recorded in [`THIRD_PARTY_NOTICES.txt`](THIRD_PARTY_NOTICES.txt) and [`docs/security/dependencies.md`](docs/security/dependencies.md).

---

<p align="center">
  Part of <a href="https://github.com/project-trames">Project Trames</a> · A portable userspace Linux runtime with pluggable execution engines.
</p>
