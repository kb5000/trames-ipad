# 2026-11-24 实现批次

最新同步性能与交付检查点：tmpfs 时间／权限／所有权修改错误触发全局持久化已改为原目标文件系统提交，保留原 FD、卸载后 mount、磁盘错误及回滚，不增加协调线程。相同主线 release 对照中同步中位耗时下降约 88%–93%，写入＋同步下降约 24%–34%；全配置对照的写回总耗时均低于既有基线，普通负载最大中位增长 2.22%，短配对对照的最大 9.60% 波动也完整保留。Apple／Linux Runtime 全工作区 CI、产品严格检查、core＋FFI 567 项、iOS 编译及 Mac 接入均通过；原版工具九组、gVisor 485 身份为 475 通过／10 原始跳过，覆盖全部 474 个原生通过项，不新增全量台账计数。新主线 d544939a 集成后，两端受影响四库 1646／1655、core＋FFI 567、Mac 37 及 iOS 编译通过，原版工具和 421 项整组再次通过；最终 2 CPU 同步 0.471 毫秒、写入＋同步 18.412 毫秒，与已修复快照相近。最终短补测普通负载最大增长 7.47%，需后续长样本确认，不用前述旧快照结果替代。本轮累计实现与必要调用方按主线流程配对交付；02 的完整领域／设备验收及 03–12 保持原清单和顺序。详见[同步性能与交付记录](../../runtime/docs/validation/fuse-sync-delivery-20261125.md)。

**最新有界写回批次**：1 MiB 快照、私有 ACK 接续、未发送页错误语义、时间合并、原租约复用及发布当下退出均已整批实现。Mac／Linux 两库 1666／1674 通过，产品按来源合并 565 通过；原版工具九组全过，gVisor 2 CPU 原始整组 411 通过／10 跳过／0 失败，无 CPU 前提替换。release 初轮 38 组及两单元追加对照均有效，8 MiB 写入＋同步 24–29 毫秒，同轮基线 17–18 毫秒，仍多 32%–68%；2 CPU 追加对照仍多约 41%，重新打开读多约 11%。性能门槛仍未通过。下一批先拆分资源处理、入池／唤醒和布局等待成本，再整批处理原布局直接续接及可复用的读输出／帧初始化路径，完成后统一验证。见[本批验证与下一批输入](../validation/fuse-writeback-batch-20261125.md)。02–12 继续按原顺序保留。

**前一整批优化及 release 复验**：冷页合并、重复缓存拷贝、小写批量、原布局快速准入、weak writer 维护和可写 OFD 的挂载引用已集中实现；Apple／Linux 两库 1655／1663 通过，产品 565 通过，原版工具九组通过。受影响原版 gVisor 421 个身份全部重跑，保留 2 CPU 原始完成顺序假设失败，明确 1 CPU 前提后合并 411／10，覆盖原生全部 410 个通过项。release 对照 38 组／364 条记录有效：8 MiB 写入＋同步降至 70–77 毫秒，但同轮基线 18–25 毫秒，仍慢 3.06–4.05 倍；读取接近基线。普通负载最大中位耗时增长 3.34%，未协商写回的传输最大增长 3.04%，部分基线有波动，不将小幅差值宣称为确定性优化。性能门槛仍未通过。下一同领域整批处理有上限的快照、仅推进原字节的 ACK 接续、未发送页错误及租约／时间回传，完成后统一验证。见[优化检查点及下一批输入](../validation/fuse-optimization-20261125.md)。首轮 51–62 倍退化及全部原始证据继续保留；不关闭 02、不切换其他领域。

当前领域为 **02 FUSE**。所有者于 2026-11-25 再次要求“先测一波”，当前冻结功能扩展，开始验证已积累候选；这覆盖下文实施期不运行测试的阶段限制。先集中处理格式／编译问题，再运行受影响测试并汇总失败，不据检查点缩减整个领域或 12 项目标。源码初始指纹与各阶段独立日志保存在 `target/validation-fuse-20261125-0952`。

**前一 release 性能结果**：已按初始源码指纹重建匹配 release 基线，正式完成 38 组／364 条记录。普通负载最大中位耗时增长 0.96%，未协商写回的两种 FUSE 配置最大增长 1.53%；WRITEBACK_CACHE 写入＋同步慢 51.18–61.63 倍，打开状态／重新打开读取也退化，性能门槛未通过。下一同领域整批处理冷页合并、缓存重复拷贝、回写批量／时间、无映射布局调度及 weak writer 注册维护，全部完成后再统一复验。完整数据、失败的缓存复用构建、有效产物及实现输入见[性能检查点](../validation/fuse-performance-20261125.md)。功能通过证据继续保留，不关闭 02 或切换到其他领域。

**前一协商写回结果**：WRITEBACK_CACHE 已接入普通缓存读写、原调用者接续、时间回写及关闭／同步路径；原版工具确认激活，在 1／2／4 CPU 的九组组合全部通过，含 81 项功能与 27 项关闭检查。按身份及范围复用，Apple 两库 1640 通过／2 忽略、Linux 1648／2，产品 565／7，均无失败。受影响原版 gVisor 2 CPU 原始为 410 通过／10 跳过／1 完成顺序假设失败，该单例在 1 CPU 前提通过；按明确前提合并为 411／10，覆盖 410 个原生通过项，保留原失败，未固定运行时完成顺序。源码／产物与逐项来源见[写回检查点](../validation/fuse-writeback-20261125.md)。仍需匹配 release 性能及完整领域／最终平台交付门槛；02–12 未关闭，本 FUSE 批次未提交／推送／更新 pin。

**最新传输／复制修复结果**：原版工具的两个阻塞，以及扩展场景发现的源端 FUSE 短入口遗漏、普通位置等待问题已集中处理。libfuse 三种配置在解释器 1／2／4 CPU 九组全部通过，含 63 项功能与 27 项卸载／退出检查。受影响两库按身份合并为 Apple 1629 通过／0 失败／2 忽略、Linux 1637／0／2；产品本次 565 通过／7 忽略，无失败／中止。原版 gVisor 421 个身份按 82 项重跑和 339 项明确复用合并为 411 通过／10 跳过，覆盖全部 410 个原生通过项，不增加历史全量通过数。两端严格 Clippy、格式与边界检查通过。当前候选对应产品 `d84dd19d`／Runtime `76f8602`，哈希及来源见[传输修复记录](../validation/fuse-transfer-20261125.md)。仍需 WRITEBACK_CACHE 协商、release 性能与最终平台／交付验收；02 和 03–12 保持开放，本批未提交／推送／更新 pin。

**此前真实工具检查点（历史失败）**：原版 libfuse 3.14.0 低层／请求 writeback／高层工具在 Linux 原生三种配置全部通过；同一工具与客户端在冻结解释器的三种配置 × 1／2／4 CPU 下未通过。六个低层配置在阻塞 splice 返回 EAGAIN 后退出，三个高层配置在 BusyBox 复制的中间 FUSE 元数据回复进入最终 copyout 时 panic。另行隔离后，高层工具的映射、跨页输出、AIO、8 MiB 文件读写共 15 项功能检查及 9 项卸载／退出检查通过。两个新阻塞尚未修复，已作为下一同领域集中实施批次的输入；请求 writeback 未在 guest 协商成功，吞吐仍仅为 debug 诊断。验证期间共享主线更新为产品 `d84dd19d`／Runtime `76f8602`，本轮证据仅属于固定旧 runner，不能覆盖新主线。完整命令、哈希、失败与范围见[真实工具记录](../validation/fuse-real-tools-20261125.md)；本 FUSE 批次未提交／推送，02–12 保持开放。

**最新修复结果**：第二轮检查点的 20 个失败身份均已通过，额外发现的原 OFD offset 唤醒竞态和信号旧预期一并处理。按受影响范围及明确复用身份合并，Runtime Mac 为 2060 通过／0 失败／5 忽略，Linux 为 2069／0／5；产品为 620 通过／0 失败／中止／7 忽略。两端严格 Clippy、格式与 Runtime 边界检查通过；原 offset 并发测试两端各 20 次通过。新解释器的原版 gVisor 七组 339 个身份为 331 通过／8 跳过，覆盖全部 330 个 Linux 参考通过项。详见[集中修复记录](../validation/fuse-repair-20261125.md)。这是有来源的范围合并，不是新一次全工作区或全量 gVisor；02 的真实工具／吞吐／平台交付及 03–12 仍开放，未提交／推送。

**第二轮集中验证（历史检查点）**：当前主线产品 `71cdad49`／Runtime `26384a8` 上的候选已完成 Apple／Linux Runtime 全目标全功能编译。受影响四库测试为 Mac 2040 通过／18 失败／5 忽略、Linux 2049／18／5，失败身份完全一致；产品测试在隔离补齐中止后的遗漏身份后为 618 通过／1 失败／1 中止／7 忽略。两端严格 Clippy 均在同一指针比较诊断停止，格式和 Runtime 执行边界检查通过。完整结果及 20 个失败身份见[第二轮记录](../validation/fuse-followup-checkpoint-20261125.md)，源码和退出回执在 `target/validation-fuse-followup-20261125-1500`。以下第一轮全工作区数量仅为历史范围，不能与本轮四库数量直接比较。下一批集中处理 FUSE 原名称／重查、原调用／执行资源及信号帧／事件等待，再统一测试；02 未完成，未提交／推送，gVisor 数量不变。

**第二轮后集中修复（2026-11-25 15:40，尚未运行）**：20 个失败身份均已关联到以下同批候选，待格式／编译检查后统一复测：

