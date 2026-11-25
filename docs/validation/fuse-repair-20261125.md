# FUSE 检查点失败集中修复（2026-11-25）

前一[检查点](fuse-followup-checkpoint-20261125.md)的 20 个失败身份均已通过；本轮另外发现的偏移唤醒竞态及信号夹具旧预期也已处理。受影响原版 gVisor 339 个身份为 331 通过、8 跳过，覆盖全部 330 个 Linux 参考通过项。

这是 02 FUSE 领域内的修复与回归检查点。真实 FUSE 工具／吞吐、最终平台与交付门槛仍开放，02 和完整 01–12 目标均未关闭；本批未提交、推送或更新 Runtime pin。

## 源码与产物

- 产品主线 `71cdad49c1ab16480b7a259f7d95a03085fb9a94`，Runtime 主线 `26384a8746933ea636888f57ac49e62d978a6612`，已提交 pin 配对相符；实际验证包括其上的未提交候选。
- 原始证据目录：`target/validation-fuse-repair-20261125-1540`。Linux 镜像位于 `target/local-ci/io/validation-fuse-repair-20261125-1540`，沿用本任务独立构建目录，没有改动另一条 Linux 宿主开发任务的环境。
- `initial-source.json`、`compile-source-02.json`、`test-source.json` 保存集中修复及编译诊断处理阶段；`refined-source.json` 保存最后三项一起处理后的候选。`refined-delta.json` 明确只有 FileWork／NamespaceWork、确定性交错测试及信号测试发生变化。
- `phase-one-source-comparison.json` 与 `refined-source-comparison.json` 分别确认两次有效测试期间已记录的文件没有变化。HEAD 不能代替这些 WIP 哈希。
- 解释器产物 `trames-run` 从本轮成功构建复制后固定，SHA-256 为 `604919298103427c616774584511504f4689a0b31e353acadf1f47d25f4d5d46`，构建及来源见 `runner.json`、`interpreter-build.json`。

## 实现与问题边界

