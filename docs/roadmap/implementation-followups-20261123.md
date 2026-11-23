# 临时实施清单：兼容性迭代后的剩余工作

日期：2026-11-23。用途：作为后续实现、定位和验收的工作队列；完成一项后在本页更新状态并链接验证记录。这里保留待办和完成标准，具体设计在对应批次开始时结合源码和目标负载确定。

## 当前基线

- 已对 Mac HVF、Mac 上运行的产品解释器和原生 Linux 重跑固定的 254 组、14,530 个启用用例。采用记录中的上游测试模式、网络夹具、CPU 前提和单例复核后，两端均通过全部 13,965 个原生通过项。
- 旧的 ptrace、proc memory、PIDFD_THREAD、UserChainTargetCrash 测试缺口已关闭。早期报告中的相关失败、解释器测试夹具问题和 mount 条件跳过，以后续记录及本次全量结果为准。
- 这套测试集的闭环不代表下列功能已完整实现，也不代表所有真实程序、长时间运行及真机行为都已验收。

依据：[全量验证](../../runtime/docs/validation/gvisor-full-20261123.md)、[syscall 路线图](syscall-implementation.md)。

## 推进顺序

主线按 **批量异常归因 → FUSE → 现代挂载 → cgroup／Android 内存** 推进。真实调试工具验收可以穿插进行；随后安排 futex2／rseq 和 perf。动态任务上限、allocator、存储和内存测量保留为后续任务，涉及方案选择的项目先测量、再定设计。

本页作为进行中的工作队列。每项区分实现缺口、待定位现象和待验收能力，不凭一次单例重跑将稳定性问题关闭。

## 实现、定位与验收队列

### 01 — 批量运行异常归因

- [x] 状态：已完成定位、修复和本地验证；原始首跑及失败证据保留。
- Unix datagram：原始单例重复 30 次复现两次异常。实际阻塞发生在 `mmap` custody 等待：映射生命周期先发布任务、后释放 custody，且终止判定漏计仍在完成的源处理。现已提前释放 custody，并用现有 `completing` 计数覆盖完整处理；定向回归恢复旧释放顺序时失败，修复后通过。
- Socket 压力：同样的混合批次在原生 Linux 三轮中也有 11 项失败，确认缩小的临时端口池与 TIME_WAIT 会污染后继 listener。两端 harness 对 `socket_stress` 逐例隔离，并记录夹具指纹；未改上游断言。
- inotify：原始用例创建 1,000 个目录和 watch 后还需运行五秒；已通过的单例耗时 22.6 秒，超过首跑约 18 秒预算。该用例显式预算设为 60 秒，保留用户 suite 超时覆盖优先级。
- 验证：Unix 原始用例解释器单例 100 次、连续批次 50 项、Mac 批次 25 项均通过；两端完整 30 例 socket 压力组和 inotify 复核通过。全部 238 项 Trames 通过结果再次核对原始 XML、准确用例身份和退出状态。
- 配套检查：Runtime 八阶段本地 CI、iOS FFI 编译、Mac 签名构建、六项生命周期测试、19 项 harness 测试通过。完整解释器检查 504 通过、3 忽略；另修正 host pause 的采样确认和共享内存观测测试的有限预算假设，并加强实际静止／执行完成断言。四组正常负载交替性能对照的中位数变化均在 ±1% 内。
- 证据：[批量稳定性验证](../../runtime/docs/validation/batch-stability-20261123.md)、[历史全量报告](../../runtime/docs/validation/gvisor-full-20261123.md)。原始计划、XML、诊断、正反回归和性能数据位于 `target/batch-stability-20261123`。

### 02 — FUSE 完整文件系统操作

