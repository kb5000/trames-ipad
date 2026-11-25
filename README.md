<p align="center">
  <img src="resources/branding/trames-app-icon-ios.png" alt="Trames icon" width="112">
</p>

<h1 align="center">Trames iPad</h1>

<p align="center">
  <strong>A local Linux terminal and graphical workspace for iPadOS.</strong>
</p>

<p align="center">
  Run unmodified ARM64 Linux binaries directly on your iPad via a shared userspace kernel and an optimized AArch64 interpreter.<br>
  No jailbreak, JIT entitlements, or hardware virtualization required.
</p>

<p align="center">
  <a href="https://github.com/project-trames/trames-ipad/releases/latest"><img src="https://img.shields.io/github/v/release/project-trames/trames-ipad?style=flat-square&amp;color=f06b45" alt="Latest release"></a>
  <a href="https://github.com/project-trames/trames-ipad/actions/workflows/ios.yml"><img src="https://github.com/project-trames/trames-ipad/actions/workflows/ios.yml/badge.svg?branch=main" alt="iPad frontend CI"></a>
  <a href="https://github.com/project-trames/trames-ipad/actions/workflows/linux.yml"><img src="https://github.com/project-trames/trames-ipad/actions/workflows/linux.yml/badge.svg?branch=main" alt="Linux runtime CI"></a>
</p>

<p align="center">
  <a href="#installation"><img src="https://img.shields.io/badge/iPadOS-17%2B-30303d?style=flat-square&amp;logo=apple&amp;logoColor=white" alt="iPadOS 17 or later"></a>
  <a href="docs/architecture/nonjit.md"><img src="https://img.shields.io/badge/guest-ARM64-f0ad43?style=flat-square" alt="ARM64 Linux guest"></a>
  <a href="rust-toolchain.toml"><img src="https://img.shields.io/badge/Rust-nightly-b65437?style=flat-square&amp;logo=rust&amp;logoColor=white" alt="Rust nightly"></a>
  <a href="apps/ios"><img src="https://img.shields.io/badge/UI-Swift-f05138?style=flat-square&amp;logo=swift&amp;logoColor=white" alt="Swift user interface"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0--or--later-7c5269?style=flat-square" alt="GPL-3.0-or-later"></a>
</p>

<p align="center">
  <a href="https://github.com/project-trames/trames-ipad/releases/latest">Download</a> ·
  <a href="#features">Features</a> ·
  <a href="#architecture">Architecture</a> ·
  <a href="#installation">Installation</a> ·
  <a href="#building-from-source">Building</a> ·
  <a href="#documentation">Documentation</a>
</p>

---

## Features

Trames turns your iPad into a capable, self-contained Linux development environment:

* **Terminals & Windows**: Native multi-tab terminal multiplexer, floating graphical windows, and split workspaces for CLI and GUI applications.
* **Persistent Linux Distributions**: Out-of-the-box support for Alpine, Debian, and custom ARM64 rootfs images with persistent package management and storage.
* **Developer Toolchains**: Run standard Linux compilers and utilities natively—Git, Vim/Neovim, Python, Clang, GCC, and Node.js.
* **Hardware-Bridged Graphics**: Integrated Wayland compositor and Xwayland server, bridging guest Vulkan and OpenGL to iPadOS Metal.
* **Host Integration**: Seamless Files app integration via File Provider, bidirectional clipboard, and native iPadOS hardware keyboard and trackpad support.
* **Granular Control**: Per-environment CPU, memory, networking, and startup profile configurations backed by TOML configurations.

> **Note on Non-JIT Execution**: Trames executes guest ARM64 instructions through an optimized software interpreter with decoded-block caches, soft-TLB, and native fast-paths. Because JIT is not permitted in standard iPadOS app sandboxes, compute-heavy tasks will run slower than native bare-metal execution.

---

## Architecture

Trames iPad is composed of three primary layers:

```text
┌────────────────────────────────────────────────────────┐
│               iPadOS Native Shell (Swift)              │
│       Window Manager · File Provider · Metal Host      │
└───────────────────────────┬────────────────────────────┘
                            │ Native FFI
┌───────────────────────────┴────────────────────────────┐
│                    trames-runtime                      │
│      Shared Userspace Linux Kernel (VFS, Net, Syscalls) │
└───────────────────────────┬────────────────────────────┘
                            │ Execution Contract
┌───────────────────────────┴────────────────────────────┐
│               trames-cpu (AArch64 Interpreter)          │
│        Non-JIT execution with native helper thunks      │
└────────────────────────────────────────────────────────┘
```