1. FUSE 路径和 NamespaceWork 的 syscall 外层 ESTALE 重查，统一回到原 live namespace 的实际挂载对象。原 root／cwd、挂载 registry 和新 walk 不再因复制投影而拥有不同的拓扑身份。原文件名／凭据、fixed aliases、数值 FD 的显式重查、一次性预算及 open 预留槽位继续保留。
2. FileWork 等待同时订阅原 position 与实际 page／proc 资源。旧实现按订阅当刻是否 busy 选择其中一类，可能在“当时空闲、ready 检查前被另一读者占用”的交错下漏掉唤醒。修复只改变已等待操作的订阅，未为普通读写增加屏障、轮询、全局锁或执行线程。
3. 已取得 native admission 的同步任务在原布局执行资源结束后仍执行已接受的工作；尚未提交的队列任务仍可结束。真正需要异步重排的 provider 释放自己的原回执。Notification 保存终态，完成后的 retirement 不覆盖原结果；回调在释放 admission 后执行。BackgroundSync 与 SyncTask 的两个调用方均已接通。
4. 已完成 copyout 的旧别名再次使用时返回原有 EINTR，不重新执行或按复用后的数字 FD 累计 timer／syscall 完成。inotify 控制在已接受的原 OFD 上完成类型错误；io_uring 的普通／固定缓冲区信号读取和 poll 使用原 submitter 的 byte／reader 能力，不能由后来的 CQ 观察者提供权限。
5. ARM64 sigreturn 的故障地址按[内核原实现](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/kernel/signal.c)使用 scalar 恢复后的 SP；读取 SP 失败时为零。修正上一批始终使用 frame 地址的错误判断，保留已恢复 GPR／mask，并增加单字段 SP 失败后的 PC／pstate 继续恢复回归。另一 worker 夹具也明确区分未对齐 frame 的早期失败与对齐坏地址在 scalar 恢复后的失败。
6. FUSE 夹具补齐真实 FLUSH 回复，再核对 OFD／名称／inode 最终释放；FORGET 按准确 node 和计数核对，不要求没有协议承诺的析构顺序。CREATE 的 mode／umask 断言保留并增加最终关闭的引用核对。同名 bind 同时覆盖成功与目标被替换后因 unhash 而拒绝的分支，符合 [d_set_mounted](https://github.com/torvalds/linux/blob/v6.12/fs/dcache.c) 的准入；未把第二次源 lookup 替换为第一次目标。
7. 手工 masked-wait worker 夹具绑定原 IoTask／TaskSignal；PROBE 夹具清零完整输入，保留 pin／ring 控制断言。静态诊断集中修正，冷等待对象继续按现有设计内联；没有为了大小 lint 添加额外分配。Apple 专用接口仍由既有 target cfg／条件依赖隔离，新增共享路径没有引入 Apple API。

## Rust 验证与复用

| 范围 | 最后实际运行 | 按身份合并后的范围 |
| --- | --- | --- |
| Mac VFS／Linux 调用方 | 1609 通过、0 失败、2 忽略 | 加本批 Kernel 203 通过，以及未受影响的 Memory 248 通过／3 忽略，共 2060 通过、0 失败、5 忽略 |
| Linux ARM64，同上 | 1617 通过、0 失败、2 忽略 | 加 Kernel 203 通过、Memory 249 通过／3 忽略，共 2069 通过、0 失败、5 忽略 |
| 产品解释器接入／FFI | 565 通过、0 失败／中止、7 忽略 | 加五个未变 CPU／compute 目标的 55 个通过身份，共 620 通过、0 失败／中止、7 忽略 |

这些不是新一轮 Runtime／产品全工作区数量。合并排除文档测试，按目标和身份覆盖旧结果，不把重复运行累加。Memory 仅修改 SyncResource 的说明，按工程约定复用原三个目标；Kernel 在最终窄改中源码未变；独立 CPU／compute 目标未受本批修改影响。

完整来源见 `mac-merged-results.json`、`linux-merged-results.json`、`product-merged-results.json`。每个目标记录原始结果文件和复用范围，所有新运行的身份／计数核对无异常。`closed-checkpoint-failures.json` 逐项确认原 20 个身份均为 passed，没有通过改名、忽略或中止后的遗漏关闭它们。

初次统一运行曾保留两端相同的 mkdir／并发 offset 失败，以及产品另一项信号预期失败；三项一起处理后才进入上述窄范围统一复查。所有失败日志和阶段源码均保留。最终 VFS／Linux 调用方有效阶段耗时 Mac 68.273 秒、Linux 66.803 秒，产品 122.405 秒，均含编译，无超时或磁盘停止。

两个宿主各五库的全目标／全功能严格 Clippy 通过，最后修改后的 VFS 又在两端检查通过；仅保留已有 manifest／vendor 警告。格式和 Runtime 执行／宿主／依赖边界检查通过。顶层干净状态／pin 验收仍属于正式交付节点，不隐藏当前 WIP 来通过它。

新确定性交错测试覆盖订阅时空闲、随后占用、FD 关闭复用、原位置完成唤醒及后续读取正确字节。原四读者 offset 并发测试还在两个宿主各连续运行 20 次，40／40 通过；每次有独立期限、日志及测试产物哈希，不据重复运行增加用例总数。证据为 `mac-offset-stress/results.json` 与 `linux-offset-stress/results.json`。

## 原版 gVisor

同一冻结解释器运行了 aio 14、iouring 26、mmap 118、msync 110、signalfd 19、timerfd 28、fuse 24，共 339 个未修改上游身份。逐用例期限 20 秒、chunk 最大 40、两 CPU、独立临时 store、6 GiB 磁盘下限；挂载 fusectl，使用普通 Linux ABI 模式，没有设置 `TEST_ON_GVISOR`。

最终 331 通过、8 跳过，无失败／中止／超时。复用的原 Linux 参考为 330 通过、8 跳过、1 失败；参考失败项 `FuseTest.CloneFromUnconnectedDeviceFails` 在 Trames 通过，未纳入原生通过项的分母。全部 330 个参考通过项都在 Trames 通过，8 个原生跳过身份没有替代任何参考通过项。

原生证据来源为 `target/local-ci/io/gvisor-full-native-20261123-final`，选集记录在 `native-reference`，每个身份保留原日志位置。原 ELF 哈希逐组匹配；当前 Linux 环境再次核对为同一 `7.0.0-43-generic`，本轮没有重新运行原生用例。`gvisor-comparison/comparison.json` 的严格比例为 330／330；`gvisor-xml-audit.json` 重新打开全部 11 份上游 XML 并核对完整 339 个身份及结果。

本轮解释器执行总耗时 89.471 秒，没有资源停止。此选集与历史全量台账重合，不增加固定集合的通过数，也不作为当前 Mac/HVF 或物理 iPad 的通过证据。

## 仍开放的门槛

继续完成真实 FUSE 工具／应用、映射／写回／取消／卸载的联合场景及吞吐与普通调用性能核对；随后在整批集成节点完成平台检查、实际 HVF／物理 iPad、完整 CI 和配对提交／推送。无 FUSE 暂停的纯 native pathname ESTALE 仍归属 03 的共享路径／现代挂载核对。其余领域按原顺序推进，不因本检查点通过而缩减目标。

后续[真实工具检查点](fuse-real-tools-20261125.md)已使用本页冻结 runner 运行原版 libfuse：确认阻塞 splice 和复制中间回复两类新增阻塞，独立高层客户端的五类场景在 1／2／4 CPU 全部通过。真实工具整体验收仍失败；运行期间进入的新共享主线提交不在本页冻结产物的验证范围内。
