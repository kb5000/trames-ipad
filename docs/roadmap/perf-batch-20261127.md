# guest perf 工具链实施批次（2026-11-27）

08 接续 Runtime `8759063`。本轮先实现本领域当前可行的生产路径、调用方与回退，再集中验证。不新增协调／采样／定时服务线程，不轮询消息总线；共享接口保持 Apple／Linux 中立。

- 接入真实执行分配、换出、CPU 迁移与 guest 页准入的软件生产者，支持 CPU/task-clock、context-switches、cpu-migrations、page-faults/minor/major，以及已有 dummy。无原始生产者的硬件 PMU、alignment/emulation、BPF-output 计数继续明确拒绝。
- 实现 task、task+CPU、CPU-wide、cgroup+CPU 作用域；绑定原任务／CPU／cgroup 资源，保留 ptrace、perf paranoid、初始 namespace 能力与原 FD 检查。cgroup 同名重建／任务迁移不重定向已打开资源。
- 区分 enabled／running 时间；原版继承、组控制、输出重定向、关闭／退出／exec 仍只完成一次。
- 使用现有后端执行定时器与解释器有界执行片段，为采样限定到期 skid；接入频率／CPU 开销预算，停用后撤销检查。原生函数保留真实边界，不伪造历史 IP；兼容内核没有 guest EL1 PC 时明确记录未解析 IP。
- 补原版 perf record 所需 COMM／MMAP／FORK／EXIT 元数据，复核 ring mmap、拆分、dump、跨 mm alias 和动态额度／权限。原始事件 producer 运行在原执行者／资源边界。
- 准备原版 ARM64 perf stat／record／script、原版 gVisor（若有对应项）、Linux ABI 参考和同 ELF 工作负载，统一验证两 CPU 后端／单槽／多 CPU。固定工作量对照无事件、计数和采样开销；不重跑未改动领域。

状态：选定软件范围实现与集中验收完成。原版工具、Linux ABI 对照、两后端单槽／多 CPU、两宿主严格检查及 iOS ARM64 构建通过；最终性能同时核对工作量和采样密度。普通高频 futex 约 3%（5–6ns）实测差值保留，不宣称零开销。硬件 PMU／callchain 等保持未支持；详见[验证记录与完整回执](../../runtime/docs/validation/perf-20261127.md)。Runtime `10dcf23f` 按 main 配对交付，iPad／Mac 产品同步 pin。
