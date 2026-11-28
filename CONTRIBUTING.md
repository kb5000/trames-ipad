# Contributing to Trames iPad

Thank you for your interest in contributing to **Trames**. This guide explains how the repositories fit together, how to set up and validate changes, and how community submissions are handled.

---

## 1. Where Changes Belong

Project Trames is split into modular repositories so Linux compatibility improvements benefit every platform at once:

| Repository | What belongs here |
| :--- | :--- |
| [`trames-ipad`](https://github.com/project-trames/trames-ipad) *(this repo)* | iPadOS SwiftUI app, File Provider extension, `trames-ffi` C ABI, iPad non-JIT composition (`trames-core`, `trames-cpu`, `trames-compute`), guest acceleration stubs, and iPad packaging/docs. |
| [`trames-runtime`](https://github.com/project-trames/trames-runtime) (`runtime/` submodule) | Shared userspace Linux kernel: syscalls, VFS, networking, process/thread/signal/futex semantics, virtual memory, ELF loader, Wayland/Vulkan/audio/video services, and the shared interpreter. |
| [`trames-cli`](https://github.com/project-trames/trames-cli) (`cli/` submodule) | Shared host CLI parser, raw PTY handling, signal forwarding, and `TRMEXEC1`/`TRAMES02` wire protocols. |

**Rule of thumb**: Do not patch Linux kernel semantics solely inside `trames-ipad`. Fix shared kernel behavior in `trames-runtime`, validate it there, and then update the `runtime/` submodule pin and `runtime-version.json` in `trames-ipad`.

---

## 2. Development Setup

1. Clone with submodules:
   ```sh
   git clone --recurse-submodules https://github.com/project-trames/trames-ipad.git
   cd trames-ipad
   ```
2. Install the pinned Rust toolchain automatically via `rust-toolchain.toml` (requires `rustup` with `rustfmt` and `clippy`).
3. For iPad app builds on macOS, install **Xcode 16+** and **XcodeGen** (`brew install xcodegen`).

---

## 3. Coding Standards & Validation

Please review [`AGENTS.md`](AGENTS.md) for the project's core engineering principles (guest memory model, lock-free/object-level concurrency, dual Linux/Apple host compatibility, and scoped verification).

Before opening a pull request, run the checks relevant to your change:

```sh
# Rust formatting and lints
cargo fmt --all --check
cargo clippy --workspace --all-targets --locked -- -D warnings

# Rust unit and integration tests
cargo test --workspace --locked

# Execution boundary & submodule pin verification
python3 scripts/check-execution-boundaries.py
```

For changes touching `apps/ios/`, also verify the iPad simulator build:
```sh
bash scripts/build-ios.sh simulator
```

---

## 4. Submitting Issues & Pull Requests

- **Bug reports**: Include your iPad model, iPadOS version, Trames version/build, environment type (Alpine, Debian, custom), exact commands run, and observed vs. expected output.
- **Pull requests**: Keep PRs focused, target `main`, explain *why* the change is needed, and list the exact checks you ran.
- **No fabricated claims**: Report only measurements and tests you actually executed.

---

## 5. AI-Assisted Workflow Disclosure

Trames is an AI-driven open-source project. Unless explicitly stated otherwise for a specific channel:

- Public GitHub issues, pull requests, and discussions are normally analyzed and processed with external AI tools to triage bugs, review changes, and maintain documentation.
- **Do not post passwords, SSH keys, private certificates, or personal identifying information in public issues or pull requests.**
- Posting publicly does not transfer your copyright, grant commercial relicensing rights, or grant model-training permission.
- For private security disclosures or sensitive logs, use the private channels described in [`SECURITY.md`](SECURITY.md).

---

## 6. Licensing

All contributions to `trames-ipad` are licensed under **GNU General Public License v3.0 or later** ([`LICENSE`](LICENSE)). Make sure any third-party code or dependency you introduce is license-compatible and properly recorded in `third-party/` and `THIRD_PARTY_NOTICES.txt`.
