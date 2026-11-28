# Linux Environments & Configuration on iPad

Trames lets you create and manage multiple isolated ARM64 Linux environments on your iPad—each with its own filesystem, installed packages, host permissions, and TOML configuration.

One local environment runs at a time. Switching to another environment stops the active session; returning from the background resumes the active session.

---

## Creating and Managing Environments

On first launch, Trames initializes the default **Alpine Linux** environment automatically. To add more environments:

1. Tap **+** in the top toolbar → **Linux Environments**.
2. Tap **New Environment** at the bottom of the sidebar.
3. Choose a distribution:
   - **Alpine**: Lightweight musl-based ARM64 Linux using a shell entrypoint.
   - **Debian 13**: Full glibc-based ARM64 Linux with selectable package profiles (**Daily Tools**, **Development Tools**, or **Minimal Base**) and selectable boot mode (**Shell** or **systemd**).
   - **Custom rootfs**: Import a `tar` or `tar.gz` ARM64 Linux rootfs archive from Files (optional SHA-256 verification). The archive must contain executable `/bin/sh`, `setsid` on `PATH`, and their ARM64 Linux libraries (plus `/usr/lib/systemd/systemd` and D-Bus for systemd mode).
4. Tap **Create Environment** to save the profile, then select **Open Environment** in the detail view when ready to install and start it.

### Maintenance & Deletion
- **Open Maintenance Terminal**: Starts a repair shell while skipping pending package setup, useful if an installation was interrupted.
- **Restart / Stop Environment**: Available under **More** in the environment detail view.
- **Reset Environment**: **Settings → Reset Linux Environment** resets only the active environment without affecting other environments or Files folder grants.
- **Delete Environment**: Choose **More → Delete Environment…** (or long-press its sidebar row) to permanently remove that environment's store, profile, host grants, and pending Files edits.

---

## Per-Environment Configuration (TOML)

The Environment Manager is organized into five sections: **Overview**, **Configuration**, **Files and Sharing**, **Host Access**, and **Activity**.

Each environment is backed by a TOML profile (`profiles/<id>.toml`) sharing the same schema as Trames on Mac and CLI:

- **Resources**: CPU core count (defaults to all available), demand-backed memory (`memory = "auto"` by default, or an explicit quota), and optional `kernel.threads-max` sysctl (`1` to `1,073,741,823`, default `65536`).
- **Startup**: Boot mode (`shell` or `systemd`), literal shell arguments (passed without shell splitting), and working directory.
- **Components**: Application Windows (Wayland), Vulkan, VA-API, native acceleration, audio input/output, and network mode (`host` or disabled).
- **TOML Actions**: **Edit TOML**, **Import TOML**, **Export TOML**, and **Create from TOML** are available in **Overview**. Edits remain drafts until you tap **Save Configuration** and take effect on the next environment start.

### Host Access & Permissions
Under **Host Access**, a single switch controls clipboard, file transfers, and host UI actions for that environment. Imported TOML files never escalate existing local permissions. Standard iPadOS permissions (Files folder bookmarks, Local Network, Microphone) always apply independently.

> **Platform Differences**: Mac-only features (direct host folder mounts, host command execution, port publication, and host `trames` CLI) are unavailable on iPadOS and fail explicitly rather than falling back silently.

---

## Appearance & Language

- **Appearance**: **Settings → Appearance → Interface Style** lets you choose **Follow System** (default), **Light**, or **Dark**, updating the workspace chrome and default terminal palette immediately while preserving scrollback and ANSI colors.
- **App Language vs. Guest Locale**: Trames supports English and Simplified Chinese (following your iPadOS per-app language preference). Changing the app language does not modify guest Linux locale; `C.UTF-8` is available by default for UTF-8 terminal text.

---

## Injected Guest `trames` CLI

Linux environments include a static ARM64 helper at `/usr/local/bin/trames` (and `/usr/bin/trames`) with no guest package dependencies:

```sh
trames --help
trames status
trames doctor
trames permissions
trames clipboard get
trames clipboard set
trames files ls
trames files cp <src> <dst>
```

Pass `--json` for machine-readable output, or set `TRAMES_LANGUAGE=zh` for Chinese help text. Files transfers use `share:<PROVIDER_ID>/<relative/path>` for explicitly authorized folders (up to 1 GiB per file).

---

## Graphics & Video Notes

- **Wayland & Vulkan**: See [`wayland.md`](wayland.md) for enabling graphical windows, Vulkan, and Mesa/Zink OpenGL ES on Alpine/musl environments.
- **Firefox on Alpine**: Install `dbus-x11 at-spi2-core font-noto-cjk` and launch with `dbus-run-session -- firefox` under Wayland (`GDK_BACKEND=wayland`).
- **Files Integration**: See [`rootfs-files.md`](rootfs-files.md) for browsing and editing environment files in the iPadOS **Files** app.

---

## Experimental Development Features (Source Builds Only)

Source/development builds may also include experimental features that are excluded from the initial App Store release:

- **Experimental Android Environments**: Supports importing a Trames ARM64 Android adapter archive and downloading pinned Waydroid images (~1 GB). Keep the app in the foreground during setup; export preparation logs via **Settings → Diagnostics → Share environment preparation log** and Android diagnostics via **Collect Android diagnostics**.
- **Picture in Picture**: Allows previewing a selected terminal or software-rendered Wayland window in a PiP overlay.
