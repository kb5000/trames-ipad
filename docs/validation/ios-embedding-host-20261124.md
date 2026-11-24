# iOS embedding 构建宿主修复：2026-11-24

产品 `3d966002` 的 Linux runtime job 在 Ubuntu 检查 `aarch64-apple-ios` 时，`trames-graphics/native/image.mm` 通过 cc-rs 查找 Apple SDK，因没有 xcrun 失败。这个检查现在已有 Apple 原生依赖，不能继续称作不需要 SDK 的纯 Rust 交叉检查。

## 修复

- 同样的 core／distro／FFI iOS 检查进入独立 `ios-embedding.yml`，在 macOS 和实际 iPhoneOS SDK 上执行。源码、Runtime pin、工具链或该 workflow 变化会触发，支持手动执行，不依赖完整 frontend 构建或旧 runtime 复用计划。
- Linux runtime job 移除错误宿主上的 Apple 检查及不再使用的 Apple Rust target；其 Linux、sanitizer、ARM oracle 等检查保留。旧 release-core 也需要编译 iOS 原生依赖，迁到 macOS，保留发布脚本核对的 job／step 名称。
- SDKROOT 显式来自 iphoneos，避免继承宿主 macOS sysroot。源绑定复用所需的 Linux step 集合及 frontend 范围识别同步更新；新增跨 workflow 的 Apple target 宿主检查，防止同类回归。
- 真实 SDK 编译继续发现 FFI 的 proc_pid_rusage 只由 macOS 版 libc 导出。进程统计统一使用公开 task_info(TASK_VM_INFO) rev1，按 SDK 的 38 个 natural_t 请求驻留及 physical footprint。返回码和输出长度均检查，只查询当前进程，不新增线程或采样策略。

## 验证

证据在 `target/ios-embedding-fix-20261124`。

- 原 iOS core／distro／FFI cargo check，真实 Xcode／iPhoneOS SDK：通过，包括原 Objective-C++ 图形依赖及修复后的 FFI。
- FFI all-features 库测试：25 通过，包含真实当前进程 RSS／physical footprint 查询。
- project-layout：4 通过；runtime-ci-reuse：11 通过。宿主检查同时找出了 release-core 的同类错误，修复后整组通过。
- FFI all-targets／all-features 严格 Clippy、格式、diff 检查通过。
- 首次本机检查继承了 macOS SDKROOT，明确报不兼容 sysroot；修复该环境后发现 libc iOS 缺失声明，最终均关闭。保留失败证据，没有跳过原生依赖。

远端 Apple 专项 CI 以本次发布源码执行；本地检查不代替远端状态。未部署应用，未重跑与交付修复无关的 CPU／网络／gVisor 矩阵。下一领域从 FUSE 开始，领域内能实现的内容和必要依赖完成后再统一验证。
