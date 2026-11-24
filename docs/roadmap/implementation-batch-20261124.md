# 2026-11-24 实现批次

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
