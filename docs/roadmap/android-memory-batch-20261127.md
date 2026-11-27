# Android 内存生命周期实施批次（2026-11-27）

本轮领域：`userfaultfd`、`process_mrelease`、全局内存压力源及原版 ART／lmkd 自动回收。所有者已明确将 PSI 与自动杀进程纳入本轮。先完成领域内实现与调用方，再集中验证；不增加生产协调／服务线程。调试使用模拟器及既有 Linux ARM64 参考机。

## 固定输入与实际调用

- 产品清单 `runtime/scripts/android-images.json` 固定的是 Android 13／API 33、Lineage 20 ARM64 Waydroid，system ZIP SHA256 为 `5fb8179ed4455f45919c5a2bfbfdd1591619295994ad321217f03bb1dcfc53a7`。不把此前其他 Android 版本的设想当作当前镜像行为。
- 从原始 system.img 只读提取的 `build.prop`、`lmkd` 和 ART 位于 `target/local-ci/io/android-memory-20261127`。对应 AOSP `android-13.0.0_r83` 源码及 Linux 参考快照位于 `target/validation-android-memory-20261127`，最终验证记录须列出摘要。
- 此 ART 使用 concurrent-copying；`Heap::PostForkChildAction` 以 `O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY` 创建 fd，再执行 `UFFDIO_API(features=0)`。后台 GC 还执行六种零长度 ioctl 并要求 EINVAL；源码明确接受创建时的 ENOSYS／EACCES。这些是能力探测，不能据此宣称已启用并发压缩或产生 GC 加速。
- 原版 lmkd 使用 pidfd SIGKILL 后调用 `process_mrelease(pidfd, 0)`；探测 `process_mrelease(-1, 0)` 时只把 ENOSYS 判作不支持。reaper 的 guest 线程属于原版 Android 程序，不是新增宿主协调线程。

## 实现边界与集中验证

- 原 mm 控制请求通过现有 proc/mm checkpoint 执行，弱请求队列支持取消，复用原通知与执行槽。
- `process_mrelease` 保留原 pidfd 身份，只接纳所有 mm 使用者均已死亡／退出的情况；私有存储及旧视图在原 mm 边界退休，保留共享 backing 及独立 pin 的实际租约，重复调用与退出竞态按内核结果处理。
- `userfaultfd` 实现私有匿名 MISSING、API 协商、REGISTER／UNREGISTER、COPY、ZEROPAGE、WAKE、DONTWAKE、read／poll／epoll、dup／SCM_RIGHTS；未提供的 WP／MINOR／fork/remap/remove 事件功能不声明支持。
- 缺页通过现有 `PageWait` 挂起，分辨用户指令／原生 guest 函数与内核拷贝；仅为已使用缺页注册的 mm 收紧硬件粗映射。fork 子 mm 不继承注册，CLONE_VM 保留原注册；关闭、取消、unmap／remap、discard、exec／最后 mm 退出解除原等待。
- 以同一静态 ARM64 ELF 比较 Linux 原生、解释器、HVF；补原资源单测、Linux／Apple 编译与相关检查。每项有独立超时及日志，未变的其他领域不重复全量测试。
- 最后运行原镜像的 ART／lmkd 压力验收，记录实际 collector、GC 暂停尾延迟与应用终止后的实际内存下降。GC 使用的路径与新增 ABI 分开报告，不把探测通过当作 GC 加速。

## 全局压力与原版自动回收

- PSI 统计实际任务的分配、回收和宿主分页等待；触发器绑定原 OFD、打开时权限及原会话，使用现有 poll／epoll 截止时间，不创建 PSI 服务或计时线程。
- 有界会话从原 aggregate budget 读取实际占用，接近 reserve 时提交原 mm 页扫描并挂起分配，释放后重试；lmkd 等受保护任务可使用 reserve，原 hard limit 仍保留。没有可回收目标时有界返回现有内存上限结果。
- HostManaged 的可用内存保持宿主提示，不改成 guest 硬配额；原宿主分页等待计入压力，原回收完成量提供扫描／回收遥测。Apple 通知留在 Mac 平台边界，共享 Runtime 使用平台无关接口。
- 恢复原版 lmkd 默认 PSI 监测；自动回收夹具只按原协议注册优先级、实际触页，必须由原版 daemon 选择并终止缓存进程。原组件的显式 reaper 测试单独保留。
- PSI 为该执行模型的按 CPU 近似统计及连续衰减平均，未声称与 Linux 内核周期采样的时间值逐位相同；缺失的 file LRU／swap 不伪造。

状态：本轮按约定范围完成。两宿主资源 2,364／2,374 通过，各 5 既有忽略；原版 lmkd 连续十轮自动回收、Zygote／Reaper、ART 三对及 iOS Rust 验收通过。ART 中位耗时 405.94→392.18 ms，保留原始波动，不宣称 GC 加速。按 main 配对 Runtime pins 交付；详见[完整验证及边界](../../runtime/docs/validation/android-memory-20261127.md)。
