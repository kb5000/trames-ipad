# Wayland and Graphical Windows on iPad

Trames includes a built-in native Wayland compositor and optional Vulkan/Metal graphics bridge. Standard ARM64 Linux graphical applications connect directly to the Trames compositor—no guest `weston` server is required.

---

## Enabling Graphical Windows

1. Open your environment's settings (**+ → Linux Environments → select environment → Configuration**).
2. Enable **Application Windows** (for standard Wayland SHM windows).
3. For hardware-accelerated Vulkan or OpenGL ES (via Mesa/Zink) apps, also enable **Vulkan** in the environment configuration and restart the environment.

Default sessions automatically export `XDG_RUNTIME_DIR=/run/user/0` and `WAYLAND_DISPLAY=wayland-0`.

---

## Quick Start (Alpine Linux)

Install standard Wayland and Vulkan test clients from Alpine's repositories:

```sh
apk add wayland-utils weston-clients weston font-dejavu breeze-cursors \
        vulkan-loader vulkan-tools vkmark
```

> The `weston` package is installed only for client decoration assets; you do not need to run `weston`.

Launch sample clients:

```sh
# Inspect compositor protocols
wayland-info

# Standard Wayland SHM windows
weston-simple-shm &
weston-editor &

# Vulkan / Metal-bridged rendering
vkcube --wsi wayland
vkmark --winsys wayland
```

> **Tip for `vkmark`**: Always pass `--winsys wayland`. Auto-detection can probe direct-display (`display.so`) first and fail with `ErrorExtensionNotPresent` before trying Wayland.

For GTK3 apps (such as `mousepad` or `imv`), install them normally via `apk add mousepad imv`.

### OpenGL ES (Mesa / Zink)

When Vulkan is enabled on supported Alpine/musl builds, Trames configures Mesa's Zink driver automatically. Install the standard Mesa packages:

```sh
apk add mesa-dri-gallium mesa-egl mesa-gles wayland-libs-egl
```

*(Note: Bundled musl graphics adapters target Alpine/musl environments. Debian and glibc-based custom environments use their own libraries and loaders.)*

---

## Window Management & Input

- **Tabs & Floating Windows**: Terminal sessions and graphical applications share the top tab bar.
  - Tap a floating window's dock button to move it into the tab bar.
  - Drag a tab down into the workspace to detach it into a floating window, or drag tabs left/right to reorder.
  - Drag the bottom-right corner of a floating window to resize it.
  - Minimize sends the window to an inactive tab; tap the tab to restore it.
- **Keyboard, Touch & Pointer**: Hardware keyboards, touch gestures, and trackpad/mouse pointers route to the focused window. Tap any terminal tab or terminal area to return keyboard focus to the shell while keeping graphical windows open.
- **Native IME**: Applications supporting `text-input` v1/v3 (such as `weston-editor`) work with the iPadOS software keyboard and IME composition.
- **Clipboard**: Bidirectional UTF-8 text copy/paste (up to 16 KiB) works via app menus, hardware keyboard shortcuts, or the iPadOS edit menu when Host Access is enabled. Image clipboard and drag-and-drop are not yet supported.

---

## Technical Details & Current Limits

- **Zero-Copy Presentation**: SHM buffers are mapped directly for Metal sampling; Vulkan swapchain images are handed straight to Metal via MoltenVK without routine CPU pixel readback.
- **Scope**: Full Wayland/Vulkan conformance, drag-and-drop, custom cursor bitmaps, and arbitrary desktop OpenGL apps are not guaranteed. See [`docs/architecture/wayland.md`](../architecture/wayland.md) for protocol details and resource bounds.
