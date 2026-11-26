# cgroup 领域实施批次（2026-11-26）

状态：功能实现及集中验证完成（2026-11-27），按 main 配对交付。功能交付 Runtime `8bfc0948`、性能实现 `9949199c`；整合远端窗口协议修正后按 `08c53fdf` 配对推送。整合后 Wayland Apple 37／Linux 36 项通过，复用未变 cgroup 的证据。冷读性能后续已实现并验证，HVF IO 比上一版快 4.01%，相对 cgroup 前基线仍有 +2.63% 的实测中位差值，详见[收尾记录](../../runtime/docs/validation/cgroup-closeout-20261127.md)。

## 当前进度摘要

- 最终性能收尾：缓存与 IO 冗余开销已成批修复，两库 Apple 1,546／Linux 1,556 通过，各 4 忽略；原版用例两执行器各 346 通过／2 跳过，真实资源约束及 iOS Rust 通过。三版本六顺序对照 HVF IO 六块均改善，对上一版 −4.01%、对 cgroup 前基线 +2.63%；当前已完成本领域收尾，历史各次测量完整保留。

- 01 交付阻塞、02 FUSE、03 现代挂载的已交付基线保持；本批完成 04 cgroup 及必要调用方，没有新增生产协调／服务线程。
- 物理内存首次归属、file／tmpfs／memfd／native 页、层级 IO、保留式等待、dirty 写回／原 mm 回收、限额／通知／OOM，以及文件、异步 IO、exec、host 传输、overlay 和私有元数据边界均已实现。
- 集中首轮失败已成批修复。最终 Runtime 清单在 Apple 2,773 通过／31 忽略，Linux 2,784 通过／15 忽略；实际内核组合两侧各 546 通过／3 忽略，FFI 32 项通过。原始失败、权限复核及各阶段记录保存在 `target/validation-cgroup-20261127/` 与 `target/local-ci/io/cgroup-20261127/`，未抹去历史失败。
- 同一原版 gVisor ELF 的 348 身份在两执行器均为 346 通过／2 原始跳过，覆盖全部 345 原生通过项；最终解释器 cgroup2 子集 85 通过／2 跳过。历史全量台账不增长。
- 实际工作负载验证了匿名物理内存增长、真实磁盘限速及子进程 OOM；原生 Linux／解释器／HVF 都通过。Apple／Linux 三轮真实宿主字节释放也通过。iOS ARM64 Rust 静态库已构建并校验源码摘要。
- 交付前已无冲突快进整合远端 Linux 桌面主线到 `aadee76`，保留已发布提交身份；最终两宿主严格检查、产品检查及配对产物均通过。普通负载最终空闲对照：解释器 +1.16%／+1.70%／+2.34%，HVF 文件 IO +7.72%、元数据 +0.48%、管道 +0.57%；HVF IO 的 39.586→42.644 ms 及四配对块一致增长作为明确优化项保留。
- 完整证据及边界见[本轮验证记录](../../runtime/docs/validation/cgroup-resources-20261127.md)。以下日志按历史检查点保留当时的“未验证／待实现”状态，不能覆盖本节的当前状态。

## 本批范围

1. 物理内存归属：保留已有匿名页、映射别名和 fork/COW 的首次物理归属；补非 mmap 文件缓存、tmpfs 和原生 extent。迁移只影响新分配，既有页保持原归属；共享页只计一次，最后物理引用释放后归还祖先额度。
2. 资源上下文：异步读取、页入、AIO/io_uring 和写回保留接受请求时的内存及 IO 归属，在已有 worker 上恢复；不能把实际分配计给送达回复的线程。OOM 与 high 反馈仍回原请求任务。
3. 实际回收：使用可重建缓存的真实生命周期，释放字节和额度；dirty、映射及执行中的 pin 不可静默丢弃。额度不足先尝试回收，仍不足才报告 OOM。提供相符的 memory.stat 和 memory.reclaim 行为，不用逻辑计数下降代替物理释放。
4. IO controller：实现层级 io.max、实际后端的 io.stat，以及配置更新／取消／部分完成。覆盖普通读写、page-in、写回、复制／splice、异步入口；tmpfs、管道和 socket 不冒充磁盘 IO。受限请求使用现有 deadline/readiness 续接，阻塞时交还执行槽。
5. 原生内存：处理已有原生 extent 在 cgroup 启用、迁移和限额变化后的准入与归属；保持原生和解释器统一访存／权限模型，不通过额外全局锁或协调线程补账。

## 实施边界

- 文件存储容量配额与 cgroup 物理内存计费分别保留。
- 资源级锁／原子操作只保护实际共享状态。回收回调不持有 cgroup 额度锁，也不阻塞等待正在使用的文件／页锁。
- 沿用平台无关资源接口；Apple 专用实现使用现有 target cfg，检查 Linux 与 Apple 两条路径。
- 不纳入其他会话的调试、allocator、存储持久化、空闲内存或 CI 工作树修改。

## 集中验证标准

- 内存：共享、fork/COW、迁移、退出、祖先限额；非 mmap 与 mmap 混合访问；实际缓存释放、dirty/pinned 页、回收失败与 OOM；原生 extent 首次触碰及迁移。
- IO：真实磁盘负载在 BPS／IOPS 和祖先限制下的吞吐；读写分别计数；异步／短传输／取消／配置变更；无额度重复消费、无执行槽占用等待。
- 复用未变用例的既有证据，集中运行受影响 Runtime、Linux/Apple 编译、解释器与 Mac HVF、相关原版 gVisor 和实际工具／正常负载性能。
- 实现及验证完成后更新领域清单，提交推送 main 并同步两端 Runtime 引用。未完成项不勾选关闭，不增加历史全量 gVisor 用例计数。

## 实现记录（保留各检查点当时的验证状态）