- FUSE ESTALE 重查回到原 live namespace 的实际挂载对象；不再复制出与原 root／cwd 及 registry 身份不同的投影，避免漏掉子挂载并提前返回 ENOENT。保留原文件名／凭据、数值 FD 的显式重查、fixed alias 和 open 预留槽位的原预算。
- 两个原生同步调用方区分未提交的 queue job 和已经取得 native admission 的 job。后者即使原布局执行资源结束也执行原 sync；仍需异步重排的 provider 结束自己的等待回执。Notification 记录终态并忽略完成后的 retirement，避免把原成功覆盖为新的错误；回调和唤醒仍在释放 admission 后执行。未增加线程。
- 完成后的 copyout 回执再次被旧别名使用时返回原有 EINTR，不再按复用后的数字 FD 重新累计 timer／syscall 完成。inotify 控制在已接受原 OFD 上完成类型错误。io_uring 的固定缓冲区 retained 路径绑定原 submitter 的 signal reader，poll 同样使用已接受的 Byte capability，外部 CQ 观察者不能提供自己的权限。
- 对照 [ARM64 rt_sigreturn](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/kernel/signal.c) 修正上一批错误判断：badframe 使用 scalar 恢复后的 `regs->sp`，SP 的 get_user 失败时为零；不是始终使用接受时的 frame 地址。恢复原实现边界，增加 SP 单字段读取失败、后续 PC／pstate 仍恢复的准备回归。原三项失败的寄存器／mask／fault 断言保留。
- masked wait 的手工 worker 夹具明确绑定原 `IoTask` 与 `TaskSignal`，与生产 snapshot 一致。FUSE 关闭夹具补齐真实 FLUSH 回复，再核对最终 OFD／名称／inode 释放；父对象 FORGET 按实际引用计数核对，不要求与原本未承诺的析构顺序一致。CREATE 的 mode／umask 原断言保留并核对最终关闭后的两份 lookup 释放。PROBE 夹具按已实现的 ABI 清零完整输入，保留原 pin 和 ring 控制断言。
- 同名 bind 的准备夹具同时保留正常成功和两次 lookup 之间被替换的拒绝分支。后者按 [d_set_mounted](https://github.com/torvalds/linux/blob/v6.12/fs/dcache.c) 不允许在已经 unhash 的原目标上挂载，保留两个独立操作数；不把第二次源 lookup 静默替换为第一次目标。共享指针比较改用 `std::ptr::eq`。

上述是集中修复候选，不能更新前一检查点的通过数量；正式工具、信号／退出／卸载及性能门槛仍开放。Apple 专用接口没有进入新增共享路径，继续同时检查两个宿主。

**本批统一验证及剩余分组（2026-11-25）**：`target/validation-fuse-repair-20261125-1540/test-source.json` 对应的候选已通过 Apple／Linux 五库全目标全功能严格 Clippy。静态诊断集中修正 default／类型别名／初始化／冗余引用，已有冷等待对象保持内联并说明避免额外分配的原因，没有改变等待算法来满足大小 lint。三个仅解释器兼容夹具构造的旧 State 变体用准确的 cfg 记录预期 dead-code，不扩大生产挂起入口。

随后统一运行三个受影响库：Mac 1809 通过／2 失败／2 忽略，Linux 1817／2／2；另外三个 Memory 目标（Mac 248／3、Linux 249／3）仅注释变化，复用上一检查点原证据。产品核心／FFI 为 564 通过／1 失败／7 忽略，原 seccomp 事件等待中止和 malformed-frame 失败均通过；conformance 全部身份一次运行完成，无中止。原 20 个失败身份中 19 个通过，仍保留原 mkdir 外层重查失败；新发现原 OFD offset 的等待竞态，以及另一信号夹具的旧 SP 故障地址预期。有效阶段各约 99／99／133 秒（含编译），没有超时或磁盘停止；原始失败完整保留。

这三项随后一起实现候选，尚未复测：NamespaceWork 的 syscall 外层重查同样回到原 live mount 身份；FileWork 在等待时同时订阅可能被后续读者占用的原 position 和实际 page／proc 资源，不根据订阅当刻的空闲状态漏掉其中一类；信号夹具区别未对齐 frame 在 scalar 恢复前失败和已对齐坏地址把 SP 清零后失败。新增无生产线程的确定性交错夹具覆盖“订阅时空闲、ready 检查前另一读者取得位置”、原 FD 关闭复用及实际唤醒；不能以延长并发测试期限代替修复。接着只统一复测受影响 VFS／产品接入及原并发失败，Kernel、Linux 和其他未变通过项按身份复用；不重跑全部 CPU／网络／图形矩阵。

**本次检查点已结束**：Mac／Linux ARM64 的 Runtime 全目标全功能编译、产品全目标编译及格式／执行边界检查通过。按身份合并后，Runtime Mac 为 2452 通过／25 失败／31 忽略，Linux 为 2459／27／15；产品接入为 603 通过／15 失败／1 中止／7 忽略。严格 Clippy 两宿主均有失败。结果仅为本地 Rust 验证，不增加 gVisor 通过数。失败身份、测试初始化修正、原始证据及下一批分组见[检查点报告](../validation/fuse-checkpoint-20261125.md)。下文保留候选形成时的“未验证”描述，现状以该报告为准；下一轮仍先集中完成领域内可实现的内容及依赖，再统一测试。

**检查点后继续实施（2026-11-25，以下均未复测）**：当前仍在整领域实施期，不因下列局部修改启动测试矩阵；上述通过数仅对应检查点源码。

- 事件等待的 seccomp 入口补接 `FileCheckpoint`，保留原调用，不再把合法的新续接类型送入 `unreachable!`。共享 copyout 凭据改为完成后消费，旧别名不能再次发布；AIO 过期 getevents 凭据按原 context 的销毁状态处理。
- 输入 Reader、RegistrationMemory、Capture 统一传递异步页能力；IO buffer 导入保留数值地址边界，把实际页面访问留到复制阶段，避免在读取原资源之前预触碰整片输出。io_uring 文件／匿名字节与普通 read／readv 的调用方一并更新。
- 管道复制复用现有 ReadLease／ReadPosition：等待页面时保留原管道前缀，按成功复制字节提交，取消释放未复制部分；补充跨页部分复制、FD 重用和旧凭据取消的回归源码。未增加生产线程或轮询机制。
- epoll 的同步嵌入接口在原对象条件变量上等实际互斥准入，内部竞争不直接输出 guest EAGAIN；source worker 仍通过原 attempt／subscribe 续接并归还执行槽。同步控制及同步等待的调用方已一并调整。
- Linux HostFS 的 ACL 刷新在有 retained parent 时从该原目录取得并校验元数据 FD，避免回退到已经失效的根相对旧路径。Apple 路径不引入 Linux API，沿用现有 target cfg。
- 原 mm 清理改为先尝试准入，仅在实际竞争时订阅并重试；成功取得 custody 后不订阅自己的 unlock，避免等待 live View 时反复自唤醒。清理测试使用另一线程持有真实竞争，旧 reader 交接测试先消费已发布工作再清通知。
- sigreturn 在部分 GPR 恢复失败后仍用接受调用时的原 frame 地址生成 fault，不使用已经被失败 get_user 清零的 SP；masked wait 的手工 worker 测试补齐生产路径已经提供的原 TaskSignal 能力。相应失败仍待统一复测。
- 按 Linux v6.12 源码区分测试预期和实现问题：[Binder](https://github.com/torvalds/linux/blob/v6.12/drivers/android/binder.c) 的入站 FD 导入失败继续等待下一事务，不向接收端返回 EMFILE；[timerfd](https://github.com/torvalds/linux/blob/v6.12/fs/timerfd.c) 设置先读输入后 fdget；[io_uring probe](https://github.com/torvalds/linux/blob/v6.12/io_uring/register.c) 先导入整块零输入，再输出。前两类修正旧预期，probe 补齐生产校验及原输入片段续接，未以先前失败计作已通过。
- 更新旧 sigsuspend／状态 ioctl 测试以进入现有原调用状态机；时间测试保留有效 timeval 写入后 timezone 失败的效果；关闭 ring 的 native sync 测试分别核对真实 host job 成功和迟到结果不再发布。检查点列出的 VFS Clippy 首批诊断已修改，尚未运行新的 Clippy。

继续实施的未收口项以紧随其后的续接记录为准。需要将所有失败身份逐一关联到已实现路径，并继续完成本页其余 FUSE 门槛后，再集中运行受影响项；目前不声明这些失败已修复。FUSE waiting 测试辅助函数补充调用点定位，后续统一失败记录能指出具体阶段，避免缺少证据时猜测修改 errno。

**同领域续接：原 actor、FUSE 重开与冷等待（2026-11-25，仍未构建／测试）**：

- FUSE 的 proc magic reopen 原先只在描述符没有缓存 inode 时走 FUSE GETATTR／OPEN；正常有缓存 inode 的情况误入普通 inode 打开流程。现统一按原 `FuseFile` 控制能力进入已有重开续接，继续保留原 FH、inode、挂载、预留 FD 和原检查；没有增加另一个打开算法。
- 兼容 `UringEpoch::capture*` 的 source 从每个快照的临时 actor 改为原 VFS 的惰性 source 根。原 VFS 的 clone／fork、空 resolver 和 control view 均取得空槽，不向 sealed resolver、Files 或 ring 复制这个根，避免 `actor → context → Files → ring` 的回引用环；生产 `capture_worker` 继续绑定已有 Linux task actor，不启用这个兼容槽。
- 已接受请求使用现有弱 `UringSource` 标识找到原 actor；更换快照不会换掉原文件／原输入 cursor。原来源已消失的请求按现有取消结果结束，PID／mm 数字复用不能接管旧操作。已绑定执行回执的 byte 请求始终通过其 actor，不会在页面变为可用后跳回无状态 fast helper 重读、重发或重写前缀。新增一项回归源码覆盖跨快照保留、原根释放、无所有权环，以及 observer 对原来源已结束请求的处理。
- 冷 event wait 的 worker 资格从“能构造 wait”改为使用该 wait 的实际 provider 准入，保留纯错误快速完成；缺少 stdin 等 provider 的 epoll 继续使用原 owner deadline。现有捕获能力只是提示，真正执行仍核对原兴趣；不执行探测性 poll、读取或用户写入。
- 原 TCP 中断续接测试没有提供 native HostEvents 能力，第一次真实等待会正确交回原 socket checkpoint。该夹具现与已有 native socket 测试一样绑定原事件源，验证真正归还执行槽、输出页等待和 FD 复用；没有创建事件线程。
- 补齐旧测试与本页既定顺序的接线：缺少能力的 uring enter 保留 typed checkpoint；原生设备 ABI 比较通过公开 start 消费 inline hint 已保留的续接；prlimit 的输出错误不回滚交换；CQ 等待错误不覆盖正提交数。layout 测试区分取消尚未重新启动的 mutation 与已经执行的 native work，后者的保留／完成仍由独立用例覆盖；没有通过丢弃全部取消场景收窄覆盖。

检查点之后新旧用例名的对应关系单独保存在 `target/validation-fuse-20261125-0952/test-identity-aliases.json`，后续合并按同一身份更新，不将改名累计为新通过项。候选指纹继续分阶段保存；上面的检查点计数不代表当前源码已通过。

**同领域续接：原 proc 唤醒与 AIO 退出（2026-11-25，仍未构建／测试当前候选）**：

- `FileWork::wait_sources` 不再仅凭命令使用文件偏移就提前订阅偏移通知；只有原偏移事务实际忙时才选择该依赖。原 proc 页面／smaps 等待现在订阅它自己的资源，避免页已激活、偏移空闲却没有后续通知的停滞。普通文件调用、AIO 及 io_uring 的调用方都有订阅后的 readiness 复查，保留订阅与依赖释放交错的处理。新增回归源码覆盖 environ／smaps、原 FD 关闭复用、实际唤醒、原偏移提交和替换 eventfd 不被消费；已有单／多执行槽 live-proc 与完整 task-proc 场景留到统一复测。
- source worker 的 fatal AIO drain 在等待原请求时通过实际 `park_io` 交还执行槽；原生工作仍保留在退出任务上。之前仅把 wait 放回 job 后返回普通 Yield，会落入测试 handoff 或未分类的 source 返回，导致单槽同行任务未运行。首次 drain 和已进入退出等待的分支一并接入，保留原请求、完成顺序和退出会计；没有创建协调线程，也没有取消已执行的原生工作。
- 补取上次检查点中被总进程中止掩盖的三项首轮断言：直接运行已保存的旧 conformance 程序，未重新构建当前候选。独立记录为 `checkpoint-diagnostic-results.json`，不能作为当前源码验收或增加通过数。其中 Binder 是回复指针解析后访问失败，完整 task-proc 是最终 marker 未写，entropy 是标志 4 的预期与返回不符。
- 按 [Linux getrandom](https://github.com/torvalds/linux/blob/v6.12/drivers/char/random.c) 修正旧测试：GRND_INSECURE（4）及其 NONBLOCK 组合合法；保留非法 RANDOM|INSECURE 和未知标志先于坏指针的覆盖。按 [Linux Binder reader](https://github.com/torvalds/linux/blob/v6.12/drivers/android/binder.c) 的最大事务记录预留，把夹具首读容量从 8 改成 80：NOOP 后允许消费 COMPLETE，剩余容量使回复留给下一次原调用。补充首读消费 8 字节和第二次实际 BR_REPLY 断言，保留 1／4／12 个执行槽及原返回值／pong／零 owner-service 覆盖。未将这些测试源码调整记录为已通过。

**同领域续接：通知名称资源及挂载接线（2026-11-25，实施中，未构建／测试）**：

- 新增 FUSE 原名称资源 `mount/entry.rs`。原 lookup／打开文件引用和目录 inode 持有实际资源；索引只存弱引用，不保留 Files、OFD 或 lookup lease，也不让后续 pathname lookup 跳过 daemon。名称键使用原 parent 的不可复用 inode identity，跨 FD 重用及同名替换不靠数字 nodeid 猜测来源。目录路径继续保存原父对象；rename／exchange 与 unlink 更新同一索引和原位置，目录别名移动检查父链，避免制造强父引用环。
- 原 `ResolvedPath` 增加名称能力，FUSE 解析、原 OFD magic link、继续打开及父路径派生接入；其他文件系统初始化为空。mount attachment／共享投影、bind／move／open_tree、递归复制和 namespace clone 传递原名称；bind 根单独保留原来源名称，避免只持有 inode 却丢掉该来源的 dentry。目录已失效时，按原目录能力构造 retained path，仍能交给相同相对路径机制；显示路径不由已经复用的名称重建。
- 准备 3 个回归函数：弱名称索引不延长 OFD／inode／lookup 寿命且替换 eventfd 不被消费；ENTRY 未命中／空名称／嵌入 NUL／slash、EXPIRE_ONLY 及重复失效；DELETE 对原名称的 child 校验、校验失败后的 unhash、原 inode 归零 links 和原 FH 保留。仅新增源码，尚未执行；通知执行体的后续接线见下，不把准备好的断言当作通过证据。
- 原生核对 [fuse_reverse_inval_entry](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dir.c)、[通知包复制](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dev.c) 与 [d_invalidate](https://github.com/torvalds/linux/blob/v6.12/fs/dcache.c)：不存在原名称返回 ENOENT；普通失效先 unhash 并分离子挂载，再校验 DELETE 的 child，不能因 child 不匹配而回滚已经完成的失效；EXPIRE_ONLY 保留 hash；名字的最后复制字节由内核置零。DELETE 的目录非空、原 link 状态和挂载交错仍需一并实现。
- `notify.rs` 已替换旧 ENTRY／DELETE 执行体：接受后保留实际选中的原名称及 inode，普通失效先 unhash／失效父属性，再通过 `mounts/detach.rs` 分离其名称子树中的挂载，之后才校验 DELETE child／非空目录并清 links。EXPIRE_ONLY 保留 hash 和子挂载；child 不匹配不回滚先前 unhash。实际 namespace 竞争使用原 Admin readiness；同步嵌入保留 Metadata 续接，生产 layout executor 使用同一 Notification，等待后不重新查名称或数字 FD。执行器失败释放原 retry completion，已经接受的 unhash 保留。这里不把手工调用 `failed` 的夹具等同于真实退出／取消验收。
- 挂载分离使用原弱 mount registry／namespace registry，按实际相关 Admin 取得短结构准入并重查原拓扑；包括 namespace aliases、覆盖层及嵌套子树。匿名树先从原 owner 的成员集中移出被分离成员，断开边界后恢复仍被原 tree FD 持有的原树成员；原打开文件及树 FD 的能力保留。新挂载／move 的最终发布与名称 unhash 共用原 Entry 索引准入；pivot 的两个 FUSE 名称按索引地址顺序准入，同一索引只锁一次，事件通知留到名称锁外。使用既有 detached retirement，没有新增协调线程、轮询任务或 Apple 专用 API。
- 调用方审计补齐 `open_tree` 的普通文件能力：非 clone 的 FUSE path FD 复用原 lookup／名称，保留 O_PATH 且没有新增 daemon OPEN／RELEASE；clone 的 mount 根保存来源 Entry，magic／空路径恢复该实际根名称。修复前只持有 inode 会丢失普通文件的名称身份。新增 6 项回归源码覆盖跨 namespace 分离与原树 FD 重接、真实 Admin 竞争及 FD 复用、执行器失败的引用释放、非空目录删除、覆盖／嵌套树分离，以及 path／clone 普通文件的 lookup 寿命和协议请求数量；全部未执行。
- 普通 worker read/write／向量、AIO 和 io_uring 的 FUSE device 先选择已有 FileTarget／FileWork，再处理直接 byte fallback；notification 在 FileWork 接受。公开 raw Connection 字节接口不能保留 namespace 等待，只是同步 transport 接口，不能把它当成完整 guest 通知入口。剩余审计仍包括 parent／child inode 的实际 namespace 事务、与 rename／create 的交错、拓扑复制／unshare 期间 unhash 的最终发布边界，以及真实执行器退出／连接卸载的资源释放。不得据当前状态机或准备断言宣布这些并发门槛通过；同领域其余映射／写回及真实工具范围不变。

本阶段保存为 `target/validation-fuse-20261125-0952/followup-source-04.json`，仍为未构建／未测试候选。实现期间主线独立前进到产品 `71cdad49`／runtime `26384a8`，其宿主文件能力和验收更新已保留，配对 pin 仍指向该已提交 runtime；本批 WIP 未纳入该 pin，也未提交或推送。格式整理不构成编译、Linux／Apple 或 gVisor 通过证据。

**同领域续接：原目录事务与 child IO（2026-11-25，仍未构建／测试）**：

- 原 inode 的独占写位扩展为同一原子状态上的独占／共享准入。LOOKUP 在发出请求前取得原父目录共享事务，最终 O_CREAT 取得独占事务；忙时 Metadata 保存原父目录、名称和操作，订阅该 inode，不发另一份 LOOKUP，不查新 FD。成功绑定后释放父事务；负 LOOKUP 进入 CREATE／MKNOD 时移交原事务，ENOSYS 降级移动同一 Create，避免中途解锁或重放。既有 setattr、xattr、普通写和 writeback 继续使用同一 inode 独占事务，readiness 区分共享读者和实际 writer。
- namespace mutation 在原已解析父目录上取得独占准入并保留到 daemon 回复。仅跨目录 rename 使用该 FUSE filesystem 自己的 rename 资源，对应 Linux 原 `s_vfs_rename_mutex`；不把其他函数或所有文件系统串行化。父目录按祖先优先、无祖先关系时源父优先；删除／硬链接取得实际原 child，rename 按源／目标子目录及非目录 inode 指针顺序取得所需资源。竞争保存具体恢复阶段和原对象，失败／取消按 child、parent、rename 顺序释放；不增加线程或轮询。
- ENTRY／DELETE 通知已接入同一父目录事务；等待子挂载分离期间保留父事务，DELETE 在校验和改动原 child 前取得 child 独占事务。完成／执行器失败将 guard 移出状态锁后释放，避免同步唤醒重入状态锁。普通 READDIR 的共享事务随原 DirectoryReply 保留到实际用户复制完成／取消，而非 daemon 回复时提前释放；原 cookie／OFD 位置事务保持已有前缀提交规则。
- 成功创建和 namespace mutation 更新原父属性版本并失效；unlink／rmdir 及 rename 覆盖目标更新原 links，目录归零、非目录递减，迟到 GETATTR 不能覆盖这一版本。rename／exchange 的原源、目标属性也按实际对象失效。这个修正不代表完整 ctime／属性掩码语义已通过。
- 准备 4 项实际入口回归：两个并发共享 LOOKUP 中取消一个后仍等待另一个；CREATE→ENOSYS→MKNOD 保留原父事务并阻止另一 LOOKUP 发包；DELETE 等待原 WRITE 后才清 links 且保留已接受的 unhash；READDIR 回复跨用户复制持有共享事务并阻止 DELETE。只新增源码，未执行。原 FileWork 对这些 FUSE／inode 等待继续保持不可被 caught handler 重放的边界；namespace topology 在产生路径续接时释放 Admin，恢复路径后再取得结构准入。
- **仍需实现／核对**：目录别名 splicing 的 filesystem rename／旧父事务、持续共享读取时独占等待者的准入与取消、拓扑复制／unshare 和 unhash 的发布交错，以及真实执行器退出／卸载。上面为新增接线候选，覆盖前段 parent／child 接线尚未实现的状态，不覆盖这些剩余边界。FUSE 映射／写回／权限／真实程序和整个 01–12 范围保持开放。

该续接源码记录为 `target/validation-fuse-20261125-0952/followup-source-05.json`。仅整理 Rust 格式并检查差异空白，未启动新构建、测试、原生探针或性能矩阵，也没有新增 gVisor 通过身份。原协议依据为 Linux v6.12 [directory-locking](https://github.com/torvalds/linux/blob/v6.12/Documentation/filesystems/directory-locking.rst)、[namei](https://github.com/torvalds/linux/blob/v6.12/fs/namei.c) 和 [FUSE dir](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dir.c)。新增生产代码不含宿主专用 API，继续复用既有 target cfg 边界；Mac／Linux 实际构建结果仍须在同领域统一验证阶段取得。

**同领域续接：独占等待、目录别名及副本发布（2026-11-25，仍未构建／测试）**：

- inode 准入增加实际 writer 等待意图。原原子状态保存 writer／waiter 位及共享读计数；只有竞争时用对象自己的短锁维护等待计数与 waiter 位的一致性。原 Lookup、namespace、setattr、xattr 和通知保存原 Attr 等待引用，未新增票据堆分配；持续的新 LOOKUP／READDIR 不再越过已登记 writer。取消最后一个等待者清除等待位并通知读者，取得 writer 时不会因释放自己的等待意图而自唤醒。此处不承诺并发 writer 的确定获胜顺序，实际争用、调度与取消验证仍未运行。
- 原目录 alias 的跨父迁移在名称索引准入内检查循环，再尝试取得原 filesystem rename 与旧父共享资源；竞争按 Linux `__d_unalias` 返回 ESTALE，不持有新父目录等待这两项资源。guard 的释放及通知在名称索引锁外。同父迁移复用已有父准入，索引相同键重发绑定时也会 unhash 被替换的原名称资源。namespace 的正 LOOKUP 回复立即绑定原 inode／名称，不推迟到准备 mutation 时才发现别名错误。依据为 v6.12 [d_splice_alias／__d_unalias](https://github.com/torvalds/linux/blob/v6.12/fs/dcache.c) 和 [FUSE lookup／create_new_entry](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dir.c)。**尚未补齐 syscall 外层的 ESTALE 重新验证／有界重试**，不能据内部尝试准入宣称整条 pathname ABI 已完成。
- 原弱 mount registry 增加短期发布归属。open_tree／fsmount、匿名 move、传播副本和 unshare 在 weak mount 注册前登记实际持有的原 Admin，真实 namespace／匿名树发布后撤销；嵌套同一 ID 的发布按原资源计数，不覆盖另一发布者。索引仅保存弱 Admin 与 mount ID，不拥有命名空间、挂载树或结构 lease。失效规划先取得这一短索引观察，再读取 mount／namespace 并升级弱引用，避免新副本已在索引中却尚无 namespace owner 时无准入地编辑它。匿名源树保留到副本发布，原 FD 关闭不会造成源索引与新副本之间的空窗。
- 私有树归属使用同一次 parent 快照，并检测观察跨越移动产生的环，避免在准入前无限爬 live parent。失效与 lazy unmount 的退休记录按实际 mount ID 去重；同一对象不会被两个退休向量互相保留。上述是原资源的发布／释放修正，没有为副本未发布新增 guest 错误、直接删除待复制子树或另开线程。
- 新准备 4 项回归：取消原 writer 后，仍有共享读者时新读者也能恢复；alias 的 rename／旧父竞争维持原名称并返回 ESTALE，后续新调用迁移同一 inode；通知 unhash 后、恢复前新建的 namespace 副本仍被重规划分离；lazy unmount 后再通知且关闭最终 FD，原挂载不因重复退休而泄漏。另扩展旧共享 LOOKUP 用例，核对 writer 已登记时新读者不发包。全部只新增源码，尚未运行。

本阶段为 `target/validation-fuse-20261125-0952/followup-source-06.json`。前段目录等待与 alias／拓扑发布的未接线状态由本段候选覆盖；完整 ESTALE 调用方、真实取消／卸载、其余权限／协议、FUSE 映射／写回／工具及 Linux／Apple 集中验证仍开放，01–12 的目标和领域顺序不变。格式和空白检查不增加任何通过身份，未提交／推送当前 WIP。

**同领域续接：有界 pathname 重新验证（2026-11-25，仍未构建／测试）**：

- 新增原 `PathRetry` 冷状态，仅在 FUSE／原生／owner 路径实际暂停时保存已导入的完整文件名、参数、凭据及显式能力。状态贯穿原 LOOKUP、权限、READLINK、剩余路径和元数据阶段；普通等待继续原请求，只有明确 ESTALE 才进入重新验证。open 的整次尝试与普通 filename lookup 的内层重查、最终回调的外层重查分别有界；descriptor-only fstat 等调用不获得 pathname 重试。此处覆盖前段“未接 syscall 外层”的部分缺口，尚不能据此宣称全部 pathname ABI 完整。
- 重新验证取得原命名空间的新观察，从当前 fs root／cwd 或重新准入的数值 dirfd 开始；固定文件及 fd_pair 的私有角色别名继续保留原 OFD 能力。原调用的凭据、链接跟随和 openat2 限制保留。正常路径不增加原文件名副本，不添加生产线程、轮询或宿主专用 API。
- stat／statfs／readlink／access／chdir／chroot／truncate 和 open 的已暂停路径接入原元数据续接；mkdir／mknod／symlink／link／unlink／rename 的 daemon 修改阶段，以及 chmod／chown／times／xattr 的最终元数据阶段，接入原 `NamespaceWork` 的一次外层重查。创建的最终 LOOKUP 失败与实际创建回调失败分开；前者依照 filename_create 的退出边界，不套用后者的重试。另一操作数仅在整次调用真正重查时重新准入，普通父目录等待仍保留已接受对象。
- 打开操作的原未公开 FD 槽位通过借用保留到真正移交／公开，失败的同步文件打开不先释放并重新挑号。FUSE 路径到普通文件 truncate／O_TRUNC 的回退保留原路径上下文，若原生步骤明确返回 ESTALE，释放失败 OFD／writer／layout 后再进入同一有限重查，open 继续使用原槽位。其 native 故障、完整取消及调度验证尚未执行。
- 新增 8 项实际入口回归源码：原请求等待／abort 与 FD 槽位、重复 ESTALE 的上限、嵌套完整路径及失败父引用、最终 GETATTR 与 descriptor stat 的区别、数值 dirfd 复用、单个固定文件／fd_pair 别名、符号链接重新验证后的 openat2 边界、mkdir 回调与 filename_create 失败的不同出口。原 alias 准入测试补上自动一次重查后才结束 ESTALE 的两次 LOOKUP／FORGET。全部仍为准备状态，没有新增通过身份。

本阶段指纹为 `target/validation-fuse-20261125-0952/followup-source-07.json`。只完成 Rust 格式与差异空白检查；未重新构建、运行当前测试、gVisor、原生探针或性能矩阵，未提交／推送。仍需核对无 FUSE 暂停的普通 native ESTALE、AT_EMPTY_PATH 各调用的重查语义及其预算边界，并继续真实 fatal／caught signal／卸载生命周期、其余 FUSE 权限／协议／映射／写回／工具和 Linux／Apple 集中验收。02 及完整 01–12 目标保持开放。

重查边界依据为 Linux v6.12 原始 [namei.c](https://github.com/torvalds/linux/blob/v6.12/fs/namei.c)、[open.c](https://github.com/torvalds/linux/blob/v6.12/fs/open.c)、[stat.c](https://github.com/torvalds/linux/blob/v6.12/fs/stat.c)、[statfs.c](https://github.com/torvalds/linux/blob/v6.12/fs/statfs.c)、[xattr.c](https://github.com/torvalds/linux/blob/v6.12/fs/xattr.c) 与 [utimes.c](https://github.com/torvalds/linux/blob/v6.12/fs/utimes.c)。

**同领域续接：执行资源结束与空路径调用（2026-11-25，仍未构建／测试）**：

- 发现原执行器真实释放时的缺口：已经从 layout 队列交给异步 provider 的 `BackgroundSync`／`SyncTask` 不再登记在原执行资源中，通知的 `state → completion → resource` 环无法仅靠回调析构打破。现于原 `FileLayouts::State` 登记正在异步等待的资源弱引用；提交原生任务前后及每次完成／重排撤销对应登记，执行资源实际析构时在无锁状态结束原资源。尚未提交执行的 layout job 也完成原错误回执。没有让登记表持有 provider、任务、OFD 或 Memory 的强引用，也未创建线程或轮询路径。
- AIO 的最后 mm 清理同样可能通过 `Context::closing → completion → FinalClose` 留下原回调。`FinalClose` 现保存自己的弱等待索引，执行资源结束只释放本清理的原回执；已经接受的 native AIO 和它的原 Context 继续由原完成路径保留。迟到完成只减少原等待计数，不重发结构操作或再次发布清理结果。实际 fatal／epoch／最后 mm 的联合调度仍须集中验收，不能用本次源码调查宣称生命周期全闭环。
- 按上述 Linux `open.c`／`utimes.c` 的区别补齐 FUSE 空路径元数据调用：`AT_EMPTY_PATH` chmod／chown，以及带空字符串的 utimensat，获得一次 syscall 外层 ESTALE 重查；普通 fchmod／fchown／NULL-path futimes 不因此重查。空路径 chmod／chown 使用 `AT_FDCWD` 时，原 `NamespaceWork` 统一按当前 cwd 操作，不把 -100 当作数值 FD。NULL pathname 的版本差异及其余调用方仍需逐项核对，未据此改写全部现有导入规则。
- 新增 5 项准备回归：通过真实 `FileLayouts`、原 FUSE 设备写和两个实际 LOOKUP 构造等待，释放原执行资源后检查通知回执与新读者的原订阅均被唤醒；queued sync 在未运行 provider 时恰好失败一次；原 AIO 已启动而清理执行器结束时，保留原 native 工作且不重复发布清理结果；空路径与纯 FD 的 chmod／times 重查上限；空路径 chmod／chown 的 live cwd。均为源码准备，未运行。

本阶段指纹为 `target/validation-fuse-20261125-0952/followup-source-08.json`；累计新增准备身份 33 项，不是新增通过数。Rust 格式与产品／Runtime 差异空白检查完成，未启动当前候选构建、测试、gVisor、原生探针或性能矩阵，未提交／推送。本段不关闭无 FUSE 暂停的 native ESTALE、其余空路径／协议／权限、真实信号／退出／卸载、FUSE 映射／写回／工具及 Linux／Apple 集中验收；02 及完整 01–12 目标保持开放。新增实现只使用已有共享资源及 Rust 通用接口，宿主专用路径继续沿用现有 target cfg。

**本批冻结并进入集中验证（2026-11-25 15:00）**：已核对当前源码，前段明确列出的目录共享／独占事务、名称失效与挂载副本发布、pathname 的有限重查、映射／写回及执行资源结束接线均已有候选。此前“尚未接入 mmap”等历史描述不能代表当前源码；其并发、实际信号／退出／卸载、工具与吞吐仍是待验收状态。现停止继续扩展本批功能，先统一编译 Apple／Linux 的工作区与测试目标，再运行受影响的 Memory／Kernel／VFS／Linux、产品解释器／session 回归及既有失败身份。编译阻塞先集中修复；运行失败按身份归组，不逐个修复后重复整个矩阵。

本次验证源码与期限、退出状态另存 `target/validation-fuse-followup-20261125-1500`。沿用各平台现有隔离 target，不重建 VM、不更改其他 Linux 宿主版本工作的环境。未受影响的既有通过证据直接复用。无 FUSE 暂停的纯 native pathname ESTALE 属于独立的共享路径层后续工作，保留在 03 的原生路径／现代挂载核对中；FUSE 已进入的路径及跨 native 回退仍属于本批验收。其余 01–12 目标、真实工具／产品交付和未验证边界均不关闭。

仍需集中收口：上面候选对全部失败身份的实际效果、FUSE 映射／写回／通知边界和真实工具门槛。上述源码修改还需在最后统一检查调用方与回退，并纳入 Mac／Linux 的同批验证；01–12 的总目标和领域顺序保持不变。

同时按所有者新要求统一 Linux／Apple 宿主边界：复用新提交的原生资源接口与 target cfg，Apple 专用类型、API 和依赖通过编译条件隔离，并验证两个宿主。此规则已补入产品与 Runtime 的 AGENTS.md。交付修复 `f5b77680` 的独立 Apple SDK CI 已通过；内存 backing／msync、文件 layout 准入及 worker 原 actor 能力仍属于本领域必要依赖。

11 月 25 日续接时仓库已前进到产品 `4f8496cb`／runtime `47f34e9`，配对 pin 相符；保留其原生资源／共享文件映射更新，本轮候选继续在当前主线实施。此前 `42e5834` 的通过结果保持原提交范围，不自动扩展为当前 FUSE 候选的通过证据。

本次继续实施期间主线已前进到产品 `68e4b7cc`／runtime `5b12cc9`，配对 pin 相符；保留其路径／DNS ABI、共享 CLI 与原生音频资源更新。本轮 FUSE 候选已进行上述检查点验证，仍有失败，未提交或推送；不将这些独立合入视为 FUSE 验收证据。

本轮先完成：懒加载共享／私有 mmap、原 OFD／FH／mount 生命周期、EOF／截断／fork／remap；原调用者的 msync／fsync／flush／unmap 写回及错误／取消／断开；映射与普通 READ／WRITE／SETATTR／fallocate 的一致性、迟到回复代次、尾页和物理计费；已可实现的 inode／entry／delete／store／retrieve 通知，以及剩余文件操作／输入输出续接。写回等待交还已有槽位，inode 等待后重新取得原 layout 准入。未实现能力不参与协商；HostFS 专用 ACL／capability 如需独立语义设计，明确登记依赖及支持范围。

上述领域内能实现的内容、调用方及回退完成后，统一构建、测试、原生对照、真实程序及吞吐验收。实施阶段不运行本领域测试，不新增协调／服务线程、轮询消息总线或普通访存屏障。

实施核对：当前 worktree 已有 FUSE 映射／写回／通知、常规读写与截断一致性、`sync/syncfs/sync_file_range`、FIFO／设备打开的候选；必要依赖还包括保留原输入片段的导入、已消费 byte／signal／socket 数据与 SCM_RIGHTS 的输出续接、accept／pipe／socketpair 的未发布 FD 预留，以及 AIO／io_uring 的原资源等待。上述候选均未进行本轮构建或测试，不计作通过或已交付。

本次续接新增候选：

- `munmap`、`MAP_FIXED` 和 `mremap` 的已移除原 backing 由 Memory 捕获并按 inode 合并；结构变更只执行一次，写回完成后返回其原结果，错误记回原文件。最后 mm 退出在释放 FD 后、发布退出结果前等待；成功 exec 的旧 mm 写回接入新任务的已有 Completion 等待。没有增加线程。
- 后台关闭写回按 inode 合并；执行器结束时完成原等待并记录写回错误，失败不自旋重提。映射原 backing 保留到关闭资源完成。
- 套接字地址／选项的控制输入保留已读片段，输出保留原查询结果；输出缺页不会重复清除 `SO_ERROR` 等状态。字节 IO 的原等待期限跨缺页续接保留，原生 TCP 冷缺页改走已有原套接字续接。
- io_uring 非固定缓冲区的字节读写、Unix/IP send/recv 已有原输入／已消费输出续接候选，包括原提交者的 signal reader 和必要的原 VFS 回退。用户复制等待独立于 SQE 的 NOWAIT 标志；已选择的原缓冲区跨缺页保留，多次接收在本次完成后再选择下一缓冲区，busy 状态由原 lease 释放，避免旧请求清掉新请求的准入。固定缓冲区仍沿用现有匿名物理 pin 能力，不把 FUSE backing 声称为可注册。
- `/dev/fuse` 纳入 FileWork 原设备通知／读取等待，避免直接字节写绕过通知准入；通知失效与 DELETE 保留收到通知时的原 inode。准备了原映射写回／迟到代次／截断及 VMA 关闭错误／替换／取消的回归源码，未运行。
- io_uring 初始 SQE 的向量／路径／open_how 导入保留已读片段及已接受原文件，预算先预留未完成导入的上限；注册数组逐次保留原 FD，结构 pin 回退保留原注册对象。setup 的参数输出保留未发布 FD，完整复制成功后才发布；enter 的扩展参数／timeout 缺页保留原 ring，期间尚不接受 SQE。上述源码候选包括原 actor／joined 接口及结构 checkpoint 调用方。
- AIO 提交输入保留原 IOCB、原 OFD／eventfd 和 aio_key 输出进度；aio_key 写入在 payload 导入之前，已接受前缀完成导入后才分发。原 inode layout 等待复用已有准入和文件执行器，销毁时尚未执行的原 ticket 可释放。
- AIO getevents 复制保留原批次及 cursor；按照 Linux v6.12 的批次边界，复制失败不推进该批 CQ head，前面已完整成功的批次计数保留。低于 min_nr 的已完成批次可先消费再等待；输出缺页期间不使用 timeout 反复唤醒。io_cancel 先读原 aio_key；context 查找自身缺页也保留已读片段。timeout 输入在 context 查找前复制，相对期限在取得 context 后开始。
- AIO context 的使用者跨输入／layout／输出等待保留；io_destroy 撤销原映射后等待原请求及使用者退出。一个调用的快照副本共用一个使用者，完成／中断只释放一次，旧快照不拖住销毁。getevents 的输出复制等待不被普通信号丢弃；已取出的完整批次计数跨后续信号返回保留。准备了复制中途失败、销毁交错、取消输入顺序及原 ring／FD 复用回归源码，未运行。
- AIO setup 的 ctxp 读取及原 mm checkpoint 已有续接候选；已接受的零值不在结构回退后重新读取。结构分派优先恢复原 setup／destroy／registration 对象。11 月 25 日续接已替换“输出缺页后撤销重分配”的初始方案：原 context、映射、句柄和已复制前缀跨缺页保留；分配错误先于句柄输出错误，复制失败在原 mm 清理原 backing 后返回。
- setup 取消使用原 mm 的被动 mapping lease 记录；Drop 不编辑运行中的 mm，不创建线程。原执行 View 返回后，由现有结构准入调用该资源的原生清理规则。源任务退出先释放原待处理调用，再等待必要的原 mm 收尾；旧共享 mm 的清理通过已有 RetiredMemory 驱动和 affected-mm 暂停推进，只保留弱 root，不增加原 mm owner。准备了实际 store 才缺页、前缀被并发改写、失败／取消、移动／替换、原 View／custody 等待和弱 root 释放回归源码，未运行。
- AIO VMA 源码核对修正了初始清理规则：lookup 使用用户 slot 与 context 当前记录的地址，支持同地址替换后的有效用户 header；fork 继承共享环字节，子 mm 无 context，移动该文件映射时拒绝。环采用原 resident backing，禁止增长／零长度别名扩展及执行；移动通过冷 VMA hook 更新原 context 地址，失败恢复源页与策略，并保留已发生的固定目的地址清除／尾部截断。普通访存不查询该 hook。
- AIO 销毁与 setup 取消按 context 记录的当前 mmap_base 和原 mmap_size 解除范围，移除其中的替换页；部分 VMA 移动同样更新 base，原范围之外的旧别名可保留。已销毁 context 的 setup 用户退出不重复解除该范围。原 AIO destroy 在等待用户退出时，也保留已解除 VMA 的关闭写回；源任务退出的关闭资源收集覆盖非最终 mm。已准备移动／fork／替换 header、失败回滚、部分映射移动、范围替换、避免二次解除及关闭等待回归源码，未运行。
- IP 部分接收的 msg_len 与剩余 timeout 输出保留原完成计数和复制 cursor；恢复不再次接收，后续长度复制失败保留更早的完整消息计数。普通信号引发的收尾输出新增原 Delivery 续接，持有原调用及完整信号回执，输出完成后才决定重启并进入处理器；只有 EINTR 才参与 SA_RESTART。准备了跨页复制／永久失败、FD 复用、后续数据不被误消费及信号队列顺序的回归源码，未运行。
- 旧共享 mm 的取消记录在处理期间继续保留队列成员身份；并发发布合并为后续一轮，不替换已接受的原清理和关闭 cursor。原 mm custody 内收集解除映射的关闭资源，使用已有 FileLayouts 等待；多批清理可在前一批写回期间保留资源，驱动切换／弱 root 消失不丢已接受的关闭。最后旧 View 返回后再次发布的数据在最终 mm 释放前补一次原 inode 关闭。准备了实际清理／关闭提交交错、两批写回、缺少执行能力后补绑定、驱动替换及最后脏写回归源码，未运行。
- proc／sysctl／mount 文本、cgroup 文件及消息队列状态的顺序读取统一使用原文件位置事务；实际复制前缀决定提交偏移，复制等待期间的 dup／seek／第二次读取复用原位置通知。cgroup／mqueue 的直接字节和 owned-byte 回退也遵守同一 pending gate，避免越过原输出事务。准备了成功／取消、跨页输出、原 FD 替换和非阻塞第二次读取的回归源码，未运行。
- 原 RetainedWait 的 poll／select／epoll 输出保存已接受的 readiness／事件记录与复制 cursor；epoll 输出缺页不重新取事件、不重查被复用的 epoll FD，输出期间不按原已到期期限反复唤醒。剩余 timeout 在原完成／中断结果确定后取一次快照，跨页等待保留已复制前缀；永久 timeout 输出错误维持原 best-effort 规则。原 worker、joined 及中断收尾接入 FilePending 的页等待。准备了 one-shot 输出续接、epoll FD 复用、timeout 前缀改写及原 ready 结果保留的回归源码，未运行。后续补入的原对象准入与逐事件提交见下；初始参数导入及完整 owner 能力边界仍待完成。
- epoll_ctl 的事件参数缺页保留原输入片段；完整参数导入后才解析 FD，遵守其原读取／fget 顺序。缺少直接 descriptor 能力时，结构回退携带已接受的标量参数，worker／旧入口不重新读用户内存；普通无异步页的调用保留原内联路径。准备了事件 mask 前缀改写、输入缺页先于 EBADF，以及复制结束前 epoll FD 替换的回归源码，未运行。
- epoll 事件复制新增原对象的跨等待准入，使用现有资源通知释放执行槽；完整事件才提交 oneshot／edge／proc 观察位置，永久失败保留此前完整记录数，取消不消费未复制记录。原 ctl 捕获两端 OFD 和完整标量后等待同一对象，worker、结构回退及旧入口复用该请求；poll／select 在观察 epoll 前按稳定对象顺序准入嵌套对象，旧拓扑变化交还执行槽后重新准入。未新增线程、轮询总线或普通访存屏障。
- epoll 输出按 AArch64 的 events／data 字段分别复制，保留 padding；入口改为地址范围检查，页权限／缺页在实际复制时处理，maxevents 按低 32 位的 int 解释。原范围内未映射输出不抢先替代 EBADF；高位扩展的 DELETE op 也按低 32 位解释。已新增永久复制失败、已完成前缀／取消、ctl 两端 FD 复用、零 timeout 的 poll／select 等待嵌套对象、padding 与错误顺序回归源码，未运行。
- poll／select／epoll wait 初始输入新增 WaitInput：pselect 参数包先于 timeout，实际信号掩码后于 timeout；成功片段、期限起点、nfds 准入及最初 fdtable 范围跨缺页保留。先复制完整 poll 数组／三个 select 集合，再取得 FD；原读取结果不因输出只读而提前失败。能力不足时保留已接受输入或原 OFD，进入原 caller 回退，不把缺少能力当作 EBADF。直接、结构和旧入口已有接入候选，完整 owner-provider 仍需继续核对。
- 临时信号掩码通过原任务资源在输入页等待前生效，普通结果在剩余 timeout 输出前恢复，零 timeout 不回写；信号中断保留给信号帧或实际用户返回边界处理。原 Context 内的代次保护后续调用与 ptrace 更新，旧回执／快照不能覆盖新掩码；该等待使用现有槽位与通知。准备了跨页输入／FD 替换、导入期间降低 RLIMIT／扩张 fdtable、参数包顺序、timeout 起点／输出故障、旧 mask 回执及实际信号帧恢复字的回归源码，未运行。旧入口的完整交接与信号帧自身缺页仍属于未完成边界。
- 原 epoll delivery 缺少 caller provider 时显式保留原对象及准入并交接，不作为空事件或 guest 错误完成；joined／旧入口查询原文件，并将 caller 的 stdin 查询传入原嵌套 epoll。poll／select 的原 OFD、原宿主等待资源和通知不因 FD 槽关闭重用而改查新文件。旧文件输入续接补入原任务 mask 能力，前后同步其实际 Context。补充了 stdin FD 重用、嵌套 provider、已接受事件跨输出页等待且不重复查询／消费 child oneshot 的回归源码，未运行；裸能力边界仍需继续核对。
- 信号入口的帧内容／SME 记录、原恢复 mask 和寄存器保存一次，逐页复制成功后才提交处理函数状态；rseq 的注册、事件、已读片段及已写前缀跨页等待保留。sigreturn 的原帧输入也保存成功片段，完整校验后才恢复架构／信号状态。原 worker 与旧执行入口接入既有 Page 等待及原任务续接；等待期间不再选取第二个 caught handler，退出／exec 释放未完成状态。正常无缺页入口不预分配续接对象，不增加线程、普通访存屏障或轮询总线。补充原 mask／寄存器、已复制前缀、rseq 描述符跨页改写、sigreturn 原输入／永久尾部失败及取消的回归源码，未运行；调度、生命周期及全部复制入口仍需整批核对与验收。
- sigwait／suspend 新增原 WaitCall：成功输入片段和已取出的信号／siginfo 输出跨页保留；suspend 的原任务掩码在完整导入后生效，baseline 交给原信号帧恢复。复制等待暂停原 timeout 并订阅实际页资源，排队、worker／旧入口及结构返回继续保留同一调用，等待期间不消费第二个信号，也不提前处理另一个 caught handler。补充输入 mask／timeout 前缀改写、已消费记录与另一读者交错、剩余队列／输出前缀及 suspend baseline 的回归源码，未运行。
- 子进程 wait 新增原 WaitCall：在原 family 选取／回收一次后，保存原 status、usage、siginfo 和结果，再逐页复制；原 family／namespace／进程组绑定与输出前缀跨 worker、结构返回和旧入口保留，WNOWAIT 允许另一读者回收而不改写首个调用的已接受结果。waitid 按原生顺序先 rusage，再写六个 siginfo 字段，保留 padding／union；无事件及入口错误的零字段输出也可等待原页。准备了 wait4 已回收对象／rusage 缺页、WNOWAIT 并发回收、waitid 错误与 padding 的回归源码，未运行。中断输出及原信号／重启交接已补入下述 Delivery 候选，完整调度、生命周期和调用方仍需核对与验收。
- 等待中断使用原 Delivery，直接持有已选中的 RetainedInfo、action、原调用及输出进度，不把该信号放回可被其他读者消费的队列；原 worker／旧入口／结构返回保留同一对象，等待原页时交还执行槽。waitid 的中断零字段与 EFAULT 优先级、IO 部分结果和 SA_RESTART 在复制结束后决定；睡眠剩余时间只取一次快照。退出／exec 资源清理及冷退休释放原 continuation，旧快照不能再次进入处理器。准备了信号竞争、原参数／保存 PC、永久复制失败、睡眠时间／已复制前缀及取消旧快照的回归源码，未运行。
- 一次性 handler 与原信号在原 sighand 对象锁内领取，领取时仅重置 handler 字段，保留 flags／mask／restorer；原收尾和信号帧等待不再在最后覆盖期间新安装的 handler。补充相同 action 重新安装的回归源码，未运行。普通 mask／altstack／架构状态仍在信号帧复制完成后提交；未新增同步层、协调／服务线程或轮询总线。

上述输入顺序、输出权限与 timeout／mask 阶段对照 [Linux v6.12 `fs/select.c`](https://github.com/torvalds/linux/blob/v6.12/fs/select.c) 的 do_sys_poll、core_sys_select、do_pselect、ppoll 与 poll_select_finish；这些仍是候选实现及待运行回归，不是本轮原生对照通过证据。

上述 child wait 的输出及回收顺序对照 [Linux v6.12 `kernel/exit.c`](https://github.com/torvalds/linux/blob/v6.12/kernel/exit.c) 的 waitid／kernel_wait4／wait4；这是源码依据，尚无本轮原生运行通过证据。

一次性 handler 的领取与成功建帧后 mask／altstack 更新对照 [Linux v6.12 `kernel/signal.c`](https://github.com/torvalds/linux/blob/v6.12/kernel/signal.c) 的 get_signal／signal_delivered，以及 [ARM64 signal.c](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/kernel/signal.c) 的 handle_signal；仍为实现依据和待运行回归。

上述 epoll 规则依据 [Linux v6.12 `fs/eventpoll.c`](https://github.com/torvalds/linux/blob/v6.12/fs/eventpoll.c) 的 do_epoll_wait、ep_send_events、do_epoll_ctl，以及 [`include/linux/eventpoll.h`](https://github.com/torvalds/linux/blob/v6.12/include/linux/eventpoll.h) 的 epoll_put_uevent；仍是实现依据，尚无本轮运行通过证据。

上述 AIO 边界对照 [Linux v6.12 `fs/aio.c`](https://github.com/torvalds/linux/blob/v6.12/fs/aio.c) 的 lookup、io_submit_one、aio_read_events_ring、io_cancel、kill_ioctx 与 aio_ring_mremap，以及 [`mm/mremap.c`](https://github.com/torvalds/linux/blob/v6.12/mm/mremap.c) 的文件 hook 失败回滚；这是候选实现依据，不是原生对照通过证据。

本次续接补入 Binder 用户复制依赖候选（仍未验证）：

- 原 ioctl 持有完整 header、当前命令、已导入数据／offset／SG 片段及逐对象 FD／节点引用；后续缓冲区缺页不重新解析已接受命令或查找已接受 FD。命令完成后才推进 write_consumed，最终回写 header；去掉写入前输出预检。
- 读阶段先复制 NOOP，出队后持有原消息与输出 cursor；FD 安装一次，完整事务记录复制成功后才允许 FREE_BUFFER、发布接收调用栈。SPAWN 在最终 header 回写之前输出。读取过程不持有 Connection 锁访问 guest 页。
- 控制查询保留已取得输出。GET_EXTENDED_ERROR 改为原线程错误、读取时清零一次，跨页续接不再次清除之后产生的错误。原事务返回错误在实际输出成功之前继续阻止后续写命令。
- 原 Binder 请求通过共享 RetainedBinder 跨 worker／joined／旧测试入口交接，保留原 OFD 与参数。输入／输出 PageWait 交还既有执行槽，并纳入普通信号、ptrace、group stop 的复制等待边界；真正的 Binder 读等待才可中断。中断先完成原 header 复制再返回 EINTR，不按 SA_RESTART 重启。退出／Delivery 取消使旧交接副本失效；没有新增线程。
- 快速完成的调用仍在原调用栈执行；仅真正挂起才分配共享续接对象。小型 header／NOOP／控制输出使用内联复制状态，避免每次正常 ioctl 增加一组输出分配。
- 新增 8 个未运行的回归源码，覆盖已完成命令、SG 后续页缺失与 FD 复用、接收记录／FD／调用栈发布顺序、逐线程错误查询、声明长度之外的命令参数读取、实际 backing 页通知、执行器交接、中断输出及旧副本取消。原 Binder profile 的 FD 类型、4 MiB 上限等支持范围没有扩大。依据 [Linux v6.12 Binder 实现](https://github.com/torvalds/linux/blob/v6.12/drivers/android/binder.c) 核对操作顺序，尚未进行原生执行对照。

本次续接补入信号控制依赖候选（仍未验证）：

- sigaction／sigprocmask／sigaltstack 共同采用导入、一次变更、原结果输出的顺序；输出失败保留已经发生的状态变更。sigaction 的 sighand 锁及备用栈的 IoContext 锁不跨 guest 复制；被忽略信号的队列清理仍和原 sighand 变更一起发生。
- 新 ControlCall 保存输入片段、原 Actions／目录／IoTask、sigprocmask 在入口取得的旧 mask 和输出 cursor；输入等待完成后才变更状态，输出等待恢复不重新查询旧状态、不重复清队列，也不覆盖后续的新处理器／mask／备用栈。小输出内联保存，普通无缺页输入不建立复制片段缓存。
- 控制复制复用 SignalWait 的已有等待与 worker／joined／旧入口交接；缺页交还执行槽，普通处理器、ptrace、group stop 采用原复制等待边界。ptrace 请求在原复制／文件结果等待期间不反复唤醒任务，实际页就绪后先完成复制再进入跟踪停止；已经停止的任务也允许其已接受的信号／child 复制收尾。完成后释放通知订阅，退出／exec／冷 mm 等待在替换任务状态之前使旧交接副本失效。没有新增线程、轮询或普通访存屏障。
- 对照 [Linux v6.12 signal.c](https://github.com/torvalds/linux/blob/v6.12/kernel/signal.c) 调整输入／验证／输出错误顺序，rt_sigpending 接受 0～8 字节输出；按 [ARM64 UAPI](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/include/uapi/asm/signal.h) 将 sigaltstack 最小尺寸修正为 5120。sigreturn 的备用栈复制失败仍是错误，复制完成后的栈验证失败按原生规则忽略；尚未执行原生对照。
- 新增 12 个未运行的回归函数，覆盖原输入和前驱、状态变更后输出失败、队列清理仅一次、原任务绑定、短 pending 输出、错误优先级、备用栈／sigreturn、实际 source 分派／缺页通知／CPU 返回交接及取消旧副本；其中 source 场景包含四种控制接口。

本次续接补入信号帧永久失败候选（仍未验证）：

- 建帧／rseq 准备的错误携带原信号编号，跨 PageWait 后发生永久错误也不丢失。普通信号建帧失败通过原线程队列强制 SIGSEGV；如果失败的原信号就是 SIGSEGV，则将其当前处理器改为默认。忽略／阻塞的 SIGSEGV 同样恢复默认并解除该线程的阻塞位，其余 disposition 字段保留。
- 强制投递保留原标准信号合并及原进程队列；原 sighand／IoContext／inbox 的变更完成后再通知，避免和现有 run queue 的观察形成锁等待。继续走原信号／ptrace 选择；不在建帧失败处无条件结束进程，仍可转到使用普通有效栈的 SIGSEGV 处理器。
- 初始坏 sigreturn 先检查帧地址对齐／范围，不因无效地址触发无关 FUSE 页读取；故障信息按 ARM64 find_vma 规则分类，包括低于后续 VMA 的空洞。Memory／View 和全部现有复制／测试包装器共用必需的纯 VMA 元数据接口，含 PROT_NONE 和已发布的匿名增量映射；不加载页、不分配物理页，也不添加普通访问检查。
- source／旧入口的坏 sigreturn 在强制故障后使用原生零返回值，原故障信息和阻塞状态保留到后续信号选择；不将 EFAULT 写成该系统调用的 guest 返回值。去掉“必须有内部 frame 计数”的附加条件，有效用户构造帧也可恢复，内部计数饱和递减。
- 新增 10 个未运行的回归函数，覆盖普通／递归失败、阻塞与忽略、合并、锁外通知、真实 VMA／匿名增量、跨页后失败、用户构造帧、备用栈失败后的 SIGSEGV 处理器，以及坏 sigreturn 的原始地址／代码／零返回。顺序依据 [Linux signal.c](https://github.com/torvalds/linux/blob/v6.12/kernel/signal.c)、[ARM64 signal.c](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/kernel/signal.c) 与 [traps.c](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/kernel/traps.c)，没有运行本轮测试或原生对照。

本次续接补入部分 sigreturn 与 rseq 原返回边界候选（仍未验证）：

- sigreturn 按 ARM64 原生顺序先恢复 mask，再逐项读取通用寄存器；永久读取错误将对应标量置零并继续访问后续标量。缺页保存已完成位置，后续失败保留已恢复的寄存器／mask，并用恢复后的 SP 分类强制故障。原入口及兼容入口共同处理部分状态，不将它们撤回为整帧事务。
- 扩展记录改为逐个读取描述头，允许原生支持的 FPSIMD 位置、重复 ESR、EXTRA 扩展描述及 256 KiB 帧范围；不读取已经终止的未使用保留区或 ESR 载荷。依次恢复 FPSIMD／SVE、TPIDR2、ZA 和 ZT，支持原有 SME 配置及原任务寄存器；后续等待／失败不重读前面已接受的字段，也不清除帧中省略的原状态。未声明的 CPU 能力仍不接受。
- 浮点载荷永久失败后仍按原生顺序访问 FPSR／FPCR，其失败随随后实际页等待保留；SME 载荷的已复制前缀保留在原状态内。去掉额外的 8 层内部建帧上限，实际可用 guest 栈决定嵌套范围。
- 正常 rseq notify-resume 和信号前 fixup 使用同一已接受请求；fixup 完成后才推进 CPU／node／mm_cid 输出，其 PC 修正在后续输出缺页或建帧失败时仍生效。直接复用原任务已有续接槽及 PageWait，未新增线程、轮询或普通访存屏障。退出／exec 复用原取消入口释放请求。
- rseq 注销通过原任务控制复制续接，保留原标量参数及已完成的 TLS 写入；全部写入完成后才解除注册。source 分派、seccomp 准入与旧入口已接入；永久输出错误／取消保留此前写入和尚未解除的注册，旧完成回执不会重复注销后来注册。
- 新增 13 个未运行的回归函数，并更新原帧／SME／任务信号夹具的事务假设，覆盖标量部分错误、PSTATE、浮点后续错误／缺页、记录位置与 EXTRA、原 SME 状态、嵌套帧、rseq 原输入／输出前缀／注销与取消，以及实际 source 返回交接和建帧失败 PC。依据 [ARM64 signal.c](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/kernel/signal.c)、[FPSIMD 状态](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/kernel/fpsimd.c) 和 [rseq.c](https://github.com/torvalds/linux/blob/v6.12/kernel/rseq.c) 核对源码，未运行本轮构建、测试或原生对照。

本次续接补入 AIO 信号参数候选（仍未验证）：

- io_pgetevents 按 timeout → 完整 sigset 描述 → 掩码字节的顺序导入，非空掩码使用原任务的代次租约，在 context 查找之前生效；空掩码指针忽略长度，非空指针要求 8 字节。描述和掩码片段跨页保留，不重复读取或重新安装。
- 缺少原任务能力时，Lookup 在接受 context 之前携带原参数交接；source／joined／旧入口的 FileBoundary 接通 AioLookup。数据／事件输出期间维持原 mask；正常结果立即恢复，待处理信号保留给原信号帧／实际用户边界。零事件并有待处理信号时使用中断结果，正事件数保留。
- AIO 逻辑完成显式释放 mask 与 context user，不等待旧快照的最后一个 Arc 析构；旧 user 回执不能重新消费 CQ 或影响后来的 mask。中断保留已取得事件计数；任务退出仍走原资源释放。
- timeout 采用原生 timespec64_to_ktime／ktime_set 的有符号字段换算及上限，负时间按已到期处理，不在掩码导入之前额外拒绝这些字段。新增 7 个未运行的回归函数，覆盖输入片段、错误优先级、context 页等待、能力交接、正常／中断／正事件 mask、输出等待、旧快照及原始 timespec 字段。
- 顺序依据 [fs/aio.c](https://github.com/torvalds/linux/blob/v6.12/fs/aio.c)、[set_user_sigmask](https://github.com/torvalds/linux/blob/v6.12/kernel/signal.c) 和 [ktime_set](https://github.com/torvalds/linux/blob/v6.12/include/linux/ktime.h) 核对。io_uring 的实际提交与参数顺序见下面的续接候选；本轮未运行构建或测试。

本次续接补入 io_uring enter 全调用候选（仍未验证）：

- 原调用先接受一次 SQE，再导入 GETEVENTS 描述；部分提交直接返回原数量，后续参数永久失败保留已提交数量。没有 GETEVENTS 时忽略参数包／实际掩码／timeout，并保持普通提交的内联路径；fd、数量及 flags 按 ARM64 u32 解释，min_complete 按原 CQ 容量限制。
- 完整描述导入后先检查 CQ；确实需要等待时才读取 timeout 和实际 mask。已接受片段、等待起点、参数、原 ring 和提交数量跨页保存，fd 关闭／复用及新 SQE 入队不改变本次调用。原任务掩码使用已有代次租约；实际 EINTR 的等待交给信号帧／用户返回边界，正常完成和取消各释放一次。
- 等待记录保存最低等待、一般期限和原 SQ timeout 计数；最低计时结束后按原规则完成或改等一般期限，避免重新使用已到期的期限。SQ timeout 完成也能唤醒更大的 min_complete。候选支持 EXT_ARG 的 min_wait_usec、ABS_TIMER 及非 SQPOLL ring 的 wake／wait 提示，不新增 SQPOLL、计时或协调线程；MIN_TIMEOUT 的 feature 位与候选一并准备验收。
- UringEnter 共享原完成回执；逻辑完成立即释放原 ring／任务引用及 mask，旧快照保存结果而不延长原所有权，不能结束后来同任务的新等待。取消清掉未完成输入并在锁外通知原等待；旧 FilePending／FileCheckpoint 副本失效后不重新导入。源码核对同时补入 AIO／uring／原文件输入的退出和 Delivery 取消交接。
- source、joined、旧 Session 及裸 Linux 入口共用同一提交／等待实现，删除了裸入口的重复 SQ 提交体。裸入口按原任务保存实际 PendingIo／FileCheckpoint，公开原等待交接；dispatch 的 Deferred 保留原寄存器，由宿主继续等待。Session 传入原 TID；Mac HVF 的固定指令示例补上该结果分支，仍待配对 Runtime pin 后的集中验收。已有 FileActorScope 继续使用原身份；裸调用在缺少该 scope 时绑定自身凭据和同步执行能力。
- 新增 13 个未运行回归函数，覆盖无 GETEVENTS／部分提交、正提交数与参数错误、CQ 快捷路径及 min_complete、真实 timeout 页等待、原 mask／期限、fd 复用、旧完成回执／取消、最低计时、SQ timeout、裸 dispatch／能力交接及原参数。更新原先假设“参数复制先于 SQE”与旧 tuple 交接的夹具。
- 原生顺序依据 [io_uring.c](https://github.com/torvalds/linux/blob/v6.12/io_uring/io_uring.c)、[io_uring.h](https://github.com/torvalds/linux/blob/v6.12/io_uring/io_uring.h)、[timeout.c](https://github.com/torvalds/linux/blob/v6.12/io_uring/timeout.c) 和 [UAPI](https://github.com/torvalds/linux/blob/v6.12/include/uapi/linux/io_uring.h)。当前仅核对源码、调用方与差异空白；未运行 rustfmt、构建、测试、原生对照或性能测量，未提交／推送本轮候选。

本次续接补入结果取消及系统元数据候选（仍未验证）：

- FilePending 的取消同时失效原输入、等待工作和已完成输出；旧快照保留终态，不继续占用原读取位置、OFD 或目录位置。取消在复制锁之外释放原资源，只提交已经实际写入的读取前缀。FileResult 的原目标改为同一共享完成记录持有，取得结果或取消时释放；迟到的宿主回复直接丢弃，避免旧记录继续持有读取位置。io_uring 原 FileIo 退役也使用同一完成记录清理。
- AIO 取消显式清空共享导入／事件／批次状态，旧快照不继续占用 user、reaping 或本地等待资源。尚在宿主执行器中的请求将原活动计数交给同一 FileResult，原执行器返回并释放回复之后才移除计数并唤醒 destroy；未开始的本地工作直接释放。排队的 layout 工作订阅原完成记录的取消通知，并在取得 mm／布局准入前结束，不新增执行线程。这里不将“宿主 FileWork 本次返回”笼统等同于所有 daemon 请求已经结束，后者仍在下述生命周期核对中。
- uname／sysinfo／getrandom 的已取得结果接入原 FileCopyout，缺页后保留快照、原字节及输出前缀；hostname／domainname 输入使用原 UTS 能力和输入片段续接。source 执行、原任务交接及旧执行入口已接入。uname 的兼容 release 改写在完整结构输出后执行；UTS setter 按权限 → 32 位有符号长度 → 实际复制的顺序处理。getrandom 接受 GRND_INSECURE，拒绝与 GRND_RANDOM 的无效组合，并保留永久复制失败前的正字节数。同步裸 helper 的其余能力边界仍未全部关闭。
- 新增 8 个未运行的回归函数：3 个针对取消后原位置、迟到回复及宿主请求／destroy 交错；5 个针对 UTS 原输入与快照、兼容输出顺序、错误优先级、一次统计采样和随机输出／取消。仅进行源码及差异空白核对，尚未运行 rustfmt、构建、测试或原生对照。
- 核对依据包括 [Linux fs/aio.c](https://github.com/torvalds/linux/blob/v6.12/fs/aio.c)、[UTS 系统调用](https://github.com/torvalds/linux/blob/v6.12/kernel/sys.c) 和 [getrandom](https://github.com/torvalds/linux/blob/v6.12/drivers/char/random.c)。原生 exit_aio 会等待整个原 mm 的 context 请求退出，且完成记录／eventfd 有自己的发布顺序；当前仍需把最后 mm 退出和 exec 旧 mm 的 AIO drain、取消后的活 context 完成通知及在途 FUSE 请求一起接入，不能依据活动计数的局部候选宣布生命周期对齐。

同轮继续补入最后 mm 与 daemon 请求收尾候选（仍未验证）：

- Memory 单独保存最终 mm 关闭资源，普通 unmap 的关闭队列不执行它，fork 产生的新 mm 不继承，共享 mm 使用原 root。AIO setup 为原 registry 绑定该资源；现有最后 mm 退出及 exec 旧 mm 的 RetiredMemory 关闭链因此保留并驱动原 context，不增加 mm owner 或执行线程。
- AIO 最终关闭撤销注册 context 的新调用准入，唤醒原任务，并通过原 Context 的 active／user／reaping 状态等待。io_destroy／setup 回滚已摘除但尚在使用的原 context 以弱引用登记；最终关闭和重复关闭仍覆盖这些请求，弱记录不延长已经完成的 context 生命周期。原完成通知在 Context 锁之外推进关闭回执。
- FUSE Completion 在原结果锁下保留取消收尾凭据。未投递请求撤回时释放；已投递请求等原 daemon 回复或断开；已消费的回复不会重新接受后来的凭据。原 page receiver 完成之后才释放凭据，所有释放在传输及完成锁之外。AIO 的本地 FUSE 等待及迟到 FileWork::Waiting 回复将活动计数转交给该原请求，避免把宿主工作返回 Waiting 误认为 daemon 操作已结束。
- 另新增 7 个未运行回归函数：3 个针对普通 unmap／fork、已摘除 context 的实际宿主请求、重复最后 mm 关闭及原 pipe 用户唤醒；4 个针对未读撤回、原回复／断开、已消费回复及 page receiver 先后顺序。加上上一段共新增 15 个未运行函数。仍只有源码和差异空白检查，没有本轮格式化、构建、测试或原生对照。
- 仍需补齐取消后活 context 的 CQ／eventfd 发布、原 mapping_close 写回完成与 context drain 的组合，以及 source／joined／裸同步入口的其余元数据与生命周期核对；上述原生发布顺序尚不能依据候选源码判定已对齐。

同轮继续补入 AIO 原任务退出完成候选（仍未验证）：

- 修正把 context 摘除等同于取消全部已接受请求的假设。原调用已经取得 context user 时，后续导入／提交不因另一个 io_destroy 标记 dead 而重新查找或拒绝；原管道、文件与 daemon 操作继续按实际结果完成。CQ 写到原 ring 页，随后通知原 eventfd、释放原完整 OFD，最后归还活动计数；context 已被摘除也保留该顺序。后台异常放弃仍使用原资源清理，不把它作为正常任务退出的完成路径。
- Source 的致命退出先在原 task／mm 上完成已接受的 AIO，再交接退出状态和 ptrace exit stop。管道无进展返回 EINTR，未交给 daemon 的原请求可撤回；已投递请求发送原中断并等待实际回复。已完成读取的用户复制遇到致命退出缺页时保留实际前缀或 EFAULT，不重放先前的写入。只在原资源通知或期限到达后重进现有槽位；退出等待标记放在原 user 的原子字段内，run queue 不在持锁时获取 batch 锁，也不因退出标志重复入队。
- Cold task／共享 mm 暂停和旧串行的单线程、整组退出均保留原 AIO，完成之后才退休 IoTask 和发布家族退出状态。旧出口的 AIO 等待使用已有完成通知，拥有 mm 不再使未完成 I/O 自动就绪；没有新增线程或轮询。
- io_destroy 的调用保留独立 destroy user，既不计入它自己的 users 等待条件，又防止最终 mm 关闭提前越过其映射处理。原 mapping_close 在 FileResult 实际完成时通知 Context；回调在原结果锁之外执行，并在完成前保留原 Context。最终 drain 同时等待 active、普通 user、destroy user、reaping 和原映射写回。
- 新增 6 个未运行回归函数：实际宿主写／原 eventfd 复用／destroy、读取输出的实际前缀或故障、最后 mm 与原 mapping_close、FUSE 致命中断、单槽 source 退出等待及旧单线程／整组退出；调整了原 pipe 销毁和 mm 关闭夹具中“dead 立即取消”的假设。本段仍只有源码及差异空白核对，没有运行格式化、构建、测试或原生探针。
- 顺序依据 [fs/aio.c](https://github.com/torvalds/linux/blob/v6.12/fs/aio.c)、[FUSE request_wait_answer](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dev.c) 及 [ARM64 fault.c](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/mm/fault.c)。核对同时发现下一项同轮工作：io_submit 当前把整批输入导完才启动前面的请求；原生逐 IOCB 接受并执行。必须补齐前驱实际启动／完成与后续输入缺页的顺序，避免后续 FUSE 输入依赖前驱 I/O 时停滞，并覆盖错误前缀、退出及回退。

同轮继续补入逐 IOCB 与异步同步操作候选（仍未验证）：

- 删除“全部输入完成后再派发”和非缺页入口的另一套提交体。普通缓冲读写保留一个当前请求；它的实际复制、CQ／eventfd 和资源释放完成后才导入下一个 IOCB 指针及描述。前驱可以改写后项指针、IOCB 或 payload，后项 FD 也在其原生阶段查找。读完描述并检查保留字段／长度后才预留活动计数，继而取得原 FD／eventfd、写 aio_key、导入数据。已完成前缀在后续导入等待／失败／致命退出时保留，不重读或重做前驱。
- 一个请求释放 staging 后可以继续后项，不再用旧的整批 MAX_WRITE 预算或 64 个描述上限提前截短。每个数据请求仍使用既有的有界转移；调用的描述上限使用原 context 的 ring 范围。活动表按真实请求数计数，不把复用同一 IOCB 地址当作 EINVAL。
- FSYNC／FDSYNC 按 [aio_fsync／aio_fsync_work](https://github.com/torvalds/linux/blob/v6.12/fs/aio.c) 接受后交给已有文件执行器，io_submit 继续导入后项，不等待同步操作完成，也不保留 guest buffer。原 context 持有实际请求；后续 inode／daemon 等待由原通知再次排入已有执行器，mapped layout 使用已有 FileLayouts。完成继续走原 CQ、eventfd、OFD 和活动计数顺序；io_destroy、最终 mm 和 exec 的旧 context 收尾覆盖这些异步请求。
- 异步同步操作冻结接受时的原凭据、TID 及写回能力，在现有执行线程作用域内恢复；只对该异步接受阶段取快照。对执行器和 FileLayouts 保留弱引用，不反向延长服务寿命；关闭或丢弃任务完成原错误回执。缺少异步执行能力时，原已导入操作经 FileBoundary::Aio 交接，期间不重新查找 FD、读取 IOCB 或重复写 aio_key。没有新增协调／服务线程或轮询总线。
- 修正 AArch64 的 O_DIRECTORY／O_DIRECT 位混用，目录可执行异步同步操作；既有 O_DIRECT 数据传输能力门槛仅作用于相应读写。FSYNC／FDSYNC 拒绝非零数据操作数；负 nr 在 context 查找前返回；O_PATH 在原 FD 获取阶段拒绝，不先改写 key。
- 新增 9 个未运行回归函数：前驱提供后项 header／指针、已完成前缀和后项缺页／致命失败、后项 FD 复用、重复地址的独立 FSYNC、异步同步在后项输入缺页期间完成及 destroy、缺少执行器的原能力交接、实际 FUSE FSYNCDIR 回复与 FD 复用／原 Source 释放、错误优先级及操作数、原凭据快照。改写旧“大批转移按 staging 截短”的夹具为每次仅一个数据请求在途、依次接受全部原请求。上述均为新增／调整源码，尚未运行格式化、构建、测试或原生探针。

同轮继续补入 timerfd 与时钟查询候选（仍未验证）：

- 按 [Linux v6.12 timerfd_settime／gettime](https://github.com/torvalds/linux/blob/v6.12/fs/timerfd.c) 的顺序，settime 先完整复制新 itimerspec，再验证低 32 位 flags／timespec、取得实际 FD 及检查 alarm 权限；不能因输出不可写而抢先拒绝更新。跨页导入保留已读片段，FD 在完整导入后取得。新旧值交换只发生一次，fdput 后才复制旧状态；原输出跨页等待时，另一任务的重新设定和 FD 关闭／复用不会触发原更新重放。大正数 timespec 按 ktime_set 转换饱和，保留原 update_mode 的取消错误行为。
- 缺少 alarm 权限能力时，通过原 TimerFd checkpoint 交接已导入值和原 OFD，普通信号／取消及旧回执释放不重新查找 FD 或重新读取输入。Source、joined 冷入口和旧串行入口接入原保留调用；joined 从当前原任务补齐能力。更新或查询完成后，输出只保留原快照，不额外延长 timer OFD。没有新增协调／服务线程、轮询或普通访存屏障。
- clock_gettime／getres 保留一次原时钟或 CPU group／task 计时结果；输出页等待不重新采样。gettimeofday 按 [原时间 ABI](https://github.com/torvalds/linux/blob/v6.12/kernel/time/time.c) 依次复制 seconds、microseconds 和 timezone，后面的失败保留前面的写入。getcpu 按 [原 getcpu](https://github.com/torvalds/linux/blob/v6.12/kernel/sys.c) 的两个 put_user 处理，第一个失败后仍尝试第二个，跨页等待保留先前错误和原处理器号。原 scalar store 与普通块复制分开处理，不把标量拆成多个页片段。
- 原 FileCopyout 补入实际标量复制和显式继续错误模式，仅 getcpu 启用后者。时钟／getcpu 的普通非异步内存路径保持栈上结果和当前执行线程，不建立等待 cursor；timerfd 的输入 journal、保留 Box 和输出 cursor 仅在需要时创建。固定 timer helper 的普通块复制也保留实际页前缀，不再预检查后面的目标页。
- 新增 14 个未运行回归函数：8 个 timerfd 覆盖输入／校验／fget 顺序、已复制片段和 FD 复用、更新后的输出等待／永久失败、旧快照、源对象释放、取消、原权限能力交接、alarm 权限及 flags／ktime 饱和；6 个时间／getcpu 覆盖原 CPU 时钟快照、错误优先级、timeval 顺序与前缀、getcpu 的双目标继续错误和原标量／处理器号。调整旧夹具中“所有输出先检查”的假设。以上仍未运行格式化、构建、测试或原生探针。

同轮继续补入 POSIX／interval timer 与 RLIMIT 候选（仍未验证）：

- 按 [Linux v6.12 prlimit64／do_prlimit](https://github.com/torvalds/linux/blob/v6.12/kernel/sys.c)，新 rlimit 完整导入后才解析原目标 PID、检查目标权限及 resource／pair；resource 按 unsigned int 解释。原 Limits 对象在自己的锁内一次取得旧值、更新新值及实时 enforcement；释放锁后才复制旧值。输出失败保留已经生效的新限制，new／old 别名的输入先完整读取，getrlimit／setrlimit 的空指针保留实际复制错误。目录与 Limits 的锁均不跨实际用户复制。
- RLIMIT 的 Call 保留原调用者权限／namespace、已读输入片段、原目标与旧值输出。输入缺页完成后才查目标；输出缺页期间 PID 被回收／复用不重查，保留目标的 limit 对象也不使旧 PID 可见。后续并发修改不被恢复过程覆盖，取消不回滚已发生的交换，也不重新执行它。Source、joined 共用现有任务等待，旧串行入口使用同一保留调用；普通非异步内存继续执行现有直接调用。
- 按 [原 setitimer](https://github.com/torvalds/linux/blob/v6.12/kernel/time/itimer.c) 与 [timer_create／timer_settime／timer_gettime](https://github.com/torvalds/linux/blob/v6.12/kernel/time/posix-timers.c)，先导入完整 timeval／timespec／sigevent，再进行相关参数检查与目标查找。real／virtual／prof interval timer 和 POSIX CPU／wall timer 的原状态变更先完成，旧值在解锁后通过原 cursor 复制；查询输出保留一次快照。后续页等待不再次查 timer ID、重设相对期限或覆盖并发操作。
- timer_create 为原 registry 预留独立 ID，未完整复制 ID 时不发布可查询的 timer；其他创建不能取得该预留。复制完成后执行原 clock-specific 初始化并发布。复制永久失败、取消或初始化失败释放原预留；已复制的前缀按实际结果保留。预留使用原 registry 的对象锁，不新增执行线程、协调器或定时服务。现有时钟及容量支持范围仍需按完整工具验收单独核对，本节不是 POSIX timer 全部行为的通过结论。
- 共用的 user_copy::Output 改用现有 CopiedOutput，小元数据结果内联保存；timer 状态锁内只生成原快照，实际用户写入及页等待在锁外。现有 SignalWait 容器承载这两类保留控制，取消和旧快照共享同一回执；等待绑定的同步通知移到回执锁之外，调度回调不会在该锁下重入。
- 新增 17 个未运行回归函数：7 个 RLIMIT 覆盖生效／前驱值／实时限制、逐页输入与目标复用、目标释放与 PID 可见性、永久输出失败与空指针、错误优先级、取消及别名；8 个 timer 覆盖一次更新、完整输入后查找、未发布创建和并发 ID 预留、失败／取消、原 sigevent、三类 interval timer、查询后删除及 ID 复制后的 clock hook 失败；另有 Source 换执行器后的实际保留调用与通知锁外回归。修改了旧夹具中“输出失败保留旧限制／旧定时器”的假设。仍未运行 rustfmt、构建、测试或原生探针；共用 Output 的变动也纳入后续 child wait 相关检查。

同轮继续补入 usage／times、robust list 与调度复制候选（仍未验证）：

- getrusage 与 times 的结构数据保留原 task／group／children 一次快照，输出页等待不重新查询计时和内存统计。按 [原 times](https://github.com/torvalds/linux/blob/v6.12/kernel/sys.c)，times 的返回 tick 在结构复制成功之后取值；已经复制的 tms 字段及前缀不重写，返回值完成后也不再次采样。普通同步 helper 的块复制按实际页前缀处理，Source 的普通内存路径仍直接执行。
- 按 [原 get_robust_list](https://github.com/torvalds/linux/blob/v6.12/kernel/futex/syscalls.c)，查询原 head 后释放目标查询作用域，先 put_user 长度，再 put_user head；长度失败不写 head，别名按该顺序覆盖。跨页等待保留原 head 和已完成的长度写入，后续 set_robust_list／PID 变化不改查。CopiedOutput 补入原 scalar store 模式，put_user 不被改为两个独立的页片段 store；块复制仍按页保留进度。
- 按 [原调度 ABI](https://github.com/torvalds/linux/blob/v6.12/kernel/sched/syscalls.c)，sched_param／affinity mask／sched_attr 先完整导入，再查目标及实施一次策略更新；PID、flags 和 mask 长度按对应 32 位 ABI 解释。sched_attr 先复制已支持 body，再检查未知 tail；tail 缺页不重新读取已接受 body。大小协商的 size 写入失败仍返回原 E2BIG，写入本身遇到暂时缺页则保留原 cursor。getter 复制原 policy／nice／mask 快照，不保留或重新查已释放的目标；既有版本 0 属性和策略支持范围仍需在完整工具验收中核对。
- affinity 更新保留原 running lease 的实际离场 Completion。输入页完成后切换到该 Completion 的订阅，等待释放原执行槽；恢复不重设 mask、重读输入或等待新请求。现有 SignalWait 容器承载 Query／Scheduling，页资源和 Completion 在通知后重绑；同步通知仍在回执锁外。Source、joined 与旧串行入口均接入保留调用，没有新增协调／服务线程或轮询。
- 新增 17 个未运行回归函数：7 个 query 覆盖 rusage 快照／永久故障、times 两个取样时点、robust 两次 put_user 的顺序／别名／标量跨页及取消；8 个 scheduling 覆盖 body／tail 输入、原 affinity 离场、目标释放／PID 复用、原输出 mask、错误优先级、E2BIG、取消及复制前缀；2 个 Source 回归覆盖原查询跨 CPU 返回和页等待转原 Completion。上述尚未格式化、构建、运行测试或原生探针，不计入通过数。

### 排队信号／pidfd 与 perf 属性输入续接（未验证）

- `rt_sigqueueinfo`／`rt_tgsigqueueinfo` 按原 48 字节 siginfo 与必要的未知扩展区导入；已复制片段跨页保留，完整输入后才检查目标／权限并入队一次。系统调用的 signo 覆盖用户头，pid_t 参数按低 32 位解释；取消不入队。
- `pidfd_send_signal` 在复制前保留原 OFD／pid 对象，复制完整后再检查 supplied signo、权限及原目标存活／身份；输入错误不会被提前的目标查找掩盖，数字 FD 关闭复用不能更换原目标。排队发送和 pidfd 的 Source、joined 与旧入口共用保留调用，仍使用已有 PageWait／SignalWait，没有增加线程。
- perf 属性先读取原 size 标量，再导入支持的 body／未来零尾区；E2BIG 的 size 提示单独续接，永久提示写入错误维持原 E2BIG。事件在完整输入后创建一次，FD 槽预留在 group／target 查找前；失败释放未发布槽。Source 与旧入口使用原任务、文件表及 factory，完成回执不会重复创建事件。CPU／cgroup 作用域及其生产者仍属于 08 perf 的剩余范围。
- 新增 16 个未运行回归函数：7 个发送用例覆盖前缀改写／未知扩展／取消／错误顺序／FD 和 PID 复用，7 个 perf 用例覆盖属性片段／未来尾区／size 提示／取消／目标与 group 查找顺序／槽释放，2 个实际 Source 用例覆盖 CPU 返回、原资源续接、通知重绑与一次效果。仅进行源码与差异空白核对，尚未格式化、构建或运行，不计入通过数。
- 输入和 fget 顺序对照 Linux v6.12 [signal.c](https://github.com/torvalds/linux/blob/v6.12/kernel/signal.c) 与 [events/core.c](https://github.com/torvalds/linux/blob/v6.12/kernel/events/core.c)；未据此扩张现有未实现作用域的支持声明。

### 异步 FD owner／状态 ioctl 续接（未验证）

- F_SETOWN_EX、F_GETOWN_EX 及 FIONBIO／FIOASYNC／socket owner 控制在触碰用户参数前保留原 OFD／slot bit；输入片段跨页保留，setter 完整导入后执行一次，getter 在锁内取快照、锁外复制。F_GETOWN_EX 保留实际字节前缀，socket owner 的 put_user 保留单个 32 位标量写；FD 关闭复用不会更换原文件。flag-only embedding 保留原 status 能力。
- owner 状态保留清零 PID 时选定的类型，新 OFD 默认 F_OWNER_TID；F_SETSIG 按原生 int 参数取低 32 位。原 public helper 同样在实际 copyout 前释放 owner 设置锁。
- Source 的控制分类、seccomp 准入、原 SignalWait 页等待及旧串行入口接入相同保留调用；取消释放整个原 OFD，完成回执复用原结果。普通无异步 backing 的调用继续原内联路径，没有添加服务线程或轮询。
- 新增 10 个未运行回归函数：9 个 kernel 用例覆盖输入／输出前缀、FD 复用、原 pipe endpoint／SIGIO、标量输出、错误优先级、取消／永久输入失败、完整 OFD 的持有释放、owner 类型与低位参数；1 个 Source 用例覆盖 CPU 返回、原文件、通知重绑和一次效果。尚未格式化、构建或运行，不计入通过数。
- 顺序对照 Linux v6.12 [fcntl.c](https://github.com/torvalds/linux/blob/v6.12/fs/fcntl.c) 与 [ioctl.c](https://github.com/torvalds/linux/blob/v6.12/fs/ioctl.c)。这里只覆盖已实现的 owner／状态控制；key／task-name 的后续候选见下。其他设备 ioctl、裸同步 helper、退出／exec／daemon 生命周期及全部原位置调用方仍需核对，本批仍不是全域完成。

### key／任务名称与任务标量输出续接（未验证）

- PR_SET_NAME 的原输入字节跨 backing 等待保留，完整导入后只修改一次；PR_GET_NAME 在输出前取得原 comm 快照，按实际复制保留前缀，永久后页错误保留已写字节。PR_GET_PDEATHSIG／GET_CHILD_SUBREAPER 的原值使用单个 32 位 put_user 输出，跨页等待不重新查询当前值。
- 现有 keyctl 0／1／5／6 支持范围接入同一保留调用：join 的字符串与调用凭据固定到原调用，完整导入后才改变原 selection；describe 的原数据／权限快照、长度及原 key 引用持续到实际输出完成／失败／取消。selection 后续切换不能提前释放原 key 的 quota 引用。名称上限失败按 strndup_user 返回 EINVAL，未实现操作维持原 unsupported 能力。
- Source 分类／seccomp、旧入口及裸 Linux dispatch 均接入；裸入口 Pending 时保留原 CPU 参数，并可导出原页等待／调用。任务退出释放对应原调用，exit_group 释放该裸上下文全部调用；生产 worker 仍复用现有 SignalWait，不新增线程。普通 helper 的输出同样删除全范围预检，实际 copyout 前缀可见。
- 新增 12 个未运行回归函数：10 个 Linux 用例覆盖名称／key 字节续接、一次更新、原 key 生命周期／权限快照、错误与短 buffer、标量复制、裸入口 CPU／交接及退出；2 个实际 Source 用例覆盖名称／父死亡信号／subreaper 原查询和 key 原 registry／对象／输出快照。原 comm ABI 回归的 GET_NAME 永久故障期望同步改为已复制前缀。尚未格式化、构建或运行，不计入通过数。
- 顺序对照 Linux v6.12 [sys.c](https://github.com/torvalds/linux/blob/v6.12/kernel/sys.c)、[keyctl.c](https://github.com/torvalds/linux/blob/v6.12/security/keys/keyctl.c) 与 [strndup_user](https://github.com/torvalds/linux/blob/v6.12/mm/util.c)。keyctl 的其余操作不在本次现有接口的复制依赖范围内，不扩张支持声明。

### 其余设备 ioctl 的保留调用、sync fence 发布与回归源码（未验证）

- 新的 ioctl 调用在用户复制前保留原 OFD，并把入口权限、输入片段、一次 provider 操作和实际输出 cursor 分期。现有 perf 控制／ID、FUSE_DEV_IOC_CLONE、namespace、TUN、render、socket 查询及 PTY 已接入候选；32 位 request 统一解释，未实现请求维持原 unsupported／ENOTTY 范围。FUSE clone 在完整源 FD 输入后查找源文件，非法源 FD／O_PATH 按原生返回 EINVAL。
- 终端属性／winsize 输入在取得终端状态锁前导入，查询在锁内取快照、锁外复制。job-control 检查跨输入缺页只完成一次；实际作业停止使用原 TerminalTask 的 readiness 等待，仍可被信号中断；用户复制等待不按作业停止处理。旧 public helper 同样释放终端锁后才写用户内存。
- sync CREATE／MERGE 在输入前预留原未发布 FD 槽，跨输入与输出等待保持原 slot／fence；输出完全成功后发布，失败或取消释放原预留。MERGE 的第二个文件在完整输入后接受，并保持到原输出结束；无效第二个文件返回原生 ENOENT。INFO 的 fence records／状态／时间戳取一次快照后按原次序复制。
- TUNSETIFF 的 link attach 先于实际 copyout；输出故障不重新 attach，永久 copyout 错误保留原操作效果。ifconf 先导入完整 16 字节 header，复制原 IPv4 records 后以单个 32 位 put_user 写 len；负容量按零条处理，保留 padding／pointer。ETHTOOL 的嵌套 command 等待保留已完成的原设备查找结果；render、TUN 与网络 bulk 输出按实际页边界推进。
- Binder 在原 OFD 接受后交给既有 typed ioctl，保留整个原文件与原 FileActor，避免分类后 FD 变为另一种文件时误走通用复制或重新查找数字 FD；完成／取消在回执锁外释放这些资源。原 Source、joined、旧 byte-I/O 和裸 Linux dispatch 接入候选；裸入口保留原 CPU 参数并导出原 PendingIo／checkpoint，退出取消共享回执。缺少必要 owner 能力时保留原调用再走现有回退。
- 新增 23 个未运行回归函数：22 个 Linux 用例覆盖可分别阻塞的读／写输入输出、原 FD 复用、CREATE 预留槽释放、MERGE 第二文件的查找时点／原对象、INFO 原记录／时间戳、PTY job-stop 转输入复制、终端锁外实际写入、一次窗口信号、perf／namespace 标量输出、TUN attach 后成功／失败／取消、ifconf 次序／负容量／header、嵌套网络设备查找、render 多输出、Binder 分类后 FD 复用、裸入口 CPU／交接／退出及最小能力回退；1 个 Source 用例覆盖真实 syscall dispatcher、CPU 返回与新入口能力下的原 OFD／输出／释放。这里只准备了源码；独立读写 Gate 是缺页注入，未据此声称真实 FUSE/COW 或两执行器通过。
- 核对回退时补上无 factory 的 slot 控制在效果前返回给 owner，避免没有原 slot token 的无效 checkpoint 循环。状态 ioctl 的公共入口检查与已接受操作分开，Source／kernel 的复制恢复复用首次入口检查；完成回执释放原 landlock domain。尚未格式化、构建、运行测试或探针，不计入通过数，也未宣称 ioctl／整个 FUSE 领域完成。没有新增协调／服务线程、轮询总线或访存屏障。
- 实现顺序对照 Linux v6.12 [FUSE device ioctl](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dev.c)、[sw_sync](https://github.com/torvalds/linux/blob/v6.12/drivers/dma-buf/sw_sync.c)、[sync_file](https://github.com/torvalds/linux/blob/v6.12/drivers/dma-buf/sync_file.c)、[tty job control](https://github.com/torvalds/linux/blob/v6.12/drivers/tty/tty_jobctrl.c)、[tty ioctl](https://github.com/torvalds/linux/blob/v6.12/drivers/tty/tty_ioctl.c)、[ifconf](https://github.com/torvalds/linux/blob/v6.12/net/core/dev_ioctl.c) 与 [IPv4 records](https://github.com/torvalds/linux/blob/v6.12/net/ipv4/devinet.c)。

### HostFS 原 inode 元数据与 Linux POSIX ACL（未验证）

- Linux 的 O_PATH symlink xattr 不再统一返回 EOPNOTSUPP：内部 proc FD magic link 是到原 inode 的纯跳转，不继续解释原符号链接的文本。Native 对象借用原 File，FD 生命周期覆盖整个操作；rename／unlink／数字 FD 复用不改变接受对象。不依赖 Linux 6.13 的 xattrat；其空路径入口使用普通 fdget，不能据 AT_EMPTY_PATH 推断 O_PATH 可用。
- chmod／chown 统一使用原元数据描述符。Linux 的 O_PATH chmod 通过同一 proc capability，chown 使用原 FD 的 AT_EMPTY_PATH／AT_SYMLINK_NOFOLLOW；Darwin 使用原 O_SYMLINK vnode 的 fchown。删除 symlink chown 重新查询旧 pathname 的路径；FD 与 pathname 属性变更前刷新原权限，guest 的 SGID 准入先于 native chmod，返回原宿主实际 mode。exec by FD 同样在执行准入前刷新原权限／ACL；Linux 内部数据访问可通过保留的 O_PATH inode 重新取得 native 读／写访问，仍由宿主检查所需访问，pathname 新 open 继续独立进行原生准入，不复用该路径。
- Linux 原生 POSIX ACL 接入 GET／SET／LIST／REMOVE、原 inode 的 DAC、chmod 和创建继承。命名 UID/GID 在原 HostFS grant 的 owner 替换、guest 全局身份及当前 user namespace 之间转换；写入及 chown 拒绝不可表示的 owner 别名，宿主 ACL 出现会与映射 owner 合并的另一原生身份时返回 EOVERFLOW，不合并权限。当前 user namespace 未映射的命名 ID 输出 UINT_MAX。保留原生 ACL setter 忽略 CREATE／REPLACE 的规则、容量查询、错误与只读准入；不把 guest ID 原样传给宿主。单个 owner 替换仍有上述可表示范围，不能作为通用 ID-mapped mount 宣称完成。
- 创建目录和普通文件时，在实际用于 mkdirat／openat 的原父目录上判断 native default ACL，存在时由宿主继承，不提前套用 guest umask，也不修改进程全局 umask。读取原 native ACL 后同步 guest 的权限视图；不用 ctime 相等推断 ACL 没变，避免较粗时间戳下漏掉宿主独立修改。新增的 Linux 元数据访问成本留待整批测量，尚无性能结论。
- 新增 13 个未运行回归函数：2 个 Linux symlink xattr、3 个原 inode 元数据／执行生命周期，以及 8 个 Linux ACL 用例。覆盖目标不被跟随、原链接／文件在关闭复用／rename／unlink 后保留、重挂载后原 ACL 权限、default ACL 与 umask、chmod／独立宿主修改、原 xattr 对象、user namespace／owner ID 转换、别名拒绝、容量／flags／只读／权限次序，以及 exec 的原 ACL 和已 unlink 程序字节。旧普通 xattr 回归按 Linux 上缺失 ACL 为 ENODATA、Darwin 保持格式门槛更新源码。尚未格式化、构建、运行或采样，不计入通过数。
- 明确保留的宿主范围：当前 HostFS grant 只有目录能力、单个 owner 替换与只读属性，挂载固定 MS_NODEV／MS_NOSUID；没有受保护的 guest 权限元数据存储。Darwin 的 ACE ACL 无法直接作为 POSIX ACL 导入；security.capability 也不能把 guest SETFCAP 转化为宿主 capability 写入。两者继续 EOPNOTSUPP，完整支持依赖独立的 guest 元数据权限／持久化设计，并须一并接入写入、truncate、可写映射、chown 撤权和 exec。不能只换 xattr 名称后声明完成，后续挂载领域保留此依赖；本轮不扩大宿主权限或移除 NOSUID。
- 顺序与原对象语义核对 [Linux POSIX ACL](https://github.com/torvalds/linux/blob/v6.12/fs/posix_acl.c)、[proc magic link](https://github.com/torvalds/linux/blob/v6.13/fs/proc/base.c)、[namei pure jump](https://github.com/torvalds/linux/blob/v6.13/fs/namei.c)、[xattrat](https://github.com/torvalds/linux/blob/v6.13/fs/xattr.c) 与 [XNU vnode 属性入口](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/vfs/vfs_syscalls.c)。仅为源码依据，不是本轮运行证据；没有新增协调／服务线程或访存屏障。

### FUSE 通知的部分页 RETRIEVE 与回复范围（未验证）

- RETRIEVE 按原 page-cache 中页是否存在复制，不再要求普通 READ 所需的整页有效状态；部分 STORE 的原字节可返回，未命中的第一个页结束回复，不跳到后面的缓存页，不向 daemon 发送 READ，也不把该页标成整页有效。未写部分继续使用现有零初始化物理页，不暴露宿主未初始化内存。
- 回复同时受原 max_write、EOF 及默认 32 页的限制；非对齐起点扣除页内偏移。EOF 及更远偏移仍发送带原 offset／cookie 的空回复。通知名称上限修正为 FUSE_NAME_MAX 1024，超限先于整包长度检查返回 ENAMETOOLONG。
- 新增 5 个未运行回归函数，覆盖实际通知接收／回复中的部分 STORE、首个缺页停止、普通读取有效状态保持、32 页边界、EOF／极大偏移及名称错误次序。依据 [Linux dev.c](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dev.c) 与 [FUSE_NAME_MAX](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/fuse_i.h)；没有运行、构建或采样，不计入通过数。
- ENTRY／DELETE 的 held dentry 存在性与返回错误、其余通知／协商、取消和原 worker 入口仍需继续核对；这些补充不代表通知全语义已完成，也不提前进入测试。

实施入口核对仍未完成，进入统一验证前继续处理：

| 必要依赖／边界 | 当前剩余工作 |
| --- | --- |
| io_uring / AIO 结构与回退 | setup、VMA 移动／查找／继承、AIO 信号参数、io_uring enter 分期／mask／期限／CQ 快捷路径及裸入口统一已有候选；原宿主请求、共享复制取消、最后 mm／exec 原 context、daemon 取消凭据、原任务退出的实际 CQ／eventfd、mapping_close 组合、逐 IOCB 数据执行和异步 FSYNC／FDSYNC 已有候选；继续完整生命周期调用方及其他裸 Linux／无 actor 回退核对。已有候选仍需共同验收，不能将候选写成通过 |
| 输出与等待入口 | IP 收尾、位置事务、RetainedWait、epoll 原 provider／逐事件提交、wait 输入／mask、信号帧／sigreturn、sigwait／suspend、child wait 和中断 Delivery 已有候选；Binder、信号控制、坏帧强制策略、部分恢复／rseq 衔接及注销、UTS／sysinfo／entropy、timerfd／clock／timeval／getcpu、POSIX／interval timer／RLIMIT、usage／times／robust list／调度／affinity、排队信号／pidfd／perf 属性、异步 FD owner／状态 ioctl、key／task-name／父死亡信号／subreaper 查询已有候选；其余设备 ioctl 的原 OFD／分期复制、sync fence 预留发布、PTY job stop 和原 Binder 文件／actor 以及裸入口已有实施候选，23 个 ioctl 回归函数已备源码，继续剩余控制入口／裸 helper 能力、取消生命周期与全部原位置调用方，整批仍待验收 |
| 生命周期／回退 | 旧共享 mm 清理并发发布／多批关闭、最终旧 View 发布已有候选；继续退出／exec／daemon 同组退出和断开、取消及全部原任务调用方；joined／无 actor 的能力边界，不允许持有执行槽等待 daemon |
| FUSE 文件及宿主能力 | 其余通知与协商错误、非普通文件语义继续审计；Linux 原生 POSIX ACL、原元数据 FD／exec 和 O_PATH symlink xattr 已有候选及 13 个未运行回归。Darwin POSIX ACL／两端 guest capability 元数据保留上文独立权限存储依赖，未虚报支持 |
| 验证准备 | 扩展原协议／真实 FUSE 程序场景、Mac 两执行器／Linux 原生对照、吞吐与正常负载采样 |

上述新增候选仅做源码与差异空白核对，尚未运行 rustfmt、构建或测试，不计入通过数或已交付范围。当前仍是同一轮 FUSE 实现，不切换到 03 现代挂载，也不提前运行局部矩阵。

对应[完整实施清单](implementation-followups-20261123.md)，保留其中全部 12 项范围。**现有候选已完成已覆盖范围的集中验证、修复及 main 合入；当前按完整清单梳理下一轮剩余实现与验收，整批目标仍未完成。** 本页保留首轮实施和验证过程；当前状态以完整清单开头的摘要为准。

Runtime `42e5834` 的两端 CI 和 Mac 产品 `2a99558` 构建通过。Apple SDK 宿主及 FFI iOS API 阻塞已修复并完成本地验证，远端专项检查随发布确认，见[交付修复](../validation/ios-embedding-host-20261124.md)。下一领域按顺序进入 FUSE 实现；尚未开始该领域新一轮测试。

动态任务上限的共享实现和 guest 行为已验证，futex2／rseq、perf 基础及映射 cache／fork COW 等也已有已合入实现，不能继续依据下表的初始状态称为“尚未实现”。FUSE mmap／写回、ID-mapped mount、回收／IO controller、Android 接口、perf 剩余生产者／作用域及真实工具／测量／平台门槛仍开放。

## 最新领域迭代安排

先修复 Apple SDK 构建宿主的交付阻塞；随后从 02 FUSE 开始，按完整清单顺序一次推进一个领域，必要依赖除外。当前领域内所有能实现的内容、调用方与回退完成后，再统一构建、测试与测量。验证失败汇总后回到该领域下一轮实施，不逐 bug 展开测试。下述跨领域实施表为历史快照，不继续作为单轮边界。

## 历史实施快照（集中验证之前）

| 范围 | 当前问题／缺口 | 实现阶段工作 | 状态 |
| --- | --- | --- | --- |
| 02 FUSE 元数据和协议 | namespace 父目录无条件 GETATTR；迟到／取消回复的共享副作用；回复长度与授予资源的边界 | 合并缓存、失效、回退和取消路径；明确有效回复的接收时点 | 父目录缓存／权限回退、迟到回复更新已有 inode、传输回复长度检查已实现候选；协议／缓存的其余依赖仍在实现 |
| 02 FUSE 数据操作 | size／truncate、打开标志、非普通文件的剩余语义 | 完成共享 inode／OFD／writer 生命周期及调用方 | O_TRUNC／FD／pathname truncate、原 inode 写／SETATTR／xattr 事务与 readiness 等待、追加偏移／实时 F_SETFL、stream／nonseekable／NOFLUSH、同步写续接及 FSYNC ENOSYS 缓存、零长度 IO、WRITE／fallocate 前的 capability／SUID／SGID 清理，以及 read 的实际 copyout 前缀／原 OFD 位置、同步写在 FSYNC 成功后提交 f_pos 已有候选；完整缓存／通知／mmap、非普通文件等依赖仍待补齐 |
| 02 FUSE 映射与通知 | 异步文件页准备、共享写回、通知及退出／卸载 | 接入现有 FileWork、内存 backing 和 readiness；等待交还执行槽 | 原页 PageWait／CPU 等待、锁外回复接收器、文件／AIO／io_uring 原结果与原资源 copyout 续接，以及 inode 清页资源／同页请求合并／失败独立重试、当前 fault actor／连接 PID namespace、NOTIFY_INVAL_INODE 和普通修改清缓存已有候选；syscall 输入导入及其余输出、写回／其余通知／映射接入仍待实现，尚未开放 FUSE mmap |
| 02 关联宿主能力 | hostfs ACL／capability 映射、Linux O_PATH symlink xattr | 原描述符能力、命名空间映射与权限路径统一 | 待实现 |
| 03 现代挂载 | ID-mapped mount、nosymfollow、传播属性、fspick／重配置 | 补齐 mount 对象、权限／所有权和别名更新路径 | nosymfollow、unbindable／传播属性、tmpfs fspick／重配置及共享 writer 准入已实现候选；ID-mapped 与其余文件系统仍待实现 |
| 04 cgroup | 共享 page cache、私有 COW 计费与回收、IO controller | 在资源归属处计费／释放，接入已有等待机制 | 映射 cache 归属／别名去重、文件私有映射首次写入、RSS 重绑、匿名 fork COW 物理租约及 retained View／硬件写权限失效已有候选；非 mmap 文件 IO 的 cache 计费、回收、native extent 动态激活及 IO controller 仍待实现 |
| 05 Android 内存 | userfaultfd、process_mrelease | 实现 ABI、原 mm 生命周期、故障等待与回收；绑定仓库固定 API 33／arm64-v8a 镜像，确认实际 ART collector／lmkd reaper 及参数 | 镜像声明与已有解包 build.prop 已核对；collector／reaper 实际选择仍待确认，接口仍待实现 |
| 06 调试工具 | 实现侧现有 ptrace/proc 路径；live attach、多线程、gdbserver、strace -f 验收缺失 | 与其他改动一并检查生命周期依赖，准备原版工具场景；硬件断点及 SVE/SME 仍按原清单的独立支持范围登记 | 已准备原版 GDB／gdbserver／strace 会话夹具，覆盖 attach、多线程、降权拒绝及 tracer 异常退出；未运行，不据此宣布兼容 |
| 07 futex2／rseq | futex_waitv/wait/wake/requeue、rseq 注册和中止 | 共用原等待表／任务／CPU 状态；迁移、抢占、信号及退出接入两端 | futex2 四个接口及 rseq 注册／恢复／抢占／迁移／信号／fork／exec／退出已有候选；补上原版 no-libc ARM64 runfile 构建依赖，未执行 |
| 08 perf | 软件事件 FD、读／控制、采样环 | 接入原任务计数和 CPU 时间，不虚报硬件 PMU | 计数 FD、原 worker 软件时钟采样、mmap 环、丢样／回绕、控制、FD／映射／retained View、直接输出重定向、原任务继承及 exec、paranoid／mlock／频率准入 sysctl 已有候选；中断／开销预算、内核采样生产者、其余软件事件／作用域和映射策略仍待补齐 |
| 09 动态任务上限 | threads-max 当前只有快照；启动配置未接入 | 原 session 容量可写，proc 读写与启动值共用同一资源；不预分配 | VFS／proc／kernel 准入／session、manifest／iPad FFI／两端 CLI 与 Mac 环境启动已实现候选；已准备回归代码，未执行 |
| 10 allocator | 实验路径仍默认关闭；短调用／竞争性能缺少收口结论 | 核对并记录实现侧候选；是否扩大支持留给同轮集中测量决定 | 短调用／竞争／原版 Vim、Python 的固定输入、轮换对照、逐样本日志与失败记录已准备；未测量，不调整默认 |
| 11 持久化 | 等持久性工作负载及瓶颈归因未完成，WAL 未选定 | 准备候选设计／记录与统一基准；不以未经测量的 WAL 替代原要求 | 单／多写者 ACK 夹具、P99／宿主进程 IO 采样、SIGKILL／冷重开驱动已准备；真实包安装、容器层及瓶颈归因仍待补齐，未执行 |
| 12 空闲内存 | 冷启动、稳定空闲及停止后释放基线未统一 | 准备统一采样入口与配置，物理 iPad 为验收依赖 | FFI 显式进程快照、Mac／Linux 宿主驱动、显式启用的 iPad App 内测量场景已准备；未采样，干净安装／冷 App 启动和真机验收仍未完成 |

01 已完成，其已有结果按未受影响范围保留。所有表内事项及实现过程中发现的同轮依赖都先完成实现；纯验收／测量项准备场景后留到统一验证阶段，不借此提前执行测试。

## 当前验证检查点

所有者随后明确要求“按现有实现的先验一轮，现在积累的比较多了”。这覆盖下面原先禁止中途构建／测试的阶段安排：当前冻结功能扩展，先验证已积累候选；编译阻塞集中修复后运行受影响测试并汇总失败。它不缩减 12 项目标，也不代表未实现功能完成。

本轮结果见[候选验证记录](../validation/candidate-batch-20261124.md)。首轮源码清单与原始日志位于 `target/validation-existing-20261124-150910`，其中 `initial-source.json` 记录产品／Runtime HEAD、所有已修改及未跟踪文件的哈希。每个构建／测试阶段设置期限并独立保留退出状态；编译修复后的源码另行记录，不拿初始指纹代表后续候选。

本次集中检查点已完成：产品解释器／FFI 533 项通过；5 组原版 gVisor 共 166 个身份为 163 通过／3 参考一致跳过，覆盖全部 162 个 Linux 参考通过项。Runtime 首轮及失败复查合并为 2196 通过／22 失败／31 忽略，另有共享 OFD 偏移间歇失败和 threads-max 启动值在 worker 路径读回为默认值的端到端失败；严格 Clippy 仍失败。完整失败和依赖归组见验证记录，下一批跨领域一起实现，不逐项修一个就重跑矩阵。此段覆盖表内历史“未执行”的验证状态，但不把相应整项勾为完成。

修复阶段随后关闭了原 22 个失败、共享偏移间歇问题与 threads-max 端到端缺口。受影响四库合并 1760 项通过，产品 533 项通过，20 次并发偏移回归、严格 Clippy 及原版 gVisor 162 个有效参考项均通过；普通文件负载对照与范围限制详见验证记录。完整功能清单仍开放，不能用该检查点勾选 FUSE mmap、Android、perf 完整生产者或真机门槛。

## 执行约束

- 按领域顺序实现；每轮完成该领域内所有当前能实现的内容，仅为必要依赖跨领域修改，不以某个局部测试容易通过来缩减本轮范围。
- 实现阶段只做源码／协议调查、编辑和实现状态记录。不启动编译、测试、原生探针、CPU／命名空间矩阵或性能测量。
- 不新增协调线程、资源服务线程、轮询消息总线、全局算法锁或普通访存屏障。阻塞工作交还现有执行槽。
- 所有实现完成后，固定候选源码及产物，再集中构建、验证和测量；统一收集失败后进入下一轮实现。
- 保留现有 WIP、原始失败、源码／runner 身份和日志。通过证据只证明其实际覆盖范围；未完成前不更新发布引用或宣称整批交付。

## 恢复记录

2026-11-24 所有者已明确恢复工作。目标服务的恢复接口返回 `set_goal requires trusted source-turn context from CCA`，因此服务状态尚未成功切回 active；这不改变所有者对当前实现工作的授权。当前先推进上述实现，不新建或缩减目标，也不把接口错误当作工程阻塞。

后续 continuation 已重新获得 active 目标上下文，读取服务确认 objective 为“按照每次完成所有能推进的部分而非只做少量bug的原则逐步推进吧”。前述恢复接口错误为历史记录，不再是当前自动续跑状态。

## 已准备的集中验证内容（尚未执行）

- futex2：多地址同时 wake 只恢复原任务一次、返回实际命中下标、requeue 后取消完整清理、比较失败和超时不遗留可消耗 wake 的票据。
- FUSE：取消前后到达的 root LOOKUP／GETATTR／SETATTR 保留共享属性副作用；短／长回复及错误回复带额外数据结束原请求，guest 得到 EIO；INIT 短回复零扩展和超长回复终止。
- 任务上限：已有任务不因降低上限而撤销，升高后立即恢复准入；fork／FD 别名共用原资源；启动配置、proc 更新不会被快照覆盖；权限、取值、strict proc 偏移和 manifest／CLI 边界。
- rseq：注册／注销的参数与错误顺序，抢占／迁移／信号中止，post-commit 和无中断路径，fork 继承及 exec 清除，非法 CS／签名／flags；原版 gVisor 的 ARM64 no-libc helper 保留上游原文件和 runfile 身份。
- FUSE 截断：OPEN 不发送未协商的 O_TRUNC；SETATTR 成功才公开原预留槽位，失败释放句柄与槽位；FD 请求带原 FH，open 截断不带 FH；属性采用 daemon 回复值，实时文件大小上限与 SIGXFSZ；新创建文件的 MKNOD＋OPEN 回退不额外截断。
- FUSE 数据续接：pathname truncate 不依赖空闲 FD、不打开 daemon 文件，跨链接到普通文件后仍用已接受 inode 的原布局修改流程；独立 OFD 的 append 共享 inode 事务，取消未发送的事务不误发 INTERRUPT；F_SETFL、stream／nonseekable、NOFLUSH、同步写的原位置／writer 保留、FSYNC ENOSYS 和零长度 IO。
- cgroup 映射页：共享别名／fork 的首次归属与最终物理页释放、零额度新读者复用已有 cache、根归属不随读者迁移、私有映射普通／native／worker 写入、新限额失败与重试、重绑 RSS 不重复计费、首次私有写同时需要 cache 与 COW 页，以及硬件只读导出不能绕过首次写准入。
- cgroup 匿名 fork：父／子共享物理租约、实际 COW 计入写入者、最后持有者保留原归属、未受控根页迁移后 COW、失败 fork 归还临时额度且不改变父映射版本，以及 retained View 的已缓存 CPU／硬件写权限重新准入。
- perf 计数：单项／组 read-format、短缓冲区 ENOSPC、enable／disable／reset／ID、dup 与 CLOEXEC、组长关闭后的独立成员与重新分组、目标退出后冻结和 readiness HUP、FD 重用不重定向原控制、时钟不保留 ProcessView／Files、属性尺寸协商及未知功能拒绝。不能替代原版 perf stat／record 验收。
- perf 采样／映射：SAMPLE 的字段顺序／标识、head／tail 与非消费 peek／消费 poll、满环丢样和 LOST 回绕、pause／refresh 及再次启用、关闭 FD 后继续采样、最后 unmap 的 HUP、retained View 保留原 VMA／OFD；数据页 RW 映射上的 CPU／native／worker／硬件写入得到 SIGBUS，fork 排除设备映射；UID 免费额度含控制页、原 mm 的额外 pinned 计费及动态 RLIMIT 重试。析构不能重入组锁的准备回归有一秒期限，并在 join 前释放原锁，失败不会挂住整组。
- perf 输出／生命周期：重定向保留源事件 ID／格式和目的缓冲区身份，不能映射重定向源；最后映射撤销所有关联，观察者 Arc 不延长逻辑映射；零 wakeup-events 使用半环字节水位。准备覆盖子／孙任务 live 计数、跨原时钟的控制时点、退出聚合一次、inherit_thread、exec 一次性启用／移除、主 ID 与独立 stream ID，以及动态 sysctl 的 signed／严格偏移／权限／频率和额外 pinned 准入。
- FUSE 写入清理／xattr：准备覆盖原 capability 大小查询、清理成功后刷新／删除 SUID 和可执行 SGID、清理失败不发送 WRITE、原 FH／offset／writer 保留、xattr 等待与取消只影响自己的操作。已有数据夹具显式回答首次 capability 查询的 ENOSYS，零长度 IO 仍不发查询。
- 异步缺页与结果输出：准备覆盖原 PageWait 的多等待者／独立取消、root／retained worker View／native span 的准确地址及失败、hardware export 等待时不创建 shadow、未完成页不先计入 RSS；文件结果跨 FD 重用与 source 重新绑定不重做 IO、不覆盖已复制前缀；目录 cursor 只提交完整项的 cookie；AIO／io_uring 在目标页等待期间保留结果与 selected-buffer lease，完成拷贝后才发布 CQ／eventfd。接收器还覆盖锁外交付、坏回复 EIO、disconnect 和过期 weak 接收器。以上仅准备源码，未执行。
- 原位置与短读：新增 FUSE／普通文件／proc 的 ReadReply／ReadPosition 候选。准备覆盖 daemon 已回复但 copyout 尚未完成时 dup/lseek 仍等待、复制 0／部分／全部字节分别提交 0／实际前缀／全长；原 FD 被关闭并重用、source 重新绑定及目标页被 unmap 后，仅更新原 OFD 的实际短读位置。零前缀永久故障仍为 EFAULT，元数据输出不套用 read 的短读规则。原并发普通文件夹具已改为订阅已有 position readiness 后有界等待，不使用轮询代替新的事务。同步写还准备覆盖 FSYNC 成功、ENOSYS 成功回退与 EIO：数据写入不回滚，失败 syscall 保留原 f_pos。均未执行。
- FUSE 清页资源：`fuse/cache_tests.rs` 准备覆盖同页请求合并、短读补零／尺寸更新、原失败结果与后续重试互不覆盖、在途失效后的独立旧／新物理页、独立取消／最终取消、读取方 cgroup 归属与回复方零额度、quota 失败不发送 IO、迟到 EOF 不撤销更新的属性，以及最后一个 signed 文件页的 4095 字节边界。通知准备覆盖正 opcode／zero unique、精确长度、未知 opcode／inode、负 offset 仅失效属性及非正 length 到 EOF；当前凭据／连接原 PID namespace 和 TLS 嵌套恢复也有准备源码。未执行。

## 本次推进的实现位置

- 07：`runtime/crates/trames-kernel/src/rseq.rs` 保存每任务注册；原 execution slot 返回 guest 前和信号入帧处处理事件，fork／exec／退出沿原生命周期传递或清理。正常未注册任务只做原子状态检查，无新线程。原始 helper 的构建依赖位于 `runtime/tests/gvisor/build_graph.py`，对应准备回归为 `build_graph_test.py`。
- 06：`runtime/tests/fixtures/debugger-sessions.c` 与 `.py` 驱动原版工具；每个自建进程组有独立期限，保留原 PID 和日志，不通过进程名称清理系统任务。
- 10：`tests/fixtures/musl-allocator.c` 新增单线程短调用模式；`tests/performance/allocator.py` 对照同一 libc／ELF／rootfs，保留每个原始样本和当前源码产物身份。原算法及实验默认开关未变。
- 11：`runtime/tests/fixtures/storage-durability.c` 在同步返回后写独立 ACK；`tests/performance/storage-durability.py` 使用每行独立 store 副本，在选定 ACK 后杀死本次创建的 runtime 进程组，再用新进程验证已确认追加前缀。单次和整批均有期限，成功副本可清理，失败 store 与日志保留。宿主 IO 数是有明确进程范围的采样下限，不能当成整盘写入量或断电持久性证据；不同 barrier contract 不混算，Apple native 普通 fsync 不自动等价于产品的 device flush。
- 12：`trames_process_memory`／`LocalSession.processMemory()` 显式读取当前进程 RSS、Apple physical footprint 和尚存 session worker 数，不唤醒 guest、不加采样线程。`tests/performance/idle-memory.py` 固定声明、rootfs 与组件，分别记录可选管理器和当前 embedding 进程；worker 归零后再采样停止阶段。`LocalMemoryProbe.swift` 与显式启用的 `LocalMemoryTests.swift` 在实际链接 runtime 的 App 中准备同样阶段。采样逐行写文件，不在被测进程累积全部样本；Mac／模拟器结果不冒充物理 iPad，fresh session 不冒充 clean-install／冷 App 基线。
- 02 数据补全：`PathOperation::Truncate` 贯穿原路径续接和 proc magic／符号链接；普通目标使用原 `FileWork` 布局准入。FUSE 共享 `Attr` 保存原 inode 的写／SETATTR／xattr 修改事务，等待与取消只使用已有 readiness；FD 数字不在等待后重新解析。xattr 获准后重查 ACL／capability 修改权，SETXATTR 的 SGID 标志在发送时按当前 inode 计算。开放 O_APPEND／O_DIRECT，并保留原 OFD 的可变标志；append 使用事务内规范 size，写入后失效属性。`mount/killpriv.rs` 在非空普通文件 WRITE／fallocate 前查询、删除 capability，必要时以 inode 为单位 GETATTR／SETATTR 清理 SUID／可执行 SGID；原数据请求及其 FH、偏移、writer 与 inode guard 保持到续接。与 chown／truncate 共用 capability 回复处理，移除 ENODATA 传播，EOPNOTSUPP／ENOSYS 按 commoncap 规则处理。保留 daemon 的 stream／nonseekable／NOFLUSH 结果；WRITE→FSYNC 续接保留原字节计数、位置和挂载 writer，ENOSYS 按文件／目录 opcode 分别记忆。完整缓存／通知／映射及非普通文件等依赖仍未完成。
- 02／04／05 的异步页基础：`trames-memory/src/page_fault.rs` 保存原页请求及独立 readiness 订阅；`MemoryError::Pending` 携带实际访存地址，不作为 guest errno 或同步信号。原 CPU／native 故障返回与 run queue 的 Page 等待保存现场、交还执行槽，完成后重试原访存；重新绑定保留新通知对象并移除旧订阅。root Memory、worker View、native span 和硬件冷准入在 page-in 成功后才计入该 mm 的 RSS；原 cgroup／COW 准入仍在实际访问处生效。FUSE `transport.rs` 的 weak 字节回复接收器在入队前绑定，正常、畸形及断连回复都在连接锁外交付，不新增处理线程。

02 的 `trames-linux/src/direct/file_copyout.rs` 候选保存原返回值及逐页拷贝位置；直接文件完成、原 owner FileWork 完成、namespace 输出、AIO 和非 pinned 的 io_uring 文件输出复用它。等待不会再次执行文件请求或解析数字 FD；目录输出的原位置事务保持到完整项拷贝后才提交 cookie。AIO CQ／eventfd 与 io_uring CQ／selected-buffer 释放保留在输出之后，完成计数不在重试时重复累加。单段输出及 stat 的 body／cursor 内联，普通读直接移动原 Vec，避免为新续接额外创建 Arc 或单项队列；吞吐和栈成本仍留到集中测量，不据源码声称性能已通过。准备回归分别位于 `page_fault_tests.rs`、`file_copyout_tests.rs`、原 AIO／uring subsystem tests 和 FUSE receiver／directory tests。

02 的读取位置候选由 `trames-vfs/src/read_reply.rs` 保留原 OFD／位置事务；拷贝记录每段成功长度，完成或取消释放时只提交实际前缀。显式的 owned-byte 消费者通过 `FileValue::into_bytes()` 接受全长，不提前丢弃 ReadPosition。FUSE、普通 FileWork、DirectFile 及本地／远端 proc 读取已接入，非定位 IO／seek 的原位置等待与既有 native disk-read 的最终通知共用 FilePosition readiness。PreparedFile 新增的 original OFD 引用仍为 weak；仅实际接受的 DirectFile 提升成原完整 OFD，保留 close／OFD-lock 生命周期，不让缓存持有已关闭文件。FUSE 同步写在 WRITE 回复后保留候选终点，直到 FSYNC 成功／ENOSYS 才提交位置；失败只释放原事务，不撤销已写入数据。

02 的 `fuse/mount/cache.rs` 新增 inode 持有的清页资源，读取请求只由原等待者强持有，页仅 weak 引用当前请求，不形成 File→Attr→Cache→request→File 环。同页等待者共用原 READ／FH；完成结果归原请求，失败后新调用能重试且不替换原失败。失效移出当前页身份，旧请求保留自己的页，迟到回复不会回填当前 map。物理页分配前由读取方 `MappedResidents::reserve_cache()` 归属，不在 daemon 回复时收费；普通 WRITE／fallocate、非 KEEP_CACHE OPEN 和规范尺寸变化清缓存。原 worker 分配边界的 `FileActorScope` 保留实时 CredentialState／TID，冷请求按连接的原 user／PID namespace 转换；不缓存 mmap 时凭据，不添加普通访存锁。`NOTIFY_INVAL_INODE` 在 transport 锁外处理，更新版本、属性／ACL 和指定 clean range，保护新属性不被旧短读收缩。WRITEBACK_CACHE 仍未协商。

这仍不是 FUSE mmap 的完整接入。清页资源尚未作为 PageSource 接入普通 buffered IO／mmap；故障持有的原请求结果、VMA 撤销／重绑、缓存回收和映射页失效仍需一起接通，不能把 cache map 移除当作已完成映射失效。syscall 输入导入、byte/socket／wait／signal 等其余 copyin/copyout、原生及设备调用的副作用边界、共享写回、其余通知及卸载／取消仍需同轮完成。当前没有让 FUSE mapping 返回成功，也不以整次 syscall／原生算法重放来处理已经产生副作用的页等待。读取候选还需与普通同步写的其余入口、Cgroup 等仍走 byte capability 的位置等待、原 IO／inotify 统计发布时点，以及其余 syscall 输出的原资源释放边界统一；pread／readlink／xattr 和 stat 的候选区分见下；不能据局部位置夹具宣布所有 syscall 输出已覆盖。
02 的 `output_resources.rs` 新增 `FileResources`：正常读取的 ReadPosition／目录位置继续保留其原资源，pread 等未定位读随 FileReply 保留完整原 OFD，copy／stream transfer 还保留原输入／输出引用。source worker、owner fallback、DirectFile fast result、AIO 和 io_uring 把资源移到原 FileCopyout，完成／错误／取消才释放，不重新查数字 FD。普通 FD 资源内联；冷路径的 OutputScope 只收集本次实际解析出的 inode／mount／magic-link／xattr target，不保留 Files、完整 VFS、active work 或 layout guard。收集帧只存在于本次已有宿主调用，完成资源才跨等待移动；没有新增线程。

这里按 Linux 原边界区别处理：pread 在用户拷贝期间保留原 OFD；readlink 保留 path／inode，空路径查找不额外延长 O_PATH FD；get/list xattr 保留原 FD 或 path。stat／statx／statfs 在取得元数据快照后已释放源引用，不能为它们额外延长 OFD 锁或 FUSE lookup 生命周期。之前将 fstat 也笼统列为必须保留完整 FD 的判断已依据原源码修正。准备回归使用第二个 OFD 的写锁检查 pread／fstat 的不同释放时点，并覆盖拷贝完成／取消、数字 FD 重用后仍输出原字节／stat 快照；另有 unlink 后 readlink 原 inode／O_PATH FD 的不同寿命、xattr 成功／失败和嵌套资源收集。仅新增源码，未执行。

普通 worker path query 的 readlink／stat／fstat／statx 原本仍直接写 guest 页，Pending 会被转换成 EFAULT；`trames-linux/src/path_queries.rs::prepare_output` 现在只接受原结果，`DirectSyscalls::invoke_path` 统一交给 FileCopyout，FUSE／native walk 仍用原续接，不重复 pathname／FD lookup。statx 不再在元数据查询前按目标页是否已驻留触发输出错误，实际页权限／错误由输出逐页处理。readlink 的 bufsiz 也统一按 signed 32-bit 参数解释：低 32 位非正先返回 EINVAL，正数按该值截断；owner 同步入口复用该 ABI 转换。新增准备覆盖四个普通 path 输出在目标页等待后 unlink／替换路径／重用 FD，恢复时只复制原结果、完成计数一次，以及 readlink 的高位截断和非正长度先于路径导入的错误顺序。未运行。

此层仍需与 syscall 输入导入、byte/socket／signal／wait 续接、原 IO／inotify 计数发布时点、整个 native／设备副作用边界和 FUSE PageSource 统一，不能据这些资源夹具宣布全部输出路径完成。原资源与环／FileReply 增加的栈和冷分配成本同样保留到集中性能验证，不据源码声称无成本。

02 的普通文件同步写候选现在由 `file_data::WritePosition` 保存候选终点，复用原 `FilePosition.pending`／readiness，不在同步期间持有位置／inode 锁。原 `Content::File` 写入的 O_SYNC／O_DSYNC 和 FileWork WriteSync（含 descriptor owner 回退）在 sync 成功后才提交 end；错误只撤销位置候选，已写数据保留。位置事务在返回错误时也释放，不由后来的 fsync 补交。`storage_tests.rs` 已准备目录同步失败注入，覆盖 append／非 append、O_SYNC／显式同步及 descriptor owner 回退、RWF_SYNC 加强 O_DSYNC、失败后原偏移与数据保留和后续成功写的实际终点；未执行。`fd_status::write_sync` 与 FUSE 共用有效同步模式选择，O_SYNC 不降级为 data-only，显式 RWF_SYNC 加强 O_DSYNC。宿主专用入口／非文件描述符的同步语义仍需按实际调用路径集中核对。

- 04 映射页计费：`cgroup_memory.rs` 的物理页租约与 `MappedResidents` 中的共享归属单元独立于每 VM 的 RSS。相同 cache 页只取一次 cgroup 额度，cache／映射资源最后释放才还额度；未受控的根归属也被记住。私有文件映射读取仍属于 cache 首个所有者，首次写在原执行者获取新的私有租约；RSS 的 resident 状态与可直接写的状态分开，CPU／native 预检与硬件可写 PTE 使用同一准入。冷转换使用页对象锁，替换了原 first-touch 自旋；正常访存不增加屏障、全局锁或服务线程。回归候选位于 `runtime/crates/trames-memory/src/cgroup_cache_tests.rs`，未执行。

04 的匿名 fork 候选进一步落在 `residency.rs`、`lib.rs`、`parallel.rs` 及 `parallel/hardware.rs`：每 mm 的 RSS 仍独立准入；原物理页租约由父／子共享，首次真正 COW 才向当前写入者收费。最后一个持有者不因迁移而重新收费；根归属只在 fork 需要身份时建立轻量标记。所有可失败的子 RSS／fork policy 完成后才撤销父写准入并发布原 Account 的 COW epoch；失败 fork 不改变父映射版本。retained View 的 CPU paid TLB、保护 MMU 快路径和硬件元数据均复用该页准入，硬件按既有映射 token／chunk stamp 失效。回归候选为 `runtime/crates/trames-memory/src/cgroup_cow_tests.rs`，尚未执行。native extent 状态仍有独立的动态激活／计费依赖，不能据此宣布所有匿名页路径已完成。

04 还需补非 mmap 文件 IO 的 cache 归属、物理 cache 回收及 dirty／pinned 页处理、native extent 在 cgroup 激活／迁移后的首次页准入，以及 IO controller。这些尚未实现，不由现有映射页／fork 夹具替代。

08 的候选位于 `runtime/crates/trames-vfs/src/perf.rs`、`perf/{ring,sample,accounting,output,inheritance,settings}.rs` 与 `runtime/crates/trames-kernel/src/perf.rs`，接入现有 source worker、FD／OFD、字节读取、ioctl、mmap 和 poll／epoll。FD 只保留原任务计时资源，避免引用完整 ProcessView 及其 Files；计时沿用执行分配的 begin／finish。采样在原 worker 的实际 CPU 执行片段返回时检查，到期只记录该时点的真实 guest PC，不补造历史样本，不创建计时线程。未启用采样时不采时钟；禁用／关闭最后一个采样事件后，检查在下一边界撤销，重新启用通过原资源 revision 准入。一次实际采样的 CPU 时间与记录时间共用同一个宿主时间点。尚未建立独立的采样到期执行中断，片段长度和原生调用导致的 skid／覆盖质量仍须随实现与真实工具验收收口，不能据此宣称原版 perf record 已支持。

08 的环保存 1＋2^n 个共享页，控制页发布 count／time／head，数据页支持 forward 非覆盖及只读 overwrite；full／pause 的丢样保留实际源事件总计，后续空间充足再写 LOST。SET_OUTPUT／FD_OUTPUT 捕获已有且仍映射的原缓冲区，避免 Event→Event 链；映射 lease 计数独立于观察者 Arc，最后 unmap 撤销全部源关联，旧环不能撤销新映射。样本身份／格式由源决定，只有 mmap 所有者更新控制页；共享环的默认字节水位与源事件额外 wakeup-events 分开处理。原 Event 最终释放只记录关闭时点并撤销后代准入，由组冷边界清理，不在析构中重入组锁。映射页保留原 OFD，`BackingIndex` 的显式设备 lease 在 retained View 中保留同一资源，不借无写回页的 sync 路径延长所有普通访存。控制页可写、数据页 SIGBUS 和 noexec 与其他内存路径共用。免费额度和额外 pinned 计费按环境／全局 UID／原 mm 维护；三个动态 sysctl 共用原环境 Settings，mlock 与 RLIMIT 在后续 mmap 重新读取，paranoid 小于零及初始命名空间 IPC_LOCK 的豁免保留。

08 的继承候选接入原 child birth 与成功 exec：每子任务独立保留实际时钟／组，所有后代直接归入最初事件族；原 FD 控制按子时钟的实际时点应用，live read 更新子计数，退出／remove-on-exec 聚合一次。没有新增任务引用或扫描 PID 的服务。inherit_thread、父退出后的后代计数、一次性 enable-on-exec、remove-on-exec 以及 ID／STREAM_ID 的区别均有准备回归。按 Linux 禁止 inherited per-task 事件直接 mmap 及 REFRESH，仍可重定向到同任务的其他已有环。软件 CPU／task-clock 的计数不因采样过滤拆分；已接受用户采样过滤，内核独占采样因缺少真实生产者仍拒绝。频率仅在 open 做动态上限准入，软件 PMU 初始化一次转换成周期并清除 freq，之后 IOC_PERIOD 使用纳秒；固定 10us 下限保留。

08 的 no-expand 候选由 backing 的冷 VMA 策略贯穿 Memory remap／零大小 alias 和 Linux mremap 选址前检查，增长返回 EFAULT，保留完整等长移动；已有初始 executable 准入也统一检查 backing 的 noexec。准备回归保留失败后的原环和等长移动身份。没有给普通字节访问增加策略查询。

08 仍需采样中断与 per-CPU／开销预算及相关 sysctl、内核采样生产者、其他软件计数、CPU／cgroup 作用域、完整权限细节，以及映射的拆分／dump 策略和跨 mm 别名计费核对。版本调查已取得 v6.12 原源码：该版本 perf vmops 没有 may_split 拒绝钩子，且重定向源再次 mmap 的行为与当前 Linux 的 EBUSY 检查不同。因此不能把最新内核的拆分禁令直接当作无版本差异的要求；这些候选与本批原生参考的版本关系仍需明确，差异保留到集中对照，未据此执行探针。准备回归位于 `perf_tests.rs` 与 `sysctl.rs`；没有运行 perf 工具，没有编译或测试这些候选，不标记 08 完成。

上述实现记录形成时尚未构建、运行测试、原生探针或性能测量；当前已按最新指令进入现有候选验证，结果另行记录。以上仍为候选实现／准备状态；04、05、08 和 02／03 的剩余依赖继续留在同一整批中，不以这些测量脚本准备完毕作为进入验证阶段的条件。

08 的 ABI 调查使用 Linux 原始 [`perf_event.h`](https://github.com/torvalds/linux/blob/v6.12/include/uapi/linux/perf_event.h)、[`fs/read_write.c`](https://github.com/torvalds/linux/blob/v6.12/fs/read_write.c)、[`ring_buffer.c`](https://github.com/torvalds/linux/blob/v6.12/kernel/events/ring_buffer.c)、[内核 ring 文档](https://www.kernel.org/doc/html/v6.12/userspace-api/perf_ring_buffer.html) 和维护者的 [`perf_event_open(2)`](https://man7.org/linux/man-pages/man2/perf_event_open.2.html)，并对照 [v6.12 `events/core.c`](https://github.com/torvalds/linux/blob/v6.12/kernel/events/core.c) 与当前 Linux 镜像的 [`events/core.c`](https://codebrowser.dev/linux/linux/kernel/events/core.c.html)、[`anon_inodes.c`](https://codebrowser.dev/linux/linux/fs/anon_inodes.c.html)，核对默认额度、O_RDWR、poll 消费、设备页权限和版本差异。FUSE 写前清理还对照 [`fs/inode.c`](https://codebrowser.dev/linux/linux/fs/inode.c.html) 与 [`security/commoncap.c`](https://codebrowser.dev/linux/linux/security/commoncap.c.html)。候选存在不说明默认 perf 命令或全部 Linux 行为已对齐，仍按上表实现／验收。

协议实现依据为 Linux 原源码：[`kernel/futex/syscalls.c`](https://github.com/torvalds/linux/blob/v6.12/kernel/futex/syscalls.c)、[`kernel/rseq.c`](https://github.com/torvalds/linux/blob/v6.12/kernel/rseq.c)、[`fs/fuse/dev.c`](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dev.c)、[`fs/fuse/file.c`](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/file.c)、[`fs/fuse/dir.c`](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/dir.c)、[`fs/namespace.c`](https://github.com/torvalds/linux/blob/v6.12/fs/namespace.c)、[`fs/fsopen.c`](https://github.com/torvalds/linux/blob/v6.12/fs/fsopen.c)、[`kernel/fork.c`](https://github.com/torvalds/linux/blob/v6.12/kernel/fork.c)。`threads-max` 的启动默认值最小值常量与 sysctl handler 的可写下限不同，候选代码按后者允许 1..=1073741823。

输出资源边界依据为 Linux v6.12 原始 [`fs/stat.c`](https://github.com/torvalds/linux/blob/v6.12/fs/stat.c)、[`fs/statfs.c`](https://github.com/torvalds/linux/blob/v6.12/fs/statfs.c)、[`fs/xattr.c`](https://github.com/torvalds/linux/blob/v6.12/fs/xattr.c) 与 [`fs/read_write.c`](https://github.com/torvalds/linux/blob/v6.12/fs/read_write.c)。