- [ ] 状态：实现缺口，另有吞吐和真实程序验收缺口。
- 剩余范围：目录枚举、创建／删除、rename、通用符号链接遍历，以及通过 FUSE 文件句柄的文件映射。
- 当前数据请求上限为 4 KiB；评估扩大请求及并发请求时，验证短传输、取消、关闭和页生命周期。
- 完成标准：上述操作与 guest daemon 正确交互，覆盖失败、并发及卸载／退出；选定真实 FUSE 程序做端到端验收，并测量实际文件传输吞吐。SSHFS、`rclone mount` 或 Android 存储路径须分别有证据后再声明支持。
- 依据：[FUSE 文件操作记录](../../runtime/docs/validation/gvisor-fuse-files-20261121.md)。

### 03 — 现代挂载剩余语义与 OCI 验收

- [ ] 状态：实现＋实际工具验收。
- 剩余范围：ID-mapped mount、`nosymfollow`、通过 `mount_setattr` 修改传播属性，以及 `fspick`／文件系统重配置。
- `mount_setattr` 的已验证递归属性修改，以及 `statmount`／`listmount`／原版 findmnt 查询，不再按未实现项处理。
- 完成标准：挂载局部 UID／GID 映射贯穿权限、所有权查询和创建；检查 namespace 权限、共享／私有存储及失败回滚。重配置正确影响同一文件系统的别名，并保持各挂载属性独立。用实际 runc／OCI 配置验收对应启动路径。
- 依据：[挂载属性](../../runtime/docs/validation/guest-mount-setattr-20261123.md)、[挂载查询](../../runtime/docs/validation/guest-mount-query-20261123.md)、[路线图](syscall-implementation.md#2-modern-mount-administration)。

### 04 — cgroup 资源管理补全

- [ ] 状态：实现＋记账和压力验收。
- CPU／cpuset、匿名内存控制、OOM group 等已有实现；剩余重点是共享 page cache、私有 COW 页的归属与计费，以及内存回收行为。
- IO controller 尚未实现：当前 `io.max` 提供空视图并拒绝写入，不能将其作为已生效的 IO 限流。
- 完成标准：覆盖共享、fork／COW、迁移、退出和祖先限额的计费与释放；在真实压力下验证计数、限流／回收及 OOM 行为。IO 控制按选定实际负载实现并验证限流效果。
- 依据：[cgroup 控制器记录](../../runtime/docs/validation/gvisor-cgroup-controllers-20261123.md)、[当前 cgroup 实现](../../runtime/crates/trames-vfs/src/cgroup.rs)。

### 05 — Android 内存生命周期

- [ ] 状态：待实现，并需绑定目标 Android 镜像验收。
- 剩余范围：`userfaultfd`、`process_mrelease`，以及 ART／`lmkd` 实际选择的相关路径。
- 先确认目标镜像中的 collector、reaper 和调用参数，再确定必须支持的 ABI、故障事件和回收范围。
- 完成标准：覆盖共享地址空间、并发退出、重复调用、unmap／remap 和故障挂起时的终止；原版 Android 应用压力负载确实使用预期路径。测量 GC 暂停尾延迟及应用终止后的内存下降，并补 Mac 与物理 iPad 验收。
- 依据：[Android 内存路线图](syscall-implementation.md#3-android-memory-lifecycle)。

### 06 — 真实调试工具完整验收

- [ ] 状态：主要为待验收，硬件调试资源仍有实现缺口。
- 已验证：真实 GDB 启动、软件断点、单步、寄存器修改和继续退出；对应 ptrace／proc memory 原始用例已闭环。
- 待验收：GDB live attach、多线程会话、gdbserver，以及 `strace -f` 跨 fork／exec；包括拒绝访问、被调试任务停止时调试器退出等生命周期。
- 硬件断点／观察点以及 SVE／SME 调试寄存器不在当前实现范围内，应单独登记支持范围和后续需求。
- 完成标准：在两种执行后端上，用原版工具运行上述完整会话并保存输出；平台相关部分补真机验证。
- 依据：[ptrace 实现与 GDB 验证](../../runtime/docs/validation/guest-ptrace-mutation-20261123.md)、[proc memory 验证](../../runtime/docs/validation/guest-proc-mem-20261123.md)。

### 07 — futex2 与 rseq

- [ ] 状态：待实现。
- futex2：`futex_waitv`、`futex_wait`、`futex_wake`、`futex_requeue`，复用现有等待队列及 private／shared key。
- rseq：同时处理线程注册、CPU 身份、迁移、抢占、信号中止和线程退出；恢复原始 no-libc 测试依赖后再作为覆盖依据。
- 完成标准：Linux 自测及真实调用程序通过，覆盖唤醒／重排／超时／信号竞态和共享别名；两种 CPU 后端语义一致，并验证旧 futex 正常路径性能。rseq 的优先级结合实际程序收益确定。
- 依据：[线程同步路线图](syscall-implementation.md#4-thread-synchronization)。

### 08 — guest perf 工具链

- [ ] 状态：待实现。
- 实现 `perf_event_open` 的选定 guest 软件事件、事件 FD、读／控制操作，以及支持的采样环。
- 完成标准：原版 `perf stat`／`perf record` 对任务 CPU 时间、上下文切换等选定事件可用，计数及采样可核对。硬件 PMU 需后端能够准确归属 guest 并通过测量后再宣布支持。
- 依据：[性能工具路线图](syscall-implementation.md#5-guest-performance-tooling)。

## 计划、优化与测量任务

### 09 — 动态任务上限

- [ ] 默认 65,536 已完成；可写 `/proc/sys/kernel/threads-max` 与启动配置接口待实现。
- `[kernel.sysctl]` 仍是拟议 schema。启动值和运行中写入应作用于同一个环境容量；`pid_max`、`RLIMIT_NPROC`、cgroup `pids.max` 保持独立，不预分配资源。
- 完成标准：读写、权限、持久配置初始化及任务准入行为一致，两端均有验证。
- 依据：[已有计划](../guides/ipad-environments.md#task-capacity-and-planned-live-configuration)。

### 10 — 原生 allocator 性能

- [ ] 目前为默认关闭的实验路径；短调用和竞争分配负载曾出现倒退。
- 完成标准：使用相同工作量对照原路径，定位通信、访存、分配和竞争成本；保留原算法同步责任。只有测量支持时才扩大实现或调整默认开关。
- 依据：[原生 allocator](../architecture/native-allocator.md)。

### 11 — 存储持久化性能

- [ ] 小写入＋完整 fsync 的性能继续评估；WAL 只讨论过，尚未实施或证明收益。
- 完成标准：先建立等持久性语义的基准，覆盖单／多写者、包安装、容器层、P99、写入量和崩溃恢复；根据结果决定是否采用 WAL、打包或其他布局。
- 依据：[私有存储同步](../../runtime/docs/architecture/private-storage-sync.md)。

### 12 — 空闲内存基线

- [ ] 尚缺统一的冷启动基线及物理 iPad 测量。此前运行中 Runtime＋Debian shell 的观察不能直接作为干净安装基线。
- 完成标准：固定 rootfs、启动方式、CPU 和组件配置；分别记录 Runtime／guest 与管理器，并区分 RSS 和 physical footprint。覆盖启动峰值、稳定空闲和停止后的释放。

## 每批交付规则

1. 开始时核对源码与已有后续记录，明确本批范围、依赖、触发程序及完成标准。
2. 一批完成相关子系统、依赖和调用方回退路径；等待释放执行槽，遵守现有 guest 内存、并发及对象级同步原则，不新增协调／资源服务线程或轮询消息总线。
3. 按改动范围运行共享 Runtime、Mac 和解释器验证，补 gVisor 未覆盖的真实工具／负载；平台相关项目做真机验收。对照受影响正常负载的性能。
4. 复用未变源码的通过证据；文档、记录及测试等待方式调整不触发完整 CPU、Linux、网络、图形重验。完整检查及全量 gVisor 放在相应集成／发版节点。
5. 更新本页状态、验证记录和遗留限制，提交并推送 `main`；涉及 Runtime 时同步两端配套引用，保留已测试提交身份。

本清单之外的 syscall、CPU 扩展及产品功能继续由各自路线图管理。此处不把所有未实现接口自动提升为本轮必做项。
