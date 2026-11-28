# Trames iPad — App Store & TestFlight Listing Copy (Draft)

> **Usage**: Copy-ready metadata drafts for App Store Connect (English `en-US` and Simplified Chinese `zh-Hans`) and TestFlight. Character limits are checked against App Store Connect constraints (Name: 30, Subtitle: 30, Promotional Text: 170, Keywords: 100).

---

## 1. English (`en-US`)

### App Name (6 / 30 chars)
```
Trames
```

### Subtitle (29 / 30 chars)
```
Local Linux Workspace on iPad
```

### Promotional Text (126 / 170 chars)
```
Run unmodified ARM64 Linux environments locally on iPadOS—with multi-tab terminals, Files integration, and Wayland windows.
```

### Description
```
Trames brings a local Linux workspace to your iPad—no jailbreak, no remote server required, and no account sign-in.

Run unmodified ARM64 Linux binaries inside a self-contained userspace Linux runtime built in Rust, right alongside your everyday iPadOS workflow.

LOCAL LINUX ENVIRONMENTS
• Start immediately with Alpine Linux, add Debian 13 (shell or systemd), or import your own ARM64 tar/tar.gz rootfs archive.
• Manage multiple isolated environments with independent storage, packages, and per-environment TOML configurations.
• Install standard developer tools—Git, OpenSSH, Vim/Neovim, Python, Node.js, Clang, GCC, CMake, and more.
• Works offline once an environment is initialized.

MULTI-TAB TERMINAL & WORKSPACE
• Multi-tab terminal emulator with Unicode, true color, touch selection, hardware keyboard support, and customizable extra keys.
• Dock terminals and graphical windows into the tab bar or detach them into floating windows.

FILES & WORKSPACE INTEGRATION
• Browse and edit files inside your Linux environments directly from the iPadOS Files app via the built-in File Provider extension.
• Import and export files through the in-app Files panel (+ → Files) or USB File Sharing without overwriting existing data.
• Optional Wayland graphical windows with hardware keyboard, pointer, and text clipboard support.

PRIVACY & OPEN SOURCE
• Local-first by design: your environments, terminal history, and files stay on your iPad.
• Free, open-source software (GPL-3.0-or-later) with zero ads, zero subscriptions, and zero developer analytics.

NOTE ON PERFORMANCE
To comply with standard iPadOS security and sandbox rules, Trames executes guest ARM64 instructions using an optimized software interpreter rather than JIT compilation. Lightweight CLI workflows, editing, scripting, and everyday tools run smoothly, while heavy compilation and compute-bound tasks will be slower than native hardware.
```

### Keywords (98 / 100 chars)
```
linux,terminal,alpine,debian,shell,developer,git,python,wayland,cli,sandbox,arm64,console,ssh,vim
```

### What's New (Initial Release)
```
Initial App Store release of Trames for iPad:
• Local ARM64 Linux environments (Alpine, Debian 13, and custom rootfs import)
• Multi-tab terminal with detachable floating windows
• iPadOS Files app integration via File Provider, in-app Files panel, and USB File Sharing
• Optional Wayland graphical window support with Metal presentation
• Per-environment TOML configuration and Light/Dark appearance support
```

### Categories
- **Primary**: Developer Tools
- **Secondary**: Utilities

---

## 2. Simplified Chinese (`zh-Hans`)

### App Name
```
Trames
```

### Subtitle (16 / 30 chars)
```
iPad 本地 Linux 工作区与终端
```

### Promotional Text
```
在 iPad 上直接运行未修改的 ARM64 Linux 环境——集成多标签终端、「文件」App 联动与 Wayland 图形窗口。
```

### Description
```
Trames 将完整的本地 Linux 工作区带到你的 iPad 上——无需越狱，无需依赖远程服务器，也无需注册账号。

借助纯 Rust 实现的用户态 Linux 运行时，你可以在标准 iPadOS 沙盒内直接运行未修改的 ARM64 Linux 工具链，并与系统工作流自然衔接。

本地 Linux 环境
• 开箱即用内置 Alpine Linux，并支持新建 Debian 13（Shell 或 systemd 启动）或导入自定义 ARM64 tar/tar.gz rootfs 归档。
• 支持创建多个相互隔离的环境，独立管理文件系统、软件包与 TOML 配置。
• 运行常用开发工具链：Git、OpenSSH、Vim/Neovim、Python、Node.js、Clang、GCC、CMake 等。
• 环境初始化完成后即可离线使用。

多标签终端与窗口工作区
• 多标签终端，支持 Unicode、真彩色、触控选区、外接键盘快捷键与可自定义扩展按键栏。
• 终端与图形窗口既可停靠在顶部标签栏，也可拖出为浮动窗口。

系统「文件」与图形工作区联动
• 通过内置 File Provider 扩展，直接在 iPadOS「文件」App 中浏览并编辑 Linux 环境内的常规文件。
• 支持通过应用内文件面板（+ → Files）及 USB 文件共享安全导入与导出文件。
• 可选的 Wayland 图形窗口支持，支持外接键鼠与双向文本剪贴板。

隐私与开源
• 本地优先设计：所有 Linux 环境、终端历史与文件均保存在你的设备上。
• 免费开源（GPL-3.0-or-later），无广告、无内购订阅，生产版本不含任何开发者遥测或统计代码。

关于运行性能的说明
为严格遵循 iPadOS 标准安全与沙盒机制，Trames 使用优化的软件解释器（而非 JIT 动态编译）执行 ARM64 Linux 指令。日常命令行操作、文本编辑、脚本运行与远程运维均可顺畅使用；大型源码编译等计算密集型任务的速度会慢于原生硬件。
```

### Keywords
```
linux,终端,alpine,debian,shell,开发工具,git,python,命令行,wayland,沙盒,程序员,ssh,vim
```

### What's New (Initial Release)
```
Trames iPad 首个 App Store 版本发布：
• 支持本地运行 ARM64 Linux 环境（Alpine、Debian 13 及自定义 rootfs 导入）
• 多标签终端与可拖出浮动窗口工作区
• 通过 File Provider 深度集成 iPadOS「文件」App，并支持应用内文件面板与 USB 文件共享
• 支持可选的 Wayland 图形窗口与 Metal 渲染呈现
• 支持分环境 TOML 配置与浅色/深色外观切换
```

---

## 3. TestFlight Beta Information

### Beta App Description (English)
```
Trames is an open-source local Linux workspace for iPadOS (iPadOS 17+). This TestFlight beta helps us validate environment setup, Files integration, terminal ergonomics, and Wayland graphics across real iPad hardware before the public App Store release.

Please note: Apple automatically collects TestFlight crash logs and beta usage diagnostics. For details, see our Privacy Policy.
```

### What to Test (English)
```
1. First-launch setup: Launch Trames to initialize the default Alpine environment; optionally create a Debian 13 environment (+ → Linux Environments → New Environment) and test package installation (e.g., apk add git python3).
2. Persistence & Files app: Create a file in the terminal, inspect/edit it in the iPadOS Files app under Trames, and confirm edits commit when returning to Trames.
3. Terminal & Workspace: Test hardware keyboard shortcuts, multi-tab switching, and detaching tabs into floating windows.
4. Background/Foreground lifecycle: Switch away from Trames and return to verify the local session pauses and resumes cleanly.
5. (Optional) Wayland windows: Enable Application Windows / Vulkan in environment Configuration and test docking, resizing, and clipboard copy/paste.
```
