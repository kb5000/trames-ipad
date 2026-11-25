# FUSE 原版工具对照检查点（2026-11-25）

后续状态：本页保留当时的失败与冻结版本；两个阻塞的修复及九组真实工具通过见[传输修复记录](fuse-transfer-20261125.md)。

本轮真实工具验收未通过：原版 libfuse 的低层工具在 Trames 启动阶段退出，高层工具的 BusyBox 复制路径触发内部断言。单独运行后，高层工具的五类普通客户端场景在解释器 1／2／4 CPU 下全部通过，卸载与退出也正常。两个阻塞都保留原始失败，尚未修改生产实现；02 FUSE 及完整 01–12 队列保持开放。

## 固定输入与版本边界

- 工具使用未修改的 libfuse `fuse-3.14.0` 标签中的 [passthrough_ll.c](https://github.com/libfuse/libfuse/blob/fuse-3.14.0/example/passthrough_ll.c)、[passthrough.c](https://github.com/libfuse/libfuse/blob/fuse-3.14.0/example/passthrough.c) 及其 helper；静态链接 Ubuntu ARM64 `libfuse3-dev=3.14.0-5build1`。只下载并解包到测试目录，没有安装系统软件或改变共享 Linux 环境。
- 原生与 guest 使用同一静态工具／客户端 ELF、同一已固定 Alpine BusyBox rootfs。普通客户端见 `runtime/tests/fixtures/fuse-tool-client.c`，场景脚本见 `runtime/tests/fixtures/fuse-tool-workload.sh`；客户端没有实现 FUSE 协议。
- 解释器沿用[上一修复检查点](fuse-repair-20261125.md)的冻结产物，SHA-256 为 `604919298103427c616774584511504f4689a0b31e353acadf1f47d25f4d5d46`。工具、客户端、rootfs、脚本与 runner 的完整哈希在 `target/local-ci/io/fuse-tools-20261125/inputs.json`，工具源码／包／产物来源在 `build.json`。
- 开始时产品基线为 `71cdad49`、Runtime 为 `26384a8`，冻结 runner 包含其上的 FUSE WIP。运行期间另有共享主线提交进入，记录结束时产品为 `d84dd19d`、Runtime 为 `76f8602`，产品 runtime pin 已由该提交配对更新。本批没有改写这些提交或更新 pin。
- `audit.json` 记录测试后 HEAD 和 11 个已记录 Runtime 文件的变化；冻结输入哈希仍全部匹配。本轮证据对应上述 runner，不能作为新主线或其上 WIP 的构建／行为验收。初始脚本原文按运行前哈希校验后保存在 `inputs-initial`；补充选集的脚本哈希另存 `clients-inputs.json`。

## 场景与结果

低层工具使用 `source=/work/source,cache=auto,xattr`，另有请求 `writeback` 的配置；高层工具使用 `default_permissions`，通过 `/work/view/work/source` 访问同一个 tmpfs 源目录。原版工具使用自身的 `-s` 单线程模式，没有修改工具的调用算法。

| 范围 | 结果 | 实际执行时间 |
| --- | --- | --- |
| Linux ARM64 原生，低层／请求 writeback／高层三种配置 | 3／3 整体通过；每种包含 BusyBox、五类客户端及卸载／退出 | 合计 0.709 秒，不含 rootfs 准备 |
| Trames，三种配置 × 1／2／4 CPU | 0／9 整体通过；六个低层启动失败、三个高层 BusyBox 阶段 panic | 合计 131.112 秒，含启动和 rootfs 导入 |
| Trames 高层工具，独立五类客户端 × 1／2／4 CPU | 15／15 功能检查通过；另有 9／9 卸载／退出检查通过 | 合计 33.097 秒，含启动和导入 |

五类普通客户端为：

1. 共享映射修改与 `MS_SYNC`、私有映射隔离、截断后的 SIGBUS，以及关闭 FD／删除名字后原映射仍能同步。
2. pipe 的已接受字节跨两个尚未访问的 FUSE 映射页输出；确认原 pipe 只消费一次，映射修改可回读。
3. `fstat`、`FIONREAD`、`timerfd_gettime`、`uname` 向跨页映射输出，并检查内容；未对未对齐指针做 C 类型解引用。
4. 从真实 FUSE 文件提交 Linux AIO，输出到另一个 FUSE 文件的跨页映射；核对原事件身份、字节数和内容。
5. 8 MiB 写入／fsync／逐块读取与内容校验。

BusyBox 场景含 mkdir、cp、cmp、硬链接／符号链接、cat、chmod／stat、mv、find、tar 打包／解包及删除。高层完整场景在复制过程中中止，不能把该场景的前缀动作计为通过。补充客户端选集省略 BusyBox，其他客户端 ELF、参数和断言与原生已通过项一致。

## 集中修复输入

1. **FUSE 设备的阻塞 splice 未进入原资源等待。** 低层工具完成 INIT 后，对原 `/dev/fuse` 与内部 pipe 的阻塞 `splice(..., flags=0)` 得到 `EAGAIN`，随后原版 libfuse 结束循环并卸载。两 CPU 轨迹明确记录 syscall 76 返回 `-11`，另外四个低层组合呈现相同的 INIT 后退出／ready 失败。初次“客户端看不到挂载”的表象不是已证明的挂载命名空间复制错误，不据此增加拓扑刷新。需统一接通原设备／pipe 的阻塞等待、订阅、非阻塞分支、关闭／取消及必要的反向传输调用方。
2. **文件复制的中间 FUSE 元数据回复逃逸到最终 copyout。** 高层 BusyBox 阶段在 `direct/file_copyout.rs` 的 `FUSE metadata output` 断言退出。两 CPU 详细轨迹显示复制 syscall 71 正在等待源文件 GETATTR；回复被当作最终复制结果交给 copyout。三个 CPU 配置均出现该断言。需让复制状态保留中间回复与已接受的原 OFD、偏移和字节，继续原复制步骤，再完成最终返回；同时审计 sendfile／copy_file_range／splice 的相关调用方、短传输、布局等待与取消，不能只将断言改成错误码。

以上为同一轮收集的两个阻塞，尚无修复后通过结论。详细轨迹为 `diagnose-{lowlevel,highlevel}.stderr`，诊断阶段 28.458 秒。

## 证据、限制与下一步

- 原始证据根目录为 `target/local-ci/io/fuse-tools-20261125`；`native-results.json`、`guest-results.json`、`clients-results.json` 保存每次命令、退出码和耗时，stdout／stderr 与导出的 daemon trace 分开保留。`audit.json` 重新核对所有成功标记、失败类型和冻结输入。初次 guest 外层 stage 返回 0 只表示收集器跑完；九个子场景实际失败，以子结果与审计为准。
- 每个客户端有独立 20 秒上限，传输为 60 秒，ready／卸载／退出等待分别有限期，外层每配置 180 秒。此轮没有外层超时、磁盘停止或遗留测试进程。原生运行在独立挂载／网络／PID 空间和临时 root 中，没有重启共享 VM、修改全局挂载或清理其他任务产物。
- 原生确实打印 `lo_init: activating writeback`；guest 没有协商这项能力。请求了 `writeback` 配置不能等同于 guest writeback-cache 已通过；共享 mmap 同步的通过结论单独保留。扩展协商能力和真实 writeback 工具验收仍开放。
- 当前是 debug 解释器。8 MiB 的读写时间仅用于诊断，不作发布版吞吐或普通负载性能验收；工具构建的条件宏也不构成 fallocate／copy_file_range 等可选回调的单独通过证据。
- 本轮没有重跑全量 gVisor，也不增加既有累计通过身份；上一检查点的 339 项、331 通过／8 跳过及全部 330 个原生通过项仍只对应其冻结版本。
- 沿用产品与 Runtime 的现有 target cfg／条件依赖隔离 Apple 类型、API 和链接项。本轮仅增加 Linux guest 测试夹具及记录，生产 Rust 未由本批改动；后续修复以最新共享主线整合，在实现完本批相关调用方后统一验证 Apple／Linux，继续遵守不新增协调／服务线程的约束。

下一实施批次先集中处理上面两个阻塞及同路径依赖，再验证受影响库、原版工具与相关 gVisor 身份。随后继续 release 吞吐／普通负载测量、最终平台／产品验收及配对提交推送；不据客户端选集通过关闭 02 或跳到下一领域。
