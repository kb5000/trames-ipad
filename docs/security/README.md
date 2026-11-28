# Runtime Security Model & Resource Boundaries

This document summarizes Trames's security model, trust boundaries, and deliberate resource limits. For reporting security vulnerabilities privately, see [`SECURITY.md`](../../SECURITY.md).

---

## 1. Security Model

Trames is a **userspace Linux compatibility runtime** executing inside the standard iPadOS (or host OS) application sandbox. It is **not** a hardware virtualization or hypervisor security boundary.

- **Untrusted Guest Inputs**: Guest ELF binaries, instructions, addresses, syscalls, paths, archives, and IPC payloads are treated as untrusted by the runtime's parsers and bounds checks. Guest faults produce Linux errors/signals rather than unchecked host pointer dereferences.
- **No Host Executable Memory (iPadOS)**: On iPadOS, guest code is never mapped to host executable (`W^X`) pages. Writable/executable guest mappings remain interpreted data inside `trames-interpreter`.
- **Virtual Privileges Only**: Guest `root` (UID 0), virtual ownership, and `/proc`/`/sys`/`/dev` views are strictly virtual. They never escalate host privileges or expose host processes or file descriptors.
- **Sandbox Containment**: Bugs in runtime parsers, FFI boundaries, or host bridges can affect the containing Trames application process (within the limits of the iPadOS sandbox). Software tests and sanitizers are not a substitute for an independent security audit.

---

## 2. Filesystem & Storage Isolation

- **Private Virtual Namespace**: Guest path operations access only the environment's private VFS. The disk backend stores files under fixed inode-object names inside the app's private or App Group container—never using guest path components directly as host filenames.
- **No Arbitrary Host Mounts on iPadOS**: Guest symlinks cannot escape the virtual namespace. Host files are accessed only through explicit iPadOS Files pickers, retained security-scoped folder bookmarks, USB transfer staging, or the bundled `TramesFileProvider` extension.
- **Integrity & Locking**: VFS manifests are versioned, checksummed (for accidental corruption detection, not cryptographic authentication), and replaced atomically. Advisory OS locks protect active stores against concurrent writers.

---

## 3. Networking & Host Access

- **Userspace Packet Stack**: Virtual networking runs in userspace (`smoltcp`) with private guest addressing and loopback. No host virtual network interface, host route mutation, or VPN entitlement is required.
- **Host Policy Applies**: Outbound guest TCP/UDP/ICMP egress uses ordinary nonblocking host sockets subject to iPadOS network permissions and host firewall rules. Guest TLS verification and package signatures (`apk`/`apt`) execute unchanged inside the guest.
- **Host Access Controls**: Clipboard, file transfers, and host UI bridge actions are gated by per-environment Host Access settings plus iPadOS system permissions. Imported TOML profiles cannot silently escalate existing local grants.

---

## 4. CPU Compatibility Profile

- The iPad runtime uses the shared non-JIT Rust interpreter with checked guest memory and bounded execution slices.
- Static native arithmetic/memory helpers operate strictly on validated guest buffers or scalar values and never generate executable memory.
- For broad userspace compatibility, ARMv8.3 PAC authentication and BTI landing checks are disabled, and MTE tags read as zero. These features are not advertised as hardware security mitigations.

---

## 5. Deliberate Resource Bounds

These bounds prevent runaway guest allocations and protect the containing host process:

| Resource | Default / Bound |
| :--- | :--- |
| Guest address domain | Below $2^{48}$ (48-bit VA space) |
| Input ELF size / program headers | 256 MiB / 1,024 headers per ELF |
| Guest memory | Demand-backed (`memory = "auto"` on iPad by default; configurable per-environment or CLI quota) |
| Private `PROT_NONE` reservations | Sparse virtual ranges; up to 65,536 intervals |
| Admitted tasks (`kernel.threads-max`) | Default 65,536 (configurable `1`–`1,073,741,823`) |
| Initial stack / `argv`+`envp` | 1 MiB non-executable stack with guard page / 64 KiB & 4,096 entries |
| Pipe / PTY buffers | 64 KiB (4,096-byte atomic pipe write) |
| Nested signal frames | 8 |
| Symlink traversal / path length | 40 hops / 4,095 bytes (255 bytes per component) |
| Virtual disk budget | 16 GiB initial logical budget (volume-bounded and configurable) |
| Inode xattrs | `user.*` only; 128 attrs & 64 KiB per inode; 1 MiB per filesystem |
| Guest Unix sockets | 1,024 live endpoints; 64 KiB / 128 queued messages per direction |
| Single output syscall copy | Up to 1 MiB per call (larger calls may return short writes) |

---

## 6. Third-Party Dependencies

All reviewed Rust, Swift, and guest payload dependencies and their licenses are tracked in [`docs/security/dependencies.md`](dependencies.md) and [`THIRD_PARTY_NOTICES.txt`](../../THIRD_PARTY_NOTICES.txt).