* **Shared Runtime ([`trames-runtime`](https://github.com/project-trames/trames-runtime))**: Pure-Rust userspace Linux kernel implementing syscall semantics, processes, memory paging, VFS, and userspace networking.
* **AArch64 Interpreter (`crates/trames-cpu`)**: High-efficiency software interpreter for ARM64 instructions, ensuring full App Store sandbox compliance without requiring `dynamic-codesigning` entitlements.
* **Host Integration (`apps/ios`)**: Modern Swift/SwiftUI frontend providing terminal emulation, windowing, and host subsystem bindings.

---

## Installation

### Prerequisites
* iPad running **iPadOS 17.0 or later** (Apple Silicon or A12X/Z recommended).

### Sideloading (Pre-built IPA)

1. Grab the unsigned IPA from the [Latest Releases](https://github.com/project-trames/trames-ipad/releases/latest).
2. Resign and install the `.ipa` using your preferred signing tool (e.g., Xcode, Sideloadly, AltStore, or TrollStore).
   > **Important**: Ensure both the main application and the embedded `FileProviderExtension` share the exact same App Group configuration.
3. Launch Trames and tap **+ → Linux Environments → New Environment** to initialize your rootfs.

---

## Building from Source

### Prerequisites

* macOS with Xcode 15+ and iOS SDK
* [XcodeGen](https://github.com/yonaskolb/XcodeGen) (`brew install xcodegen`)
* Rust nightly toolchain (configured via `rust-toolchain.toml`)
* Python 3

### Clone & Build

Before running the Apple build script, prepare the matching Linux guest libraries and graphics/video payloads. These generated files are not included in a fresh clone; see the guest build and artifact staging steps in the [iPad CI workflow](.github/workflows/ios.yml).

```sh
# Clone with submodules
git clone --recurse-submodules https://github.com/project-trames/trames-ipad.git
cd trames-ipad

# Build for iPad Simulator
bash scripts/build-ios.sh simulator

# Or build an unsigned bundle for physical devices
bash scripts/build-ios.sh device
```

For device testing, open `apps/ios/Trames.xcodeproj` in Xcode, select your personal/team development identity, configure your App Group, and run on device.

---

## Documentation

Comprehensive architecture notes and engineering validation logs are maintained in `docs/`:

* **Getting Started**: [iPad Environment Configuration](docs/guides/ipad-environments.md) · [Rootfs & File Management](docs/guides/rootfs-files.md)
* **Execution & CPU**: [Interpreter Architecture](docs/architecture/nonjit.md) · [Execution Backends](docs/architecture/execution-backends.md)
* **Memory & Acceleration**: [Region Memory Model](docs/architecture/region-memory.md) · [Native Allocator Fastpath](docs/architecture/native-allocator.md) · [Hybrid Libc](docs/architecture/musl-hybrid.md)
* **Graphics Subsystem**: [Wayland Compositor](docs/architecture/wayland.md) · [Xwayland Bridge](docs/architecture/xwayland.md)
* **Engineering Logs**: [Syscall Implementation Status](docs/roadmap/syscall-implementation.md) · [Validation Evidence](docs/validation/)

---

## Community & Contributing

We welcome issues, benchmark submissions, and pull requests! Please read our [Contributing Guidelines](https://github.com/project-trames/.github/blob/main/CONTRIBUTING.md) and [`AGENTS.md`](AGENTS.md) before submitting patches.

* **Bugs & Features**: [GitHub Issues](https://github.com/project-trames/trames-ipad/issues)
* **Security Concerns**: See [Security Policy](https://github.com/project-trames/.github/blob/main/SECURITY.md) for private reporting.
* **Code of Conduct**: [Code of Conduct](https://github.com/project-trames/.github/blob/main/CODE_OF_CONDUCT.md).

---

## License

Trames iPad is licensed under the **GNU General Public License v3.0 or later** ([GPL-3.0-or-later](LICENSE)).<br>
Third-party dependencies and graphics payload licenses are tracked in [docs/security/dependencies.md](docs/security/dependencies.md).

---

<p align="center">
  Part of <a href="https://github.com/project-trames">Project Trames</a> · A portable userspace Linux runtime with pluggable execution engines.
</p>
