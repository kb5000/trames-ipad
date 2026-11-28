# Browsing and Editing Linux Files in the iPadOS Files App

Trames includes a File Provider extension (`TramesFileProvider`) so you can browse, preview, copy, and edit regular files inside your Linux environments directly from the iPadOS **Files** app.

---

## Enabling Trames in Files

1. Open the iPadOS **Files** app.
2. Go to **Browse** → tap **…** (top right) → **Edit**.
3. Enable **Trames**.
4. Open **Trames → `<environment name>`** to browse that environment's root filesystem.

Each Alpine, Debian, and custom environment appears as its own folder.

---

## Supported Operations

| Action | Supported in Files App | Notes |
| :--- | :---: | :--- |
| Browse directories | Yes | Works even while Trames is paused or stopped. |
| Preview / Quick Look / copy / export regular files | Yes | Streamed in bounded chunks without launching a second kernel instance. |
| Edit existing regular files in place | Yes | Supported for UTF-8 paths in editors that save in place. |
| Create new files, rename, or delete | No | Use Trames's in-app **Files** panel or Linux shell commands instead. |
| Open symlinks or `/proc`, `/sys`, `/dev` | No | Symlinks and special nodes show metadata only and are never followed onto the host. |

---

## How In-Place Edits Work (Deferred Writeback)

To avoid concurrent writes to the guest virtual filesystem, the File Provider extension never opens a writable VFS directly:

1. When you save an edited file in another iPadOS app, the extension stages a copy into the shared App Group journal.
2. When you return to Trames with that environment active, Trames atomically commits the staged edit after checking that the original inode and SHA-256 content digest have not changed.
3. If the target environment is currently stopped or another environment is active, your edit remains safely staged until you open that environment again—it is never written into the wrong environment.
4. If a conflict occurs (the file was modified inside Linux in the meantime) or disk space is exhausted, Trames preserves the original Linux file and keeps your staged copy under **Pending Edits (`待写回的编辑`)** in Files so you can recover or export it.

---

## In-App Files Panel & USB File Sharing

For creating new files, importing archives, or transferring large datasets:

- **In-App Files Panel**: Import files via the system document picker or export guest files without overwriting existing destinations.
- **USB File Sharing**: Connect your iPad to Finder (macOS) or Apple Devices (Windows) to transfer files into or out of Trames's `Documents` transfer folder, then import/export them from the in-app Files panel.

---

## Signing Note (Self-Built & Sideloaded Copies)

The Files app integration requires the main app and `TramesFileProvider` to be signed with the same team and matching App Group entitlement (`group.org.trames.Trames` in official builds). If your signing setup does not include a shared App Group, Trames automatically falls back to its private store and in-app / USB transfers remain fully functional.
