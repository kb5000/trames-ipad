# 真实调试工具实施批次（2026-11-27）

领域 06，接续 Runtime `4fb6a45` 的 Android 内存收尾。先完成本领域实现、原版调用方和必要回退，再集中验证；不新增生产协调／服务线程或轮询消息总线。使用现有 Linux ARM64 参考、解释器和 Mac HVF，Apple／Linux 路径同时检查。

## 实现与调用方

- ptrace 控制和 PEEK／POKE 保留原 tracer 任务身份，不将权限扩展给其线程组里的兄弟线程；普通 wait 的线程组语义保持独立。依据 Linux `ptrace_check_attach` 和 `ptrace_access_vm` 的原 parent/current 检查。
- 非 leader exec 交接原观察者时，先完成 trace slot 修改和解锁，再派发等待通知、退役回调和原 stop acknowledgement；避免 trace slot 与运行队列的锁逆序。
- SEIZE 新生任务使用 `PTRACE_EVENT_STOP/SIGTRAP`；远程内存服务唤醒 tracee 时仍保留 syscall-entry 的待执行 SVC，直到 tracer 恢复，避免提前 exec 使原版 strace 漏记父线程。
- 原版 GDB 的 live attach、多线程、追踪期间 clone、读写内存与寄存器、实际 CPU 单步、detach；原版 gdbserver 的 attach、launch 和 detach；调试器异常退出、目标正常退出／停止时被杀、拒绝访问；原版 strace 跨 fork／exec 及非 leader exec。
- 夹具使用同一 ARM64 ELF 和工具 rootfs，完整记录工具／输入 hash 和所有会话。GDB 使用命令文件，避免将多行 commands 块误作单条 `-ex`。每个进程组有界清理，各独立失败都收集，不以首例失败跳过后续会话。

## 集中验证计划

- 原资源回归覆盖精确 tracer 所有权和观察者通知解锁边界；所选原版 ptrace／wait／exec 用例按原身份比较。
- Linux、解释器与 HVF 各运行完整 14 个原版工具会话；两执行后端另复核单执行槽下的多线程、clone、远程调试、detach、异常退出和非 leader exec。
- 只重跑受影响库、组合及平台构建，复用未变结果；保留失败与修正后的证据，不重复累加既有 gVisor 全量台账。
- 硬件断点／观察点、SVE／SME 继续单独登记后端能力；不由软件会话通过推断这些功能已支持。未运行的物理设备验收不记为通过。

## 集中验证检查点

- 已完成第一批原 tracer 身份与非 leader exec 通知解锁修正；真实工具发现的 SEIZE 新生任务错误 SIGSTOP 也已合入第二批。
- `PTRACE_SEIZE` 自动追踪的子任务现在以 `PTRACE_EVENT_STOP/SIGTRAP` 开始；原有 ATTACH 模式保留 SIGSTOP，已有组停止状态仍独立处理。
- 原生 Linux 与两端使用相同的签名 Alpine GDB 15.2／gdbserver 15.2、strace 6.13、同一 ARM64 夹具。gdbserver 实际绑定通配地址，验收使用已有隔离虚拟网络；无需改变产品网络策略。
- GDB 在多线程目标被杀后可能先报 ESRCH 再异步报告 SIGKILL；原生 Linux 日志已复现。夹具等待终止事件后再发 quit，保留原工具错误与退出状态，不用交互命令时序覆盖事件。
- 最终 Linux／解释器／HVF 各 14/14；两后端单执行槽八项各 8/8。原版 gVisor 各 165 通过、1 原始跳过；解释器最后只重跑受影响 38 项、复用其余 128 项，不增加全量台账身份。
- Apple／Linux 内核各 204 项通过，Linux session 11 项通过；产品组合 547 通过／3 忽略。两宿主受影响库严格 Clippy、Runtime 格式与边界、解释器／签名 HVF release 和 iOS ARM64 Rust archive 构建通过。Mac 配套网络参数使用另一轮已发布的 `f620fa2`，没有覆盖其余领域改动。

状态：领域 06 按本轮约定范围完成验证，随 Runtime pin 配对提交。完整证据及支持边界见[验收记录](../../runtime/docs/validation/debugger-sessions-20261127.md)。下一领域为 07 futex2／rseq。
