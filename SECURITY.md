# Security Policy

## Scope & Security Model

Trames is a **userspace Linux compatibility runtime** running inside the standard iPadOS application sandbox—not a hardware virtualization or hypervisor security boundary.

- Guest ELF binaries, syscalls, paths, and network inputs are treated as untrusted by the runtime's parsers and bounds checks.
- Guest `root` (UID 0) is strictly virtual and confers no iPadOS privileges.
- However, a memory-safety, parser, or FFI bug in the runtime or host bridge can affect the containing Trames application process (within the limits of the iPadOS sandbox).

For detailed resource bounds and threat-model assumptions, see [`docs/security/README.md`](docs/security/README.md).

---

## Reporting a Vulnerability

**Please do not open a public GitHub issue for security vulnerabilities.**

Instead, report vulnerabilities privately through either of these channels:

1. **GitHub Private Vulnerability Reporting** *(preferred)*: Use the **Security → Advisories → Report a vulnerability** button on the [`trames-ipad`](https://github.com/project-trames/trames-ipad/security/advisories) (or [`trames-runtime`](https://github.com/project-trames/trames-runtime/security/advisories)) repository.
2. **Email**: Send details to `a123edcrfv@gmail.com` with `[SECURITY]` in the subject line.

Please include:
- Affected component and version/commit (`trames-ipad`, `trames-runtime`, or `trames-cli`)
- Clear reproduction steps or a minimal proof-of-concept
- Potential impact (e.g., host memory corruption, sandbox escape, SSH key/credential exposure, File Provider path traversal)

---

## How Private Reports Are Handled

- Private security reports are **never** published without coordination with the reporter and are **excluded** from the public-channel AI processing default.
- If you voluntarily include diagnostic logs, please strip any personal keys, passwords, or tokens beforehand. Any remaining sensitive material is screened out before diagnostic review.
- Because Trames is maintained as an open-source project on a best-effort basis, we cannot guarantee a fixed response SLA, but we prioritize credible security reports and aim to acknowledge receipt promptly.
