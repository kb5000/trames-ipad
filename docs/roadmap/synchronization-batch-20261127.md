# futex2／rseq 实施批次（2026-11-27）

领域 07 接续 Runtime `167b66a` 的调试工具收尾。先补齐本领域实现、原始调用方、回退及验收输入，再集中验证。不新增协调／服务线程、轮询总线或普通访存屏障；共享 Rust 路径同时覆盖 Apple 和 Linux。

## 本批内容

- futex2 的 U32 private／shared 四个入口共用原 futex 表；按当前 Linux 保留 requeue 的相同 flags 限制，修正负数 wake 计数和带超时 SA_RESTART 行为。其他位宽与可选 NUMA／MPOL 标志保持明确的未支持边界。
- futex2 向量／绝对超时输入和已复制片段属于原调用。异步后备缺页通过现有 PageWait 停放，活值比较与队列发布仍在同一原 bucket 边界；失败／取消不留下可消耗 wake 的票据。调用随原 Job／Done／Process 和 session 返回续行，不交给新服务线程。
- rseq 沿用原线程注册及 CPU／信号／抢占／迁移／fork／exec／退出接线，复核声明的 28 字节 feature／32 字节分配 ABI。恢复固定版本 gVisor 的原始 ARM64 no-libc helper；不以缺失 runfile 后的跳过作为覆盖。
- 原版 Linux futex 自测、gVisor futex／rseq、同 ELF 的竞态／共享别名／生命周期夹具、原版 glibc `sched_getcpu` 快速路径及 fallback、legacy futex 普通负载与争用吞吐，统一准备后集中验收。
- 对照固定 Linux 源码、当前 ARM64 Linux 参考、解释器和 HVF；补单执行槽与多 CPU 条件。未变通过证据复用，不重跑无关领域全量矩阵。

状态：Runtime `8759063` 已提交，本批选定 ABI 与原版调用方集中验收完成。原版 gVisor futex／rseq 在 Linux 对照、解释器和 HVF 各 80/80；两后端单槽 rseq 各 13/13。原版 Linux 18 个程序／参数组合两端全部完成；含 200 线程、两千万次 yield 的两个大压力程序，240 秒硬上限补跑各约 149–151 秒通过。产品 conformance 553 通过／3 原有忽略，Apple／Linux session 各 16/16；严格检查和 iOS ARM64 构建通过。

同 ELF 对照基线 Runtime 167b66a，1／4 CPU 预热后各五次中位数：legacy futex 耗时变化约 −0.4% 至 +0.6%，mutex 吞吐持平或略升。短样本的波动未复现。原版 glibc rseq 快速路径及其官方 opt-out 均实际执行并验收，不以启动注册替代快速路径覆盖。

实现、原版输入、逐项结果、性能及支持边界见 [Runtime 验收记录](../../runtime/docs/validation/synchronization-20261127.md)。下一领域按顺序进入 08 guest perf 工具链；没有遗留本轮已选定的 futex2／legacy rseq 功能阻塞。
