# iPad kernel conformance CI 修复（2026-11-26）

本轮修复测试的等待、时钟比较及精确退休前提，没有修改生产执行、syscall、内存借用或调度路径。Runtime 为 `2c5cc31ad139091de7fdb23c66090ec682d86b29`；iPad 配对 pin 与定向工作流为 `229838ae74020a8a2c0b6c3ba9ca987cc2131688`。

## 原因与修复

- 原 `040b2506` 的 ARM64/QEMU job 为 **524 passed / 18 failed / 3 ignored**。多项有限程序调用测试用的 `parallel_round`，10 ms 到期就收回 cohort，却又要求 guest 已完成所有调用、写出标记。在较慢的 QEMU 下，该前提不成立。改用既有的 persistent epoch，等实际 guest 阻塞、退出或 fault 边界；保留单 cohort、原 worker、返回值、内存内容及不回放 syscall 等断言。
- 两项取消测试在 worker 启动前就可能耗尽 10/20 ms 窗口。测试现在先观察 guest 的真实进度，再取消；纯计算用例仍没有 guest syscall。精确预算 1、17、65,539 的计数断言保留，并增加 5 秒失败看门狗及阶段诊断；等到看门狗才收到完成也会失败。
- `times(153)` 返回随时间增长的 USER_HZ tick。原 ABI 对照要求两次调用完全相等；现在两条路径共用时间起点，并要求后一次返回落在前后观察区间内，错误值继续精确比较。
- VM 进度／隔离测试将 100 ms 当成固定调度保证。改为保留实际进度、应暂停域与无关 VM 继续运行的断言，5 秒只作为失败看门狗。
- 后续 `66cb4a45` 在进入 QEMU 前先遇到 native 日志读写测试失败：要求精确退休 6 条，实际在 4 条时被按宿主时间抢占。与相邻同类测试一样使用既有的 test-only `Epoch::unpreempted`，继续核对精确退休、退款、park/resume、输出和无 syscall 重放。
- 扩大检查后还发现统计测试固定在 200 ms 取消，并假定所有 4/12 个任务都已运行。现在每个任务完成一次原始 sysinfo 后，通过原 console 写一个唯一就绪字节，再继续原 sysinfo 循环；宿主等全部任务就绪且 live CPU tick 实际增长后取消，不读取正在执行的 guest 内存借用。

这些辅助逻辑全部位于 conformance 的编译隔离中。四个含生产实现的文件，其 test module 之前的字节与 Runtime 父提交完全相同；另外 12 个文件均由 `cfg(all(test, trames_interpreter_conformance))` 引入。没有在生产热路径增加探测、检查、屏障、计数器或线程。

## 源码身份与集成

初版本地冻结基于 iPad `66cb4a45`、Runtime `7f80b14`、CLI `8dd30e4`，只覆盖本轮测试文件。后续四项冻结更新到 iPad e962／Runtime c56 加两文件测试补丁，最终 Rust 测试字节与 2c5 一致。冻结未纳入其他会话尚未提交的 Swift、allocator、调试、存储或安全审计工作。

验证过程中，现代挂载迭代独立发布了 Runtime `2351187`／iPad `d262c3a3`。其三个生产文件变更为页面范围 helper 的内联提示，以及将既有挂载快速标记用于默认配置；验证见 [现代挂载收尾记录](../../runtime/docs/validation/modern-mount-closeout-20261126.md)。本轮保留该提交，在其后仅修改 16 个测试文件。**本地 7f80 快照和初次 235+c56 集成来源是不同来源，不混称同一个受测源码。** 云端完整检查先验证 iPad `e962b804`／Runtime `c56e544`／CLI `8dd30e4`：两宿主原生均为 542 passed / 3 ignored；QEMU 为 541 passed / 1 failed / 3 ignored，原先 18 项均通过。新增暴露的退出发布用例仍使用短截止。继续修复此项，并审阅同类调用，补充 prctl 的有限完成等待，以及两个需要先观察实际 syscall／seccomp 进度的有界轮次用例。