- 批次起点核对：io.max 是空视图且拒绝写入；当时普通文件读取尚未使用可回收的统一缓存；文件容量配额不能作为内存计费依据。
- 已落未验证候选：接受请求时的内存上下文在 FileWork 和冻结 FileActor 中保留原归属与原任务压力反馈；物理租约区分 anon/file/shmem；祖先额度不足先回收再决定 OOM。
- 已落未验证候选：私有磁盘的普通读取和 mmap 共享字节及首次物理租约；写穿更新缓存，截断清零原页；回收跳过 dirty／映射／执行 pin，并实际释放最后字节引用。HostFS 继续保留外部写者的一致性路径，不能直接使用私有存储缓存假设。
- 已落未验证候选：memory.stat 与 memory.reclaim 入口；实际内存回收不足返回 EAGAIN，无 swap 时不会丢弃匿名／shmem 数据。已添加跨 worker／迁移、祖先压力、dirty/pin 及普通读取与 mmap 混合访问的回归场景，尚未执行。
- 已落未验证候选：FUSE 普通／批量 READ、buffered write 和 STORE 的租约登记到原 inode；直接回收只删除无请求、无映射、无外部字节 pin 的干净缓存，dirty 页继续走既有写回。对应请求／dirty／pin／重新读取场景已添加，尚未执行。
- 已落未验证候选：tmpfs 普通读写、mmap 和 copy-up 统一物理页；shmem 分类和失败时内存／存储准入回滚。完整页截断使文件别名重新缺页，旧 CPU／provider 字节 pin 保留旧物理租约；新分配归实际新调用方。临时的无内容页只保留元数据，不代表实现了 fallocate 的 cgroup 计费。
- 已落未验证候选：PageSource 增加仅在物理字节替换时变化的 generation；快照直接复用与共享源 intern 同时核对该 generation，避免截断再增长后复用旧 loaded 字节。普通数据写入继续使用原版本／区域访问协议。
- 已落未验证候选：内存控制启用／迁移通知原任务；worker 在原 mm checkpoint 撤下旧 coarse export。已分配原生 granule 保留原 bytes／归属，新页继续使用已有逐页准入的硬件执行路径。全部共享任务回到 root 后可重新准入尚未触及的 coarse extent；没有关闭硬件执行、复制整片虚拟地址空间或重收既有 RSS 总额度。
- 已落未验证候选：内部 IO 层级预算、真实请求大小的 credit 累积、部分完成退款、取消不消费额度、原组退役通知和统计聚合。后续已接入下述控制文件、任务上下文及私有磁盘数据入口；全部实际后端和调用方仍未覆盖，尚无有效限流的运行证据。
- 已落未验证候选：fallocate 用具名零 extent 一次准入祖先额度；materialize 的 slice 继承首次归属。截断／退役释放无 pin 页，slice 使用资源内的对象计数处理最后 Arc 析构竞态。tmpfs 预分配与文件容量一并回滚；同长度 truncate 释放 EOF 外的 keep-size 预分配，fallocate 内部不再误调用同长度 truncate。
- 已落未验证候选：SharedFile 增加平台无关的资源计费／pin 接口，写入、预分配、原生映射和 FD 传递使用原 inode 的同一份 shmem 归属。新分配沿用原 charged→resize 顺序；已有字节操作继续并发，不持有新增算法锁。大零 extent 不展开 host buffer／页图；短写退还未使用的预留，受限写入保留已完成前缀。
- 已落未验证候选：memfd mmap 只建立元数据，首次缺页才映射并计费；共享别名使用同一弱 canonical SharedPage。截断使源 generation 与旧快照失效，保留旧 native pin 的租约。原生 FD export 捕获发送时 actor；无显式 actor 的 native consumer 恢复该上下文，显式 guest／root actor 优先，不把分配计给送达线程。
- 已落未验证候选：private COW 独立记录完整页截断代次；普通缓存换页／文件写入保留私有副本，部分 EOF 页的 COW 也保留。完整页截断后，直接读取、原生读写、硬件准入和 joined 快照使用新源；输出 preflight 的新准入保留到实际写入，不重复消费旧已驻留 COW 的额度。失效 PTE 退出 RSS，旧字节／快照的物理租约继续保留到最后引用释放。
- 已落未验证候选：FUSE 页将稳定映射身份与物理 allocation 分开，完整页截断、干净失效及延后写回失效换代；READ 保存原 allocation，晚回复不填回新缓存，已完成和命中缓存的 READ 也保留原字节。需要物理副本的回复恢复接受请求时的内存上下文；失败且未生成字节的 READ 退还预留。新加入截断／迁移／旧 pin、命中与晚回复、失败后退款的回归场景，尚未执行。
- 已落未验证候选：joined reclaim 清理旧 private PTE 及对应缓存快照；有尚未提交的 worker 写入时不退休，外部保留的真实快照／pin 继续持有原物理租约。私有失效使用独立的冷通知代次，普通文件变更和正常 reclaim 不展开页表扫描；后续回收也识别前一轮因外部快照暂不能释放的旧 charge／source。discard 改用新 PTE 归属，保留旧快照的物理租约。
- 已落未验证候选：平台无关的 ResourceWait／ResourcePending，支持同一原资源的多个配置／完成通知和动态 deadline。PageWait、CPU 缺页、复制输出、AIO／io_uring、信号／子任务输出等待接入多订阅；已有等待队列从订阅读取当前到期时间。普通 readiness 订阅不增加字段或新的线程／轮询源，已知单一完成源的 FUSE 订阅保留原接口。
- 已落未验证候选：FUSE 压力写回入口复用原 SyncExecutor／OFD／inode 写事务，按物理页原 cgroup 选择 dirty 页，合并相同代次；完成后再调用实际 reclaim，失败／取消不假装释放内存，同一未改变脏页代次不无限重试。Sync 保存接受时的 FileActorSnapshot；执行器退役通知等待者。此入口尚未接入 memory.max 准入失败或 memory.reclaim 的自动等待，不能据此宣布压力下 dirty 回收已闭环。
- 已落未验证候选：SharedFile 的内部资源等待携带已完成写入前缀，避免将内部等待作为一次成功短写交付；shmem 准入保留原类型等待。FileWork 写入 cursor 和同步执行器已接入候选续接，完整调用边界与压力回收仍待完成，自动内存准入等待尚未启用。
- 已补未执行场景：配置变更的多订阅／动态 deadline／取消、内部 IO 错误的类型保留、共享文件写入前缀续接、写回去重与失败后真实额度，以及同 inode 两 cgroup 的选择性 FUSE 写回和执行器退役。未构建、运行或提交本批候选。
- 已落未验证候选：同步执行器保留原 SyncResource 的 Pending，释放执行槽和原 layout admission 后在 FileLayouts 等待。沿用原通知／deadline 队列，资源就绪后重新申请同一文件的布局租约；完成与退役仍只交付原回调一次。订阅回调不运行文件 IO，也没有把任务／内存权限加入 HostProtocol。已补未执行的进度续接、等待期间新 reader、退役取消和无通知到期场景。
- 已落未验证候选：PlainFile 与 SharedFile 的对象内事务租约；在原数据／charged 锁准入内检查，等待时不持有该锁。原生共享别名、普通写入、截断和预分配使用相同资源；已有映射保留原 CPU／协议访问模型。直接 PlainFile 写入在事务忙时于效果前回退，普通读保留原字节访问协议；已有 SyncExecutor 的页写回可推进回收，不因前台等待形成反向依赖。FileWork 的普通写入 cursor 已使用该租约，数据阶段结束后释放，再进入同步阶段。
- 已落未验证候选：每次请求作用域的 VFS Resource IO 错误桥，保留原 PageWait／进度，嵌套和取消会恢复原作用域；数字错误码不作为内部等待依据。普通映射 FilePage 的 materialize 将桥内等待恢复为真正 MemoryError::Pending；文件缓存／tmpfs 准入不再丢弃等待类型。SharedFile 已完成前缀交给原 VFS 元数据处理，同时保留类型化等待；FileWork、joined／inline 入口已增加请求作用域和续接，其他入口及回退仍待审计，自动限额等待尚未启用。
- 已补未执行场景：原生别名跨线程事务归属、旧写者排队后在原锁内重检、截断与后台写回、PlainFile 直接路径回退、嵌套请求和取消的等待归属及 IO 前缀跨错误边界保留。测试代码静态核对时同步纠正了先前 SharedFile read_at 夹具的参数类型及原 IO errno 的整数转换；没有运行测试或构建。
- 已落未验证候选：FileWork 普通写入保留整个原 Vec 和字节 cursor，不移动／复制前缀；在首字节前保留共享 OFD 的位置，append 起点在原数据事务内选定。恢复时跳过自己的 pending 位置，使用固定偏移和剩余切片；真实错误保留已写前缀，数据阶段被信号中断时交付前缀而不重放。退出释放原事务并保留相应位置；同步阶段失败则保持原位置回滚。已补追加竞争、FD 关闭复用、零／部分进度中断、真实错误与失败同步的场景，尚未执行。
- 已落未验证候选：普通受控文件的 DirectFile 读写在效果前退回可保留请求的路径；joined／inline 入口显式保留接受时的 root／cgroup 内存上下文。内部资源等待释放普通 layout admission，晚到 FUSE 元数据保留原请求并恢复原写入 cursor，不以元数据结果完成整个写入。
- 已落未验证候选：原普通映射 SyncResource 与文件 fsync／fdatasync／同步写保存完整的嵌套 provider 写回计划，按页保留已写偏移及后端同步阶段；完成的页不重新标脏，未完成的后缀重新读取当前字节，取消／退役恢复未完成脏页。每页沿用原写回锁增加可等待的归属，不创建执行线程；完成通知和页引用释放移出 inode／数据锁。已补嵌套映射、部分页、后续并发写、等待者、取消及退役的场景，尚未执行。
- 已落未验证候选：sync／syncfs 保留原文件列表、当前文件及 checkpoint 阶段；后面的文件等待时不重放已完成文件。sync_file_range 保留原范围与 flags，范围外脏页不参与；wait-only 不启动新写入，零 flags 保留无效果路径。HostFS 的最终范围同步沿用原平台接口／cfg，避免重复调用后端。已补文件级续接、范围筛选和仅等待的场景，尚未执行。
- 已落未验证候选：截断／fallocate 保留原 descriptor、对象内事务、准入结果和预留／写回／EOF／元数据／checkpoint 阶段；shared/native 别名的现有写者先完成，随后才在原数据事务内选择长度。等待后重新申请原 layout，已完成的预留和嵌套映射同步不重放；fallocate 续接不缩小外部写者已经扩展的文件。原公共入口与 FileWork 复用同一实现，减少原数字 FD 的重复查找。已补预留等待、关闭复用、位置保持及取消未消费额度的场景，尚未执行。
- 已落未验证候选：映射截断在最终 EOF 修改后只保留现有前缀并清零尾部，不再重新读取整页；等待期间新产生的存活前缀 dirty 标记保留，完整移出 EOF 的页继续失效并保留旧 byte pin 的物理租约。可选 SHM 升级跳过新 dirty 页，其失败等待也不混入原 resize。已补嵌套映射、布局重新准入、旧 FD、取消和后续同步真实新前缀的场景，尚未执行。
- 已落未验证候选：F_ADD_SEALS 与原写入／截断／分配使用同一个数据事务；F_SEAL_WRITE 保留原页写回计划，成功后再发布 seal 并唤醒新写者。查询仍使用原只读路径，现有 writable mapping 的 EBUSY 检查保留。已补 resize／seal 竞争和写回等待后的 seal 发布次序场景，尚未执行。
- 已落未验证候选：复制／splice 的普通文件读取及输出使用原 FileWork 作为中间请求，保留接受时的上下文、原 inode／OFD 和字节进度；等待源、动态 deadline、布局域及 owner checkpoint 逐层传递。外层等待时释放普通 layout admission，不以子写入的内部前缀完成整个复制。joined／inline 的 transfer 入口统一保留原工作，尚未构建或执行。
- 已落未验证候选：原输入 Vec 一次移动给中间写入，恢复时保留该缓冲区，完成后收回 scratch；转入等待后不重新读取被其他线程修改的源文件。forward self-sendfile 的尾段扩展只在原对象写事务准入后执行一次，并回传实际 offered／end，保留原短写及重复尾段算法。
- 已落未验证候选：外层 FileWork 持有一次原传输的退役租约，取消／完成会提取原管道前缀、位置记录和子工作，在传输状态锁外释放；保留旧请求 Arc 不会阻止退役。部分输出取消只消费已完成的管道前缀，真实短写仍交付其实际计数；已有 FUSE 请求需要等待取消终态时，沿用该请求的取消保留接口，不新增线程。
- 源码边界核对及候选修正：Linux 的 [sendfile／copy_file_range](https://raw.githubusercontent.com/torvalds/linux/master/fs/read_write.c) 和 [splice](https://raw.githubusercontent.com/torvalds/linux/master/fs/splice.c) 使用局部偏移，返回时写回共享 f_pos。候选传输保留局部偏移与实际输出 end，不新增整次输入／输出 OFD 位置租约；字段发布仍走原短对象锁，竞争的 seek／读写保留其原责任及可变获胜顺序。常规 read／write 的既有位置事务不因此移除。
- 已补未执行场景：复制等待期间修改源文件、关闭并复用两端 FD、显式／隐式偏移及并发 seek；外部仍持有旧请求时的取消；管道输出等待后的完整完成、真实短写和取消前缀只消费一次；forward self-sendfile 的重复尾段与真实输出 end。上述均为源码候选，没有增加历史 gVisor 通过计数。
- 调用入口源码核对：cgroup 文件本身属于 `file_target`，不在 namespace／epoll／BPF 等特殊描述符的 `nonbyte_target` 同步分支内；普通 Session 的文件系统调用先进入 DirectSyscalls 的保留请求入口。此核对不等于所有 legacy 调用都已覆盖：无 query 的复制／splice、隐式同步及 loader／installer 的阶段仍需补齐。
- 已落未验证候选：Wayland `wl_shm_pool.create_buffer` 在原 SharedFile 映射返回 ResourcePending 时，保留原文件、范围、几何、buffer ID 和传输上下文，通过原事件循环续接。仅暂停该客户端的请求解析，原 transport 保留已接收消息和 SCM_RIGHTS；后续 attach／commit／sync 和第二个 FD 不在映射前提前消费。其他客户端保持运行，成功后先发布原像素，再直接续接已缓冲消息，真实映射错误继续只断开原客户端。
- 已落未验证候选：vendored Wayland Rust backend 增加单客户端暂停／恢复，沿用原锁和 Linux epoll／Apple kqueue 编译隔离；暂停从原可读集合撤下，避免未消费输入使 poller 空转。当前产品使用 Rust backend；可选 libwayland C backend 显式报告不支持此扩展，不伪装完成暂停。挂起连接通过原生 peer 的关闭事件退役，原请求、未消费 FD 及订阅在既有 backend cleanup 中释放。
- 已落未验证候选：资源通知复用原 Wayland 线程、唤醒 socket 和 deadline；回调只发通知，不执行映射，不持有 guest task／mm 权限。原事件循环识别命令通道断开，最后 Server lease 在该循环退役时不因资源通知保留的 wake writer 而失去退出条件。没有新增生产线程或轮询总线，普通无挂起请求的路径不增加逐轮 poll 数组分配。
- 已补未执行场景：真实协议＋SCM_RIGHTS 下的两阶段映射等待、原动态 deadline、通知不执行映射、其他客户端继续完成、原 attach／commit 保留、后续 FD 只消费一次、peer 关闭取消和真实错误隔离。修改文件仅做 Rustfmt 解析和 diff 空白检查；本批仍未构建、运行测试或提交，也未增加 gVisor 通过计数。
- 已落未验证候选：NamespaceWork 保留类型化资源等待、多订阅和动态 deadline，沿用 FileWork 等待队列交还执行槽；数字 EAGAIN 不作为等待依据。chmod／chown／times 仅在实际资源等待时保留原 MetadataTarget，恢复时使用原 inode／mount／magic 引用；等待目标从短调用视图移入原工作，完成、取消或重验不污染下一次调用。正常入口不复制路径、不建立额外路径表，冷保存使用 Box，不放大普通文件 authority 的内联大小。
- 已落未验证候选：`security.capability` 的 NamespaceCall 保存原 node／filesystem／mount、已解析参数、对象事务及隐式 mapped DataSync。嵌套映射和层提供者保留已完成页／前缀，续接后使用跳过已完成同步的原 capability 核心；事务等待不持有 namespace、metadata 或字节锁。准入时重检 create／replace 竞争，可写映射的 EBUSY 保留，真实 checkpoint 错误沿用原属性／ctime 回滚和一次通知。同步页票据通知移出上述锁；持有原 capability 事务的内部等待不交给 caught-signal 重启整个调用。
- 已落未验证候选：PlainFile 的设置／撤销 capability 在原字节锁内重检同一个对象事务；原写者和写回继续使用已有 owner／writeback scope。MappedData 和 LayerFile 的同步后入口仍保留原 writable-map 检查及层 ID 转换，没有新增算法级锁或生产线程。
- 已补未执行场景：capability 等待中的 FD 关闭复用、路径改名／替换、一次属性通知、取消不改属性／时间，以及准入时竞争创建的 EEXIST；chown 撤销等待后仍修改原 inode，复用原 PathQueries 的下一操作使用新路径。既有嵌套映射部分页写回夹具加入 capability 隐式同步计划入口。仅完成修改文件的 Rustfmt 解析和 diff 空白检查，本批尚无构建／测试通过证据。
- 已落未验证候选：发布 IO controller 的层级启停及 `io.max`／`io.stat` 控制文件，任务迁移更新实时归属；FileWork、冻结 actor 与 mapped 同步保留接受请求时的 IO 上下文。worker 同时恢复内存和 IO 归属；原生无 guest 上下文显式恢复为空，避免继承无关任务。IO 资源停用后，已接受上下文归一到仍在线的祖先，旧控制句柄拒绝继续配置；通知移出层级锁。根提供统计，不提供限额文件。
- 已落未验证候选：私有磁盘数据的实际 read_at／write_at 在后端调用前取得额度，按实际完成字节和请求计数结算；缓存命中、tmpfs／管道及 HostFS 不冒充该私有磁盘设备的 IO。受限 DirectFile 在数据效果前回到保留请求的路径，并捕获检查后配置变更产生的类型化等待。元数据／manifest／capability sidecar 的原始后端写入及全部入口尚未纳入，不能据此宣布完整 IO 覆盖。
- 已补未执行场景：跨 worker／迁移的接受时归属、显式原生上下文、控制器停用后的祖先归属；私有磁盘真实读写与缓存命中计数、限额等待期间关闭并复用 FD、任务迁移、配置解除／控制器停用，以及通知回调重入层级访问。上述场景尚未运行。
- 已落未验证候选：删除 cgroup 后的 dying 计数及内部组额度同时跟踪原控制 inode、内存资源和 IO 设备资源；控制器反复停用／启用保留仍被旧请求或物理租约引用的旧代次。IO 记录使用设备资源的弱引用，覆盖已经丢弃 Group 句柄但仍在结算的 Permit；弱记录不自行保活资源。已删除子组尚存时，计数与清理保留其原来已删除的祖先。查询和既有创建入口计算存活关系，不增加回收线程或轮询源。
- 源码依据及边界：[cgroup v2 文档](https://docs.kernel.org/admin-guide/cgroup-v2.html)允许 dying 组继续持有受限资源；[Linux v6.12 memcontrol](https://raw.githubusercontent.com/torvalds/linux/v6.12/mm/memcontrol.c)对显式 active_memcg 保留原 CSS 引用，offline 不重置 memory.max。候选因此保留已接受内存上下文的原资源和原限额，不把它与 IO 队列停用时的祖先归一混为一谈。此为源码参照下的实现选择，仍需本批原生对照。
- 已补五个未执行生命周期场景：旧控制 FD 保留已删除父子组、旧物理页跨 memory 控制器重新启用及最后别名释放、已接受内存上下文在删除后仍受原限额约束、已发出 IO 的 Permit 保留已删除祖先，以及旧 IO 代次停用／替换后的取消与最后释放。另补了 IO 退役前已发出请求在退役后只结算一次的统计断言。修改文件仅做 Rustfmt 解析和 diff 空白检查，未构建或执行。
- exec 起点核对：旧 `ExecutableFile::bytes`／`Vfs::executable_data` 一次性收集完整 Vec，Session 的 `Request::load` 消费整份请求、DiskJob::Image 只交付最终 Result。以下候选接入了生产 Session 的主文件、shebang 和 PT_INTERP 读取；旧同步 API 及自动内存压力下的 ELF 映射阶段仍不据此关闭。
- 已落未验证候选：`ExecutableRead` 保留原文件、接受时的内存／IO 上下文、长度及已读前缀，每步最多一页；后续页限流不丢弃前缀，缓存被真实回收也不重读已完成部分。完整输入只发一次原 ACCESS 通知。执行租约由原 Session 请求持有；取消释放原读取、等待和执行租约，不在等待期间保留 inode／字节锁。
- 已落未验证候选：Session 请求保留脚本深度、原 exec 文件名／参数、当前执行文件、主 ELF 字节、原 PT_INTERP 文件及原凭据准备。PT_INTERP 复用原 ELF 头解析器，全部原输入读取完成后再进行段映射；动态加载器等待不重新读取主程序、重复推进脚本参数或重做主段映射。Runtime 原同步入口与预读入口复用同一最终 ELF 核心。
- 已落未验证候选：生命周期接受请求时从实际 CgroupTask 冻结内存和 IO 归属，并使用原层级锁与迁移事务保持一致；host child 使用接受时父任务的资源归属和子任务凭据。`Pending` 区分已完成结果和拥有原 Request 的资源等待，通知只唤醒源执行器；就绪后将原请求一次移回既有磁盘池。普通 resident exec 保留原内联执行，真正等待时才挂起；没有新增线程、轮询或把 task／mm 权限交给后端回调。
- 已落未验证候选：exec checkpoint 和 host launch 的多订阅保存动态资源 deadline，现有执行队列同时检查通知与到期；配置变更后重新读取当前 deadline。尚有 checkpoint 等待时不判定为终态空闲，custody deadline 只恢复原工作，不把到期误交付为系统调用结果。等待时交还既有执行槽与 task／mm custody，磁盘 worker 不停在那里等待额度。
- 已补五个未执行场景：实际磁盘 exec 读前缀跨 cache reclaim／迁移／FD 复用／路径替换；读取取消与 text lease 释放；脚本→主程序→PT_INTERP 的两阶段 IO 等待和原文件／参数／统计归属；完整请求取消后不再排队；无额外通知的动态 deadline 重新取得原 checkpoint。只完成修改文件的 Rustfmt 解析及 diff 空白检查，没有构建、测试、提交或增加通过计数。
- 已落未验证候选：VFS `HostImport<R>` 拥有原 reader、固定 64 KiB 缓冲、未发布 plain inode、实际写入偏移、EOF／同步／发布阶段和接受时的资源上下文。输入已交付的前缀与后端已写前缀分别保存，恢复不重读已经消耗的流，不重写已提交字节；取消丢弃匿名对象并归还原存储额度。新建与替换复用原 inode／namespace 提交核心，保留已同步的数据，返回实际发布 inode；新建提交重检同名竞争，父目录并发创建只复用真实目录，不覆盖竞争对象。
- 已落未验证候选：iPad FFI 实时无条件导入／替换接入拥有 reader 的 VFS 请求，在接受时冻结实际源 cgroup，上池后只打开一次原宿主文件；等待时将完整 HostCommand 交回既有 FileLayouts 的多订阅／deadline 队列。任务迁移或宿主路径替换不重定向已接受输入，原文件在导入完成前继续可读。该检查点当时未接入条件编辑、导出和 timezone；条件编辑及导出的后续候选见下方，timezone 等旧入口仍开放，不能泛化为全部 host IO 已覆盖。
- 已落未验证候选：传输取消和会话关闭通过原 console readiness 唤醒等待导入，取消无需等待 IO 额度。通知发生在 transfer／session registry 锁外；结束前先释放未发布暂存，再更新容量记录和原响应，关闭执行器也释放原暂存。沿用现有宿主任务／等待类型，没有增加线程、平台 API、Cargo 依赖或链接项。
- 已补六个未执行测试函数：跨 worker／迁移的完整流与新建／替换发布、部分暂存取消与额度退款、无效目标不消费输入、输入资源等待保留已交付前缀、实时导入保留原宿主 FD／原 IO 归属，以及传输／会话取消立即退役并保留旧文件。Runtime 和 FFI 修改文件仅完成 Rustfmt 解析与 diff 空白检查，尚未构建、测试或提交。
- 安装调用方核对：当前 iPad rootfs／SDK／Xwayland 安装在 `session.rs` 的 Runtime 创建／运行之前执行，没有给这些同步入口新增任意线程继承的 guest 归属。通用 `import_reader`／`stage_import` 及停止态编辑 helper 仍为同步 API；受控入口不能因为新增了 HostImport 就当作全部完成，后续按实际调用来源继续处理。另需核对 RAM backing 的 `Bytes::Memory` 与 guest file 页的边界，不能只凭 host Vec 驻留就推断归属或复用磁盘 cache 的通过证据。
- 已落未验证候选：VFS `HostExportWork<W>` 保留原 guest inode、页内读取缓冲、实际输出偏移及 EOF／flush 阶段；原 iPad FFI 导出请求只创建一次宿主输出文件，等待时释放 layout admission，复用原 FileLayouts／取消通知。失败清理前按 Unix dev／ino 核对输出名仍指向原句柄，外部宿主写者继续承担自身文件系统同步责任；已被外部改名的输出不通过猜测新路径清理。
- 已落未验证候选：导入与条件编辑复用同一 `StagedImport<R>`，保留已消费输入、已写入后缀、EOF 与已完成同步。条件编辑在暂存完成后取得原 provider 的对象事务，增量校验原 inode；等待不保留 node／byte／namespace 锁或布局准入。原生／CPU 映射保持原访问责任，实际映射／provider 版本变化后只重做校验，保留已同步暂存；不新增热路径版本计数。最终 namespace 提交重检原路径身份与同一版本，并复用原元数据、旧 OFD／硬链接和回滚核心。ENOSPC 下仍可识别已保存重试。
- 已落未验证候选：iPad FFI 条件编辑接入上述工作，原宿主源文件只打开一次并沿用原替换摘要核验；真实冲突保留编辑副本，传输／会话取消释放原暂存与对象事务，并在响应前更新容量快照。timezone、通用旧同步 API 和元数据 checkpoint 的实际后端限流／分阶段续接仍需处理，不能据此宣布全部 host IO 或完整存储提交已覆盖。
- 新增 11 个尚未执行的测试函数：Runtime 导出 2、条件编辑 4，FFI 实时导出 2、条件编辑 3。场景包括 cache reclaim／迁移／FD 与路径复用、部分输出及 flush 等待、两侧原文件句柄、分阶段暂存／校验、取消退款与对象事务释放、映射变化重校验、竞争替换，以及无暂存空间的已保存重试。修改文件仅完成 Rustfmt 解析和 diff 空白检查；整个 cgroup 批次仍未构建、测试或提交，没有增加 gVisor 通过计数。
- 普通读取起点源码核对：`PlainFile::read_bytes` 的缓存循环在后续页等待时丢弃局部输出 Vec，FileWork 的普通读取与 joined／inline 入口仅保存 wait 后重做原调用。下一步需让原请求持有已读前缀及原 FileRead 位置，保留完成／copyout 前的位置语义、取消和一次 ACCESS／atime；等待期间不新增整次数据读锁。新增游标还须避开自身 pending 位置造成的 readiness／host_work 自等待。legacy `DiskReadJob` 直接调用后端的入口也保留审计，不据此当作已受控。
- 已落未验证候选：普通 FileWork 读取、joined 及 inline 入口持有原 inode／OFD、FileRead 位置、固定原偏移与原输出 Vec。private cache 的读取直接向原 Vec 追加，后续页等待不丢弃完成前缀；mapped／layer／lower 递归保留新追加范围，映射覆盖字节在向外返回等待前写入该范围。等待释放 node／byte／layout 准入，普通数据读不取得新增对象事务，其他 OFD 和原 CPU／native 映射仍能修改字节。恢复只读取原缓冲后缀，真实错误／caught signal 返回已读前缀；取消丢弃未交付数据并释放原位置。
- 已落未验证候选：普通读取的 ACCESS／atime 在最终读取结果上处理一次，原隐式位置继续由 ReadReply／ReadPosition 保留到实际 guest copyout；后续页输出错误只发布实际复制字节，显式偏移不改共享位置。read cursor 避开自身 pending 位置的 waiting／host_work／订阅／执行前检查，没有新增线程或对普通 guest 字节访问增加同步策略。
- 已落未验证候选：private disk 的快速读取在跨页时只探测已驻留缓存；冷尾页在实际 backend IO 前回到 FileWork，因此配置在检查后收紧也不会丢弃前一页的已发出 IO。单页冷读与纯缓存命中仍沿用原快路径；HostFS 不套用私有磁盘缓存假设。该路径尚无性能通过证据，不能用源码变更代替最终测量。
- 新增 8 个尚未执行的测试函数：VFS 6、Linux 调用层 2，覆盖三种读取入口、显式／隐式及非页对齐偏移、迁移／cache reclaim／关闭复用／路径替换、并发写者、取消／信号、真实后端丢失、mapped 覆盖前缀、快速入口无 IO 回退，以及没有路径查询上下文的 joined 读取和 read／readv 后续真实 guest copyout 错误。仅完成 Rustfmt 解析和 diff 空白检查；本批仍未构建、测试或提交。
- legacy 调用方源码核对：Session `process.rs` 和 `process_lifecycle_dispatch.rs` 在普通文件 syscall 的 legacy body 前调用 `DirectSyscalls::start_owned_file`，可从实际 VFS 建立 FileWork；没有 PathQueries 参数不代表普通文件调用必然落入 errno-only body。其余未被接受的类型、公开同步 VFS 接口及 legacy DiskReadJob 仍需审计，不据此关闭所有旧入口。CLI 导出在 runtime.run 返回后执行，当前源码搜索未发现生产 Session 使用 legacy DiskReadJob；这不是受控同步接口已支持等待的证明。
- 异步入口起点核对：AIO 在 IOCB／原 FD 接受之后仍可能等待 aio_key／payload 导入，旧代码到创建 FileWork 才捕获资源；io_uring 的原执行上下文也可能在输入等待、迁移或其他观察者续行后才创建 FileWork。只在最终工作构造时恢复当前 worker 归属不足以表达原请求。另有不建立 execution actor 的快速／字节请求，不能仅在 actor 表中保存归属。
- 已落未验证候选：DescriptorFactory 可按 Runtime 的实际 TID，在原 cgroup 拓扑读事务内一次捕获内存和 IO；不使用 namespace 显示 PID，也不修改普通 CPU 访存路径。已知 source factory 中没有该实际任务时显式使用原生空上下文；没有 factory 的旧受控调用仍可捕获其显式调用作用域。此选择避免原生工作继承碰巧执行它的 guest worker 限额。
- 已落未验证候选：AIO IOCB 绑定原文件／eventfd 时保存原资源，aio_key 和 payload 继续导入时恢复该快照；Request 的等待、管道及最终 guest copyout 也恢复同一原归属。异步 fsync 的 actor 在该接受边界冻结并随原请求进入已有 owned executor，未再到晚期提交时重新捕获。已完成文件工作自身仍保留原进度和资源，不增加服务线程。
- 已落未验证候选：io_uring 在 enter 接受 SQE 批次时、取得 ring 元数据锁之前，捕获实际提交任务的成对资源，每份 UringRequest 持有原值；后续 metadata 重放和 selected-buffer 视图保留它。原 ring pump 在锁外的请求执行／copyout 阶段统一恢复，覆盖没有 actor 的快速／字节请求；有完整 source context 的普通文件请求进入同一可保留 FileIo 路径。原预算增加了新资源字段的大小，纯观察者 pump 不重新查询请求归属；没有新增线程、忙等或资源总线。
- 新增 5 个尚未执行的测试函数：AIO 3、io_uring 2。包括原 source factory 在没有 ambient guest scope 时捕获实际任务、payload 缺页后的迁移／FD 复用／换 worker、后续 IO 等待与最终输出的文件内存／IO 归属、另一观察者续行、取消，以及原生接受明确清除无关 guest 限额。修改文件仅完成 Rustfmt 解析与 diff 空白检查；没有构建、运行或提交，不增加历史 gVisor 通过数。上述候选不等于 setup／注册等控制输入入口或全部存储后端已完成审计。
- 控制输入起点核对：AIO 的 SetupInput／Lookup 和 io_uring 的 Setup／Registration／EnterInput 已能保留输入及实际 copyout cursor，但等待后恢复还会使用当前 worker 的资源归属。setup 的输出 COW、固定缓冲区与描述符环的物理 pin、GETEVENTS 的后续 timeout 页入均属于实际资源分配边界。
- 已落未验证候选：AIO 初次调用按实际 Runtime TID 捕获控制输入的内存／IO 归属；SetupInput 和 Lookup 在参数、原映射分配、实际结果写回及清理阶段恢复该值，原 context 的 User 继续持有同一次调用的归属。数据 IOCB 仍按各自绑定原 OFD 的接受时点取实际任务快照，不用整批控制输入快照替代。User 的对象级资源槽在逻辑完成／取消时释放；保留的旧 RetainedAio 快照不会因此继续钉住已删除的组，实际执行作用域及物理字节仍按自身生命周期释放。
- 已落未验证候选：io_uring setup 的两条生产 Session 调用方传入实际全局 TID，原兼容调用继续保留显式资源作用域；Setup 在输入及原 prepared FD 输出阶段恢复原值。注册请求在首次接受前保存资源，涵盖参数片段、buffer／descriptor-ring pin 和分开的实际输出；兼容 UringSnapshot 复用原 factory。GETEVENTS 的 descriptor／timeout／mask 在原 EnterInput 下恢复；SQE 批次还在原 ring 锁之前进入接受的资源作用域，避免初次 metadata 导入误计给 worker。等待仍交还原执行槽，没有新增线程或总线。
- setup／注册的可保留入口也识别已启用的 cgroup 内存归属，不只检查当前 mm 有没有异步文件 backing，避免纯匿名 pin／copyout 在随后启用自动压力等待时误走一次性接口。其他 ABI／SQE 的同类能力判断仍随自动压力等待实施一并复核；此处没有启用自动内存准入等待。
- 新增 6 个尚未执行的测试函数：AIO 3、io_uring 3，覆盖 setup 参数跨页等待后的真实 COW 输出、明确原生无归属、事件 copyout 及取消释放消费权、固定 buffer／descriptor-ring 注册在输入等待后迁移并保持实际 pin 到 unregister、GETEVENTS 等待后从私有磁盘读取 timeout 页，以及保留旧 AIO 快照时完成／取消对已删除资源组的逻辑退役。修改文件仅完成 Rustfmt 解析与 diff 空白检查，整个 cgroup 批次未构建、未运行测试、未提交／推送，不增加 gVisor 计数；以上源代码和场景均须在整批集中验证中确认。
- 已落未验证候选：有 query 的 Vfs::splice 对普通磁盘／管道传输也保留整个 FileWork，不再只有 daemon-backed 路径保留外层请求；子读取 IO 等待不析构外层偏移、管道预约及读写 cursor。新增 1 个未执行测试函数，覆盖普通磁盘到管道的后续页 IO 等待、迁移、原 FD／路径复用、成功交付与取消释放；修改文件仅完成 Rustfmt 解析和 diff 空白检查。这不关闭没有 query 的旧同步入口。
- 元数据存储起点核对：DiskStore::commit 的 manifest 和 DiskObject::set_caps 的 capability sidecar 直接调用 native write_all，先前未经过私有虚拟设备 IO 预算。普通 user／ACL xattr 的 manifest 失败已有原属性／mode／时间回滚；但 CapabilitiesWork 只保存 mapped sync，sidecar 发布后的 manifest 等待会重新套用 XATTR_CREATE／REPLACE，真实错误后的 sidecar 恢复也不能作为一次忽略返回值的等待丢弃。
- 已落未验证候选：私有 manifest／sidecar 按完整原记录在临时文件创建、数据同步和实际写入之前申请 IO；没有额度时通过原请求的类型化 ResourcePending 返回，没有临时文件或已发行 permit。原生短写在同一次已准入的虚拟设备记录内继续，真实错误保留已写字节统计，未发行的 permit 在打开／同步失败时退款；文件 sync／rename 与目录持久性仍沿用原实现。这里统计虚拟设备的原记录请求及其实际完成字节，不声称等同于宿主块设备的物理 BIO 次数。
- 已落未验证候选：CapabilitiesWork 保存原 mapped sync、sidecar 是否已经发布、原 capability／ctime、实际发布时间和原 checkpoint 错误。内部 manifest 等待不回滚／重复发布已完成的 sidecar；恢复不重新套用 CREATE／REPLACE 或写入 ctime。真实 checkpoint 失败仍按原语义最佳努力恢复旧 capability，恢复的写入若受限也保留原终态错误等待；ctime 只在仍是该次发布的值时恢复，避免覆盖等待期间的其他元数据变化。取消在 sidecar 发布前无属性效果；发布后的取消保留已经发生的效果，析构不追加磁盘 IO，不把它记作调用成功。
- 新增 7 个尚未执行的测试函数：原生短写／失败前缀统计 1，manifest／sidecar 场景 5，导入 publication 等待 1。覆盖 manifest 开始前的等待／原 inode 与 FD 复用、取消退款、真实打开失败退还未发行额度并保留外部临时文件、已发布 sidecar 后的完成／取消与重开、发布前取消、manifest 已写后发生真实 rename 错误并等待恢复旧 capability，以及完整暂存后的 create／replace 不重复消费输入或数据 IO。原导入和条件编辑的统计预期增加实际 manifest 写入；没有运行这些测试。修改文件仅完成 Rustfmt 解析与 diff 空白检查，本批仍未构建、测试、提交／推送。
- 本次后端接入仍不能关闭所有元数据调用方：overlay copy-up 的局部读取／写入进度、ProjectionEdits 等原回滚／析构中的 sidecar 恢复、无 query 的旧同步 API 及初始元数据加载来源继续审计；新产生的类型化等待必须保留到原请求，不能用忽略返回值、重复执行整个 copy-up 或绕过 IO 限额完成回滚。自动内存准入等待仍未启用。
- copy-up 起点核对：原 Overlay::copy_up 在 inode 借用内一次读取／写入整个文件，后续页 IO 等待会丢弃局部 Vec／偏移和已创建的未发布上层文件；PathQueries 的 open 路径也未将此等待转为保留原 inode／FD reservation 的工作。此前资源等待在 run 起点释放，若等待持有冷续执行状态，还会在真正重入资源前丢失它；不能直接保留已就绪等待而让它遮住新的 OFD／命名空间等待或持续触发已就绪订阅。
- 已落未验证候选：每个正在 copy-up 的 inode 使用一个弱索引和对象级进度 mutex，原请求的 PageWait 持有实际工作；没有常驻所有者、服务线程或轮询。工作保存首次内存／IO 归属、未发布 provider、读取前缀、分块／部分写入偏移、稀疏源 mapped sync、capability 与最终 sync 阶段，迁移／换 worker 后只推进未完成部分。读取使用原 read_into 前缀语义，写入单独保留实际已完成前缀；源节点和数据／上层命名空间锁不跨资源等待。完成后释放原工作上下文；最后等待者取消时退还未发布存储并移除弱索引，不逐次扫描全部 copy-up 条目。
- 已落未验证候选：普通 open 到达 overlay 资源等待后，FileWork 持有已接受的原 inode／mount、原 flags、未发布 FD 槽和 writer use；不重新读取 guest 参数或以新 pathname 替代原对象。原权限和 Landlock 检查已在接受前完成，冷恢复凭证保留当时的 denied mask；原 open 核心的已检查阶段不因等待期间 chmod 再次撤销已接受的访问，新 open 仍执行全部原检查。后续等待继续携带相同 reservation，O_TRUNC 仍进入现有 provider／layout 路径。同步检查同时发现先前 resize cursor 完成时跳过原 PreparedOpen::publish，已与普通完成路径复用该发布步骤，错误仍丢弃原隐藏 FD。
- 已落未验证候选：FileWork／NamespaceWork 保留已就绪的资源凭证到真正操作恢复；ready／waiting／订阅不让旧凭证遮蔽其他等待。HostImport／HostEdit 的同类旧等待也保留到本次 run 结束。mapped sync 的 copy-up 通知可在进度 mutex 和节点／数据锁释放后退休；普通 sync 的原 notify 路径保持原递归释放方式，没有为它额外合并分配。
- 新增 7 个尚未执行的测试函数：copy-up／open 6、资源凭证 1。场景覆盖下层原文件改名／FD 复用、迁移／换 worker／缓存回收后的已读前缀、两块数据的写入等待与取消退款、已 copy-up 后的 merged 路径替换及 O_TRUNC 原 FD 发布、首等待者取消后另一个已接受调用继续同一份数据工作、data／capability／manifest 分次获得 IO 额度不重写已完成阶段、非特权 open 接受后权限变化不撤销原授权且下一次 open 拒绝，以及已就绪凭证不能遮住新位置等待或触发忙重试。上述源代码只完成 Rustfmt 解析和 diff 空白检查；本领域仍未构建、运行测试、提交／推送，不增加 gVisor 通过数。
- 前一剩余边界：ProjectionEdits 的 sidecar 发布／回滚与其析构中的恢复尚未完成分阶段保留。该项已由下述 2026-11-27 候选继续处理，仍无构建或运行证据；公开无 query 的一次性同步 API、ELF 映射／初始栈的内存等待、普通 dirty 写回后真实回收、自动准入重试／OOM、限额收紧及其余退役生命周期继续开放。
- 投影起点核对（2026-11-27）：旧 ProjectionEdits 在临时 raw inode 上直接写入转换后的 capability；manifest 等额度时，析构又尝试写回旧 capability 并忽略等待。重复 checkpoint 因而可能不断重写相同 sidecar，既不能可靠回滚，也可能在低 IOPS 下持续耗尽下一阶段的额度。现有挂载范围允许嵌套 lower，overlay 作为 upper 仍明确拒绝；本轮不改变该支持范围。
- 已落未验证候选：投影 ID 中间值只在当前 checkpoint 的作用域内可见，不改其他线程所见的真实 provider capability。只为最终物理 provider 的目标值写不可变暂存记录；provider 保留弱索引，原 PageWait 持有已完成暂存，后续预算波次复用原字节。取消只清理未发布暂存，不在 ProjectionEdits 析构中重写 sidecar。初次设置／copy-up 仍负责 mapped 写回；投影只转换已有授权的 ID 域，并保留最终发布前的原生数据同步屏障。
- 已落未验证候选：manifest 额度先于最终发布取得；全部受影响 provider 先按稳定顺序完成原事务准入，再以原字节锁覆盖已准入的原生 sidecar／manifest 提交。任何准入等待均释放已取得的事务／字节锁，锁不跨资源等待；没有新协调线程或轮询。真实提交错误用原 sidecar 的文件链接／重命名恢复，不需要第二次数据写入额度；恢复的原生失败明确返回错误，不假装成功。capability 输入保存原实际值，暂存期间发生的真实撤销会在发布前触发重新取当前状态，不能恢复旧授权。
- 已落未验证候选：checkpoint、投影 O_TMPFILE 和 unlinked metadata 共用上述提交边界；inode 登记失败也进入原路径／链接／投影恢复分支。私有存储重开识别并清理固定宽度、仅小写十六进制的自有 .new／.old／.pub 暂存名，所有目录项验证后才删除垃圾，陌生条目仍拒绝。持久性仍为原有 sidecar 先于 manifest 的顺序，不新增跨两种记录的崩溃原子性承诺。
- 新增 8 个尚未执行的测试函数：两项投影作用域／输入撤销场景、六项磁盘挂载／恢复场景，覆盖 idmapped upper 与嵌套 idmapped lower、完成／取消／迁移／换 worker、多 inode 的分次 IOPS、manifest 已写后的真实 rename 错误且回滚不再等待额度、原 provider 的真实写入撤销、后续 provider 忙时释放先前事务，以及重开清理自有暂存、陌生条目拒绝且验证前不删除。仅完成选定源码 Rustfmt 解析和 diff 空白检查；整个 cgroup 批次仍未构建、运行测试、提交／推送，不增加 gVisor 计数。
- 下一实现输入：继续处理无 query 的 legacy errno-only 入口、受控同步导入／旧 executable API，以及一般 xattr／overlay copy-up／存储 checkpoint 的后端阶段与等待边界，并核对其余接受／退役边界。源码核对确认 transfer_io 的子工作已保留到自身 Child，但没有 query 的 Vfs::splice／FileCopy::copy_prepared 仍可能让临时外层在资源等待时退化为 errno 并析构；不能据上述异步控制输入候选关闭这些公开同步入口。ELF 段映射／初始栈等内存分配尚需在自动内存压力等待启用前复核，不把 IO 输入候选等同于自动内存等待闭环。无可保留请求的调用必须在实际效果前进入可续接路径，不能让已部分输出的工作退化成普通 errno。随后将普通映射 dirty 写回和旧快照维护接入 cgroup 压力，写回后实际回收再决定 OOM；自动内存限额等待尚未启用。非阻塞内存配置写入、配置收紧后的 OOM／通知及其余退役边界仍需补齐。04 保持实现中，先补完整批再统一验证。

## 后续实现需要保留的事务边界

### 2026-11-27 重启前断点（所有者要求暂停）

- 当前工作全部保存在工作区，尚未提交；本次恢复后仍只实现 04 cgroup，没有启动构建或测试。上面的领域验证状态不变。
- 最新未验证源码：新增 VFS `host_sync::Wait`，通用同步 import／replace import／export 与 `ExecutableFile::bytes` 复用现有 HostImport／HostExportWork／ExecutableRead 的进度；只在宿主原调用线程使用已有 HostWait，不创建服务线程。生产 guest 的受支持 copy／splice 已核对会先由 DirectSyscalls 接受为 FileWork；无 query 的公开 transfer API 仍待接齐，不能宣布收口。
- 最新未验证源码：新增 `trames_elf::Loader`，主 ELF／PT_INTERP 按段和页保留映射、写入及权限阶段；新增 `image::InitialStack`、`vdso::Install` 与 Session `InitialEntry`。原 exec Request 已保留输入完成后的 Loader、LoadedElf、vDSO／初始栈及原凭据状态，等待后不重新读取文件或重建已完成映射。选定文件只经过 Rustfmt 解析；新增 ELF 对 trames-shared 的依赖尚需在集中构建时更新／核对锁文件。
- 刚开始、尚未接齐：`trames-memory/src/cgroup_memory/reclaim.rs` 将干净映射旧 pin 的退休排到已有 MemoryReport／proc-read 原 mm checkpoint；Report、Account、Charge、PageBacking 增加对应候选入口。这个子步骤尚未完成：需要补 PhysicalCharge 的 reclaimer 登记方法、核对 snapshot 字段访问边界与 provider 清理后的归属判断，再实现普通 mapped provider 的选择性写回／干净 canonical bytes 丢弃。当前仍不能据此声称实际压力回收已闭环。
- 自动内存准入等待仍未启用；MemoryCgroup::take 仍走原 clean reclaim／retry／OOM。下一步还需实现有界 writeback → 原 mm pin 退休 → 实际额度重试，完善 memory.reclaim／限额收紧／通知，复核所有 async-fault 能力判断；不得先开启等待再让未接齐入口吞掉 Pending。
- 其余未完：FFI 停止状态下的旧 transfer／条件编辑回退、旧同步校验／安装器入口，以及公开无 query 的 copy／splice。现有带外部 LayoutExclusive 和 FnOnce 流式校验的旧 HostEdit::replace 不能简单在其内部睡眠，应迁移到已有可保留 HostEditWork，并在等待前交还 layout。
- 按原要求，本领域必要调用方和回退完整实现后再统一验证，随后提交推送 main 并同步 iPad／Mac Runtime pin。当前不关闭领域，不增加 gVisor 计数；没有运行中的本轮构建／测试任务需要跨重启接续。

- tmpfs 的大范围 unwritten allocation 不能退化为逐页 host Box 预分配，也不能只沿用文件容量计数。应由具名零页／extent 资源持有首次 cgroup 归属和限额租约，实际 materialize 的页继承对应 slice；截断先退役 extent，仍有实际字节 pin 的 slice 最后释放时再还额度。压缩零内容与 guest 物理页额度是两个层次，不能让迁移后的写者重付已经分配的页。失败须同时回滚新增 extent 与新租约。
- extent 的 materialized slice 需要对象内的 pin 计数／状态，不能只观察 Weak::strong_count 决定退款：最后 Arc 的析构可能尚未拿到同一资源锁，单看 strong_count 会产生重复退款窗口。
- IO 限流必须发生在真实 backend 请求之前。冷读、page-in、同步写回、copy/splice、AIO/io_uring、exec 的继续执行都要保留原资源和已完成进度；内部等待不能作为 guest EAGAIN 或 SIGBUS 交付。不能靠重跑整个 sync 把已经写完的页重新标脏并重复消费额度。
- `memory.high/max` 压力下的 dirty 页需要复用既有 SyncExecutor 的写回续接；写回完成后才尝试释放实际字节。文件锁、布局 admission、task/mm custody 和 CPU 槽均不能跨阻塞等待保留。
- SharedFile 的 memory-reclaim 等待也必须保留已写入前缀，不能把内部等待变成一次短写／EAGAIN。需要沿用原文件的异步写事务和位置等待，把整个原请求接续完；Append／元数据变更与原对象准入一并处理。
- 文件等待的错误桥应以每次原请求作用域持有类型化的 wait／progress，作用域结束后移入原 FileWork／PageSource／SyncResource，不能遗留一个可被别的任务误消费的无作用域 TLS 槽。普通短调用需在效果前转入原工作路径；错误码相同不足以证明它是内部等待。
- 原写事务需要覆盖共享 OFD 的 f_pos 与实际数据对象的 append／截断顺序；不得跨等待持有 Node／PlainFile 字节锁或布局租约。现有 active_file_work 能让直接路径回退，但单次在锁外观察该标记不能阻止已等待底层字节锁的旧快速写者越过事务。事务的重新检查必须在原数据锁准入内；不同 inode／数据对象不应共用锁。既有后台页写回仍需能推进，不能被等待回收的前台写事务反向堵住。
- 接通 FileWork 时需保留原写缓冲区及 cursor，后续阶段借用剩余切片，不能移除 Vec 前缀形成反复拷贝。共享位置在首字节之前建立原 pending lease；append 的原起点在对象租约内选定，首个实际前缀使用原 append 算法并回传实际 end，保留 HostFS 外部写者的原生原子追加；已有进度后不重新选择 EOF。`PathQueries::file_command`／`FileCommand::execute_target` 可使用显式进度参数调用原写入核心；`file_write_description_inner` 现有三处调用需保留默认行为，续执行单独传固定偏移、禁用重新 append 及延后同步策略。
- 分段数据完成后释放对象写事务，再推进原 O_SYNC／O_DSYNC／WriteSync 阶段；共享 f_pos 仍保留到原结果交付。Linux 的 [generic_file_write_iter](https://github.com/torvalds/linux/blob/master/mm/filemap.c) 在 inode_unlock 之后调用 generic_write_sync，而 generic_file_read_iter／filemap_read 没有整次读取的 inode 写锁；因此不新增整次读请求的租约，也不把读者冻结到整个前台写入／同步结束。原页／字节锁与原生函数的同步责任保留，此源码核对不替代最终参考内核验收。
- 写回资源保留已完成页／阶段，不能重复跑会重新标脏的 flush_range；成功位置、同步失败时的位置回滚、信号／退出时已写前缀的交付均需沿用原语义。FileWork 在资源等待时释放普通文件的 layout_exclusive，不能只复用目前的 FUSE 释放分支。
- private COW 失效区分“原文件被截断导致 PTE 无效”和普通缓存物理页换代。候选在原 Backed 上记录 truncate generation；joined prepare／reclaim 时退役旧 private PTE 并重新挂回原 cache，直接读取也不得在 truncate→grow 后暴露旧 COW 内容。旧 byte pin 仍保留旧物理租约；普通文件写入或 clean cache 回收必须保留已有私有副本。回归覆盖普通文件／tmpfs／memfd、fork、部分／完整页截断及重复输出 preflight，尚待本领域集中验证。
- IO ABI 的字段、零值错误、最大值与 IOPS clamp 对照 [Linux 控制器实现](https://github.com/torvalds/linux/blob/master/block/blk-throttle.c)，用户可见范围对照 [cgroup v2 文档](https://docs.kernel.org/admin-guide/cgroup-v2.html)。本批尚无通过证据，以上链接也不替代原生对照和真实负载验收。

### 2026-11-27 恢复后的候选与验证起点

- 普通私有磁盘映射通过原 SyncExecutor、原 OFD 和原 layout admission 选择性写回；FUSE 压力写回在 daemon 回复后回到原 layout，再丢弃可重建的干净 canonical bytes。不会丢弃匿名、shmem、未写回 dirty 或私有 COW。
- 原 MemoryReport 的既有 checkpoint 接收弱索引的 pin 退休请求，不保留 Memory/CPU View。取消、原 mm 退役和已尝试的映射代次均有终态；未变化代次不会不断重新排队。外部实际 byte pin 仍持有原物理租约；pgsteal 只在最后一个被回收的物理租约释放时记入首次归属及祖先。
- 自动 memory.max/high 压力准入已接入干净回收、原资源写回、原 mm pin 退休与实际额度重试；max 仍失败才报告 OOM。Memory/View 的 async-fault 能力判断覆盖活动 cgroup，保留现有系统调用输入／输出／exec 的续执行入口。
- memory.max/high/reclaim 写入保留原控制文件、参数、已应用配置、共享 f_pos 事务和阶段；等待不持有 topology/position/layout 锁。O_NONBLOCK 配置只更新限额；其他写者的新限额可以唤醒并终止旧配置的同步回收，不重放旧值。收紧 max 的组内 OOM 等待原受害者退出，保护 oom_score_adj=-1000；memory.events/local 的层级事件通过原 poll/inotify 资源通知，不借用控制 inode。
- 宿主同步读写、copy/splice、sync、open/resize、元数据和安装器入口改用原 FileWork/NamespaceWork/HostImport/HostExportWork 的状态；内部资源等待不丢弃局部进度。只在宿主原调用线程使用已有 HostWait，guest 的生产可保留路径交还执行槽。生产 FFI transfer 的死旧实现已移除；完整 transfer 使用前述 live 请求阶段。没有 originating Linux TID 的宿主配置／诊断命令显式恢复 native 资源，避免继承文件 worker 上其他 guest 的作用域。
- ELF 主映像/PT_INTERP、初始栈和 vDSO 的分段映射／页写入已接齐原 exec 请求；熵、地址及完成前缀保留。新增测试覆盖这几段的压力与取消、普通/FUSE 映射的真实 pin、控制写入取消／换 FD／限额更新，以及原 mm 请求取消／退役；均待集中执行。
- 已进入集中编译，日志位于 `target/validation-cgroup-20261127/`；Linux 日志位于 `target/local-ci/io/cgroup-20261127/`。重启后的临时 runner 已重建，每阶段具有限时、16 秒进度输出和 1.5 GiB 磁盘余量下限。Linux VM 已启动，复用原构建缓存；冻结源码排除其他会话的测试 fixture。
- 当前编译正在成批修复：回收 Account 初始化、Apple kevent 输出类型、原 OFD mount 接口、进度方法可见性、磁盘文件借用类型及测试构造等。尚未运行本批功能测试；历史 gVisor 计数不变。接下来统一执行本领域测试并汇总修复，再做真实负载／性能和交付检查，按 main 配对提交推送。