最终 `c56e544 → 2c5cc31` 只改变两个测试文件内的四个函数。源码逐字节验证这四个函数以外无变化；使用精确重跑复用每条路径 **538 个未变通过项与三个原有忽略项**，重新执行四个变化项。最终云端来源为 iPad `229838ae`／Runtime `2c5cc31`／CLI `8dd30e4`，产物中三个 commit 文件逐项核对。最终覆盖结果由完整来源与精确复验合并；最后版本没有重新完整执行 545 项。

## 验证

表中的“原生”指宿主直接执行 Rust 测试程序，内部使用解释器 conformance；QEMU 再承载 ARM64 musl 测试进程。

| 来源 | 结果 | 范围 |
|---|---|---|
| 本地 ARM64 Linux musl 原生 | 542 passed / 3 ignored | 原完整通过结果复用；后续变化项按相应源码精确重跑通过 |
| 本地 QEMU 10.0.13，V3 完整并发执行 | 542 passed / 0 failed / 3 ignored | 104.68 秒，7f80 加初版修复；后续四项另按集成源码复验 |
| 本地 Apple | 542 个不同用例通过 / 3 ignored | 初次 525 通过；17 个本机 socket 用例因沙箱 EPERM 失败，同二进制开放回环访问后仅重跑这 17 项通过；后续变化项按相应冻结源码精确重跑 |
| 本地最终格式／严格 clippy | 通过 | 冻结的 kernel-conformance target，无生产依赖重写 |
| 云端 Linux amd64 原生 | 542 个不同用例通过 / 3 ignored | 538 个未变通过项复用＋四项精确重跑 |
| 云端 Apple 原生 | 542 个不同用例通过 / 3 ignored | 538 个未变通过项复用＋四项精确重跑 |
| 云端 ARM64 musl／固定 QEMU | 542 个不同用例通过 / 3 ignored | 538 个未变通过项复用＋四项精确重跑 |

[完整来源 CI](https://github.com/project-trames/trames-ipad/actions/runs/45767694663) 与 [最终四项精确重跑](https://github.com/project-trames/trames-ipad/actions/runs/45770641227) 为手动入口，执行两宿主 conformance 格式、严格 clippy 与原生测试，再在 Linux 下执行固定版本 QEMU 的 ARM64 musl conformance。保留原并发执行及错误／内存内容检查，未新增忽略项；times 的动态返回按上述前后区间验证。原有三个忽略项不计作通过。归档时同时验证了 worker stderr 插入 libtest 结果行的真实情况；结果解析器按测试记录重建并核对汇总。六份云端真实日志、零匹配及含混／缺失结果负例均验证通过，未因此重新运行 guest。选择入口只接受精确 Rust 测试名，以结构化参数传给 Cargo；零匹配即使 Cargo 返回 0 也判失败，避免空跑变绿。非法名称及真实零匹配的负例均验证通过。

中间失败记录也保留：第一版本地 QEMU 在统计用例的旧窗口失败，剩余精确预算用例出现异常等待，382 秒后明确终止，该轮不计作完整通过。随后预算阶段的单项及额外并发诊断通过；补充每任务就绪通知和显式失败看门狗后，最终完整 QEMU 执行通过。看门狗会将今后同类等待报告为失败；不据一次完整通过宣称排除所有调度竞态。

## 复核与资源

[源码范围](runtime-conformance-20261126/source-scope.json)、[原始记录](runtime-conformance-20261126/raw-evidence.tar.gz)、[离线审计](runtime-conformance-20261126/audit.py) 和 [审计结果](runtime-conformance-20261126/audit-result.json) 保存原失败名单、有限历史日志片段与原日志 SHA-256、固定源码清单、测试辅助代码前后版本、本地结果及云端实际 pin 产物。离线审计不运行编译或 guest：

```sh
PYTHONDONTWRITEBYTECODE=1 python3 docs/validation/runtime-conformance-20261126/audit.py
```

复用现有 Linux VM、QEMU、guest CLI 和编译缓存，没有新建 VM 或下载镜像。清理了 57 个只属于本轮私有快照的可再生成编译输出（627,467,651 逻辑字节）、两份哈希核对的宿主临时源码副本，以及 VM 中的失败中间二进制和临时源码；保留最终本地测试二进制、源码清单和日志。其他项目、原有冻结 KVM 性能 runner 和所有者的工作保持不动。

本记录范围为 conformance CI；产品发版、物理 iPad、GPU／音视频及系统级应用验收沿用各自独立台账。
