# 临时实施清单：兼容性迭代后的剩余工作

日期：2026-11-23；当前状态核对：2026-11-24。用途：作为后续实现、定位和验收的工作队列；完成一项后在本页更新状态并链接验证记录。这里保留待办和完成标准，具体设计在对应批次开始时结合源码和目标负载确定。

当前共 12 项：01 已完成；09 的共享实现和本地行为验证已完成，仍需实际产品／真机交付验收；02—08、10—12 仍有实现、工具验收或测量工作。已合入实现及其通过证据不能自动关闭整项。

## 当前剩余工作摘要

已发布修复基线：Runtime `42e5834`，iPad `3d966002`，Mac `2a99558`。Runtime 的 Linux／macOS CI 均已通过。核对时 iPad 产品 CI 在 Ubuntu 的 iOS 交叉检查中失败：`trames-graphics` 编译 `native/image.mm` 需要 `xcrun`，当前宿主没有该工具。修正 Apple 原生构建的宿主／SDK 检查安排是下一轮交付阻塞；不能将这次失败当作新的 guest syscall 差异。Mac 产品构建随后也已通过。

| 项目 | 已有实现／证据 | 仍需迭代 |
| --- | --- | --- |
| 02 FUSE | 目录、路径续行、命名空间、元数据、截断／追加／同步写、xattr、异步页来源及原 OFD 结果续接已合入；已覆盖回归通过 | mmap／共享写回、完整缓存与通知、剩余权限／协议边界、宿主 ACL／capability 映射及 O_PATH symlink xattr；真实 FUSE 程序、吞吐及两端验收 |
| 03 现代挂载 | nosymfollow、传播属性、tmpfs fspick／重配置已有实现；递归属性及挂载查询已有真实工具证据 | ID-mapped mount 的 UID／GID／权限／创建依赖，其余文件系统重配置，真实 runc／OCI 启动验收 |
| 04 cgroup | 映射 cache／别名及匿名 fork COW 的物理页归属、计费和写准入已有实现及回归 | 非 mmap 文件 IO 的 cache 计费、物理回收及 dirty／pinned 页、native extent 动态计费，真正的 IO controller 和压力验收 |
| 05 Android 内存 | 已确认仓库镜像的 API 33／arm64-v8a 声明；既有 cgroup 基础可复用 | 确认 ART collector／lmkd reaper 实际路径，实施 userfaultfd／process_mrelease，验证 GC 尾延迟及退出回收 |
| 06 调试工具 | ptrace／proc memory 原版用例闭环；GDB 启动、软件断点、单步及寄存器修改已有证据 | live attach、多线程、gdbserver、strace -f、拒绝访问及异常退出会话；硬件断点／观察点、SVE／SME 单独列支持范围 |
| 07 futex2／rseq | 四个 futex2 接口及 rseq 注册／中止／迁移／信号／fork／exec 已有实现；已覆盖单元／解释器检查通过 | 原版 no-libc ARM64 runfile、Linux 自测和真实调用程序，两种执行后端的生命周期及旧 futex 性能验收 |
| 08 perf | 软件时钟计数／基本采样、FD／控制／映射环、输出重定向、继承／exec 和部分动态策略已有实现及回归 | 采样中断／开销预算、内核生产者、其余事件与 CPU／cgroup 作用域、映射／权限细节，原版 perf stat／record 验收 |
| 09 动态任务上限 | 默认 65,536、可写 proc、manifest／CLI／FFI／两端启动共享准入资源；实际 guest 初始化、动态读写及拒绝 fork 已验证 | 实际 Mac/HVF 产品运行及物理 iPad、安装应用的配置交付验收；共享实现不再按未实现排队 |
| 10 allocator | 默认关闭的实验路径；固定输入和轮换对照驱动为 WIP | 完成短调用／竞争及 Vim／Python 等真实负载对照，定位既有倒退后再决定默认开关 |
| 11 存储持久化 | 同步写结果／偏移问题已修；ACK、P99、崩溃重开测量驱动为 WIP | 等持久性单／多写者、包安装／容器层、写放大及恢复验收；依据测量选择 WAL 或其他布局 |
| 12 空闲内存 | FFI 进程快照入口已合入；宿主采样及 iPad 场景仍有 WIP | 固定配置的启动峰值／稳定空闲／停止释放，区分 RSS／physical footprint，干净启动和物理 iPad 基线 |

已关闭的旧问题不再作为待修项：UserChainTargetCrash、旧 ptrace／proc memory／PIDFD_THREAD 缺口、批量挂起原因、原 22 个稳定失败、共享 OFD 偏移竞争、threads-max worker 读回问题，以及后续 Linux 网络节点栈溢出和进程测试额度／完成条件问题。具体证据见[候选修复记录](../validation/candidate-batch-20261124.md)。

固定 gVisor 集合的最近一次全量及复核仍为两端各 14,014 通过，其中 13,965／13,965 个 Linux 原生通过项全部对齐。当前修复只复查受影响集合；163 个通过项与全量台账重合，不累加成新用例。当前提交的完整 254 组、实际 HVF／物理 iPad 及真实程序验收仍需在相应集成／交付节点收口。

推进依赖顺序仍为 FUSE → 现代挂载 → cgroup／Android 内存；调试／同步／perf 的已知缺口与调用方一并登记。每轮先完成全部已知实施问题及依赖，再集中验证；纯工具／测量项先备齐场景，不逐个 bug 展开一轮矩阵。全程不新增协调／服务线程或轮询消息总线。

## 历史实施与验证记录

以下阶段记录保留形成时的“候选／未执行／未交付”描述；现状以本页摘要及其链接的后续验证记录为准。

2026-11-24 恢复后的实现阶段统一记录在[当前实现批次](implementation-batch-20261124.md)。FUSE 元数据／回复边界、截断／追加／同步写、xattr 写事务与写前权限清理、异步页等待及文件／AIO／io_uring 原结果输出续接、部分现代挂载、映射页及匿名 fork COW 的 cgroup 归属、futex2／rseq、动态任务上限，以及 perf 计数／基本采样环／输出重定向／继承／exec／部分动态策略已有新增候选，真实调试会话及 allocator／存储／内存测量入口已部分准备。完整清单中的缓存／mmap、ID-mapped mount、回收／IO 控制、Android 内存、perf 生产者／完整作用域／映射策略等依赖仍未完成。所有者随后要求先验证现有候选，当前已进入集中验证检查点；以下历史通过结果不覆盖新增修改。

实现约束：整个清单不额外引入协调线程、资源服务线程或轮询消息总线；复用现有执行器和 readiness，阻塞等待交还执行槽。

现有候选的集中检查点已完成，见[2026-11-24 验证记录](../validation/candidate-batch-20261124.md)：解释器／FFI 533 项通过，受影响原版 gVisor 覆盖全部 162 个 Linux 参考通过项；内部仍有 22 项稳定失败、共享 OFD 偏移间歇失败、threads-max 端到端失败及静态检查阻塞。下一实现批次统一处理这些跨领域问题及其依赖，当前不勾选整批完成。

上述当前失败批次现已修复并集中复验：受影响四库 1760 项、产品 533 项通过；共享 OFD 偏移连续 20 次通过，任务上限具备实际 guest 前后反例，严格 Clippy 和受影响原版 gVisor 对照通过。具体修正及未覆盖门槛见同一验证记录，12 项总目标仍未完成。

性能记录更正（2026-11-24）：下文历史长探针的纳秒换算存在跨秒错误，其原百分比不再作为定量性能结论；旧记录保留，后续以修正探针的结果为准。短探针与功能测试不受影响，详见[FUSE 验证记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#performance-timing-correction-2026-11-24)。

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

- [ ] 状态：进行中；目录、传输协议、命名空间及数据续接已合入并通过已覆盖验证，完整 mmap／写回和整批程序验收未完成。
- 本批范围：目录枚举、创建／删除、rename、通用符号链接遍历，以及通过 FUSE 文件句柄的文件映射。
- 已交付版本的数据请求上限为 4 KiB；候选依据 INIT 的 `max_write` 协商请求大小，并设置 128 KiB 上限，已验证 16 KiB 请求及短传输。继续验证并发、取消、关闭和页生命周期。
- 完成标准：上述操作与 guest daemon 正确交互，覆盖失败、并发及卸载／退出；选定真实 FUSE 程序做端到端验收，并测量实际文件传输吞吐。SSHFS、`rclone mount` 或 Android 存储路径须分别有证据后再声明支持。
- 依据：[FUSE 文件操作记录](../../runtime/docs/validation/gvisor-fuse-files-20261121.md)。
- 2026-11-23 本地候选进展：目录根和后代支持 OPENDIR／READDIR／RELEASEDIR／FSYNCDIR；目录 cookie 在 guest 拷贝后逐条提交，短缓冲区、部分 EFAULT、dup 共享偏移、FD 复用及迟到／取消的句柄清理已有定向回归。mkdir、mknod、symlink、unlink/rmdir、rename 和原子 CREATE 候选已接入异步请求，包含 CREATE 的 MKNOD＋OPEN 回退、rename2 的 ENOSYS 处理、sticky 权限、错误路径及 mount writer 生命周期。
- 阶段证据：扩展后的同一 guest daemon/client ELF 在原生 Linux及解释器 1／2／4 CPU 通过；27 项 FUSE 和 18 项 Linux 文件路径定向测试通过。原始 gVisor FUSE 组为 22 通过、2 个既有夹具跳过。产物位于 `target/fuse-complete-20261123`。候选源码、fixture 与阶段记录仍在本地工作区，尚未包含在已交付 Runtime `47b7435` 中；阶段记录路径为 `runtime/docs/validation/fuse-complete-progress-20261123.md`。
- 下一步：补齐其余路径调用方、挂载 writer／权限及错误顺序审计，并接入异步 mmap；之后完成真实程序、吞吐、正常负载性能与两端整批验收。现有阶段证据随本批一并验收后才能关闭整项。
- 目录身份进展：候选已将目录 FD／cwd 接入 VFS inode 与共享 fs_struct，验证相对查找、chdir/fchdir、重命名后的 cwd、删除 cwd 后返回父目录，以及 FD 复用和权限失败；同时修正冷路径缓存延长 FUSE 连接寿命、导致卸载后 daemon 无法退出的问题。扩展后的同一 ELF 在原生 Linux和解释器 1／2／4 CPU 通过；33 项 FUSE、133 项相关路径／挂载／proc 测试通过（选集有重叠）。保留原始失败及修复后证据。仍需通用符号链接／跨挂载遍历及其余目录 FD 操作，不据此关闭本项。整个候选复用现有 worker、对象同步和 readiness，不额外引入协调或资源服务线程。
- 路径续行进展：metadata/open/access/chdir 已保留同一次 VFS 遍历状态跨 daemon 等待，验证符号链接、`.`／`..`、跨普通文件系统及 FUSE 连接、openat2 作用域、原 FD 预留与取消清理，以及 FUSE 子目录的 open_tree/move_mount 绑定和挂载边界。最终同一 ELF 在原生 Linux及解释器 1／2／4 CPU 通过；42 项 FUSE 和 18 项 Linux 文件路径测试通过。仍需将创建／删除／rename 的父目录遍历和其余路径调用方接入，补共享 fs_struct／挂载属性采样、mmap、真实程序和最终验收。详细证据继续记录在本地阶段文档，不勾选整项完成。

- 父目录续行进展：创建／删除／rename 已移除 FUSE 私有父目录遍历，复用公共 PathWalk；等待期间保留原目录身份，解析后再判断实际挂载、只读准入和 EXDEV。新增父目录符号链接、挂载遮盖、转到普通存储及共享 umask 变化的对照；同一 ELF 在原生 Linux和解释器 1／2／4 CPU 通过。760 项相关 VFS（含 48 项 FUSE）和 18 项 Linux 文件路径测试通过，原始 FUSE 组仍为 22 通过、2 夹具跳过。仍需其他路径调用方、atomic CREATE 的 umask 阶段、动态挂载属性、错误顺序、mmap 和整批验收；不勾选完成。

- open/create 状态进展：现存末级符号链接的 O_CREAT|O_EXCL 已按 Linux 返回 EEXIST；access 在 LOOKUP 等待后读取当前只读／noexec 属性；CREATE 与 ENOSYS→MKNOD 回退重新采样共享 umask，并保留原父目录到创建回调结束。原生 Linux 反证纠正了额外的 VFS umask 处理假设，最终同一 ELF 在原生 Linux和解释器 1／2／4 CPU 通过。51 项 FUSE 回归通过；其余未变路径复用前一阶段验证，原始 FUSE 组仍为 22 通过、2 夹具跳过。保留失败实验和修复后证据；整批仍未交付。

- native 路径调度进展：续行进入 hostfs／私有卷／overlay I/O 前，保留原遍历和 FD 预留并交还执行槽，由现有 native executor 执行；重新进入 FUSE 时改等新的 daemon 请求，不对已完成请求发送中断。hostfs 启用时 56 项 FUSE 回归通过；763 项相关 VFS、18 项 Linux 文件路径测试及解释器 1／2／4 CPU 同 ELF 对照通过。guest ELF 未变，复用对应原生 Linux 结果；原始 FUSE 组仍为 22 通过、2 夹具跳过。仍需 proc 的 typed original／owner 回退和其余路径调用方；本阶段不新增线程，不勾选整批完成。

- proc／原资源回退进展：缺少 proc 元数据或 legacy TTY 资源时，保留原路径续行与 FD 预留，经已有 owner checkpoint 后回到正常调度，可继续等待 daemon／native 阶段，不重走原始 FUSE 路径。修正 proc-fd 重新打开 FUSE 普通文件时丢失文件能力的问题，保留原 inode／句柄并发起新 OPEN，验证独立偏移、源 FD 关闭复用和清理。63 项含 hostfs 的 FUSE、770 项相关 VFS、18 项 Linux 文件路径和 4 项解释器文件调度测试通过；扩展后的同一 ELF 在原生 Linux及解释器 1／2／4 CPU 通过。原始 FUSE 组仍为 22 通过、2 夹具跳过；仍需其余路径调用方（含 proc magic jump 后的 access）、异步 mmap、真实程序和整批验收，不新增协调线程、不勾选完成。

- access 最终 inode 进展：proc magic jump 后保留实际文件／目录身份，向原 nodeid 发起 ACCESS，跨 owner 等待保留 real／effective IDs；修正零 mask 的 F_OK 委托，并覆盖 fd 关闭复用、取消、ENOSYS、实时只读／noexec 和已删除 bind root 的 allow_other。宿主目标在已解析 inode 上交给现有 native executor，通过原 fd 刷新权限，避免重查被替换路径。72 项含 hostfs 的 FUSE、778 项相关 VFS、18 项 Linux 文件路径和 4 项解释器文件调度测试通过；同一扩展 ELF 在原生 Linux及解释器 1／2／4 CPU 通过，原始 FUSE 组保持 22 通过、2 夹具跳过。仍需其余路径调用方、完整权限／writer 生命周期、异步 mmap、真实程序和最终验收；不新增线程，不勾选整批完成。

- access 错误顺序补充：按 Linux 顺序先检查 noexec，再接收 ACCESS 的权限结果，成功后读取当前只读属性；避免提前 EROFS 掩盖 daemon 的 EACCES／EPERM，不为 access 获取 writer。daemon 在 ACCESS 回调期间实际 remount 的同一 ELF 对照在原生 Linux和解释器 1／2／4 CPU 通过。最新累计为 73 项含 hostfs 的 FUSE、779 项相关 VFS，通过 all-target／all-feature clippy；原始 FUSE 组保持 22 通过、2 夹具跳过。前一阶段 18 项 Linux 文件路径、4 项调度和执行边界结果按未受影响范围复用。此补充纠正上一阶段只读检查的阶段，整批继续待办。

- chdir 最终目标进展：复现并修正 FUSE→proc-fd→hostfs 在 native 等待期间因原路径被替换而切换到新目录的问题。最终 descriptor／inode 和宿主目录能力保留到 cwd 发布，proc-cwd 目标换目录后也不重定向；fchdir 复用同一检查，并通过原能力刷新 search 权限。新增 FUSE GETATTR／ACCESS 期间 rename、fd 复用、拒绝／取消清理及 ENOTDIR 回归。79 项含 hostfs 的 FUSE、默认 782／hostfs 826 项相关 VFS、18 项 Linux 文件路径及 4 项调度测试通过；同一 ELF 在原生 Linux和解释器 1／2／4 CPU 通过，原始 FUSE 组保持 22 通过、2 夹具跳过。普通路径长探针中位变化约 -0.01%／+1.40%／-0.24%。仍需 proc 跳转后带剩余组件的目录能力审计、其余权限／writer 生命周期、异步 mmap 和整批验收；不新增线程，整项保持未完成。

- proc 目录剩余组件进展：修正跳转后带子路径、`.`／`..` 或尾斜杠时丢失原目录身份的问题；hostfs 保留原生目录能力，内存／私有卷保留原 inode，覆盖等待期间 fd 关闭复用、rename、同名替换、已删除目录和挂载遮盖。父目录写操作及普通硬链接在现有命名空间锁内重定位原目录，不新增协调线程或轮询总线。84 项含 hostfs 的 FUSE、默认 785／hostfs 833 项相关 VFS、18 项 Linux 文件路径及 4 项调度测试通过；同一 ELF 在原生 Linux和解释器 1／2／4 CPU 通过，原始 FUSE 组保持 22 通过、2 夹具跳过。普通路径长探针中位变化约 -0.47%／-0.16%／+0.11%。仍需其余路径调用方（含经 FUSE 符号链接解析 mount 目标）、完整权限／writer 生命周期、异步 mmap、真实程序和整批验收；整项保持未完成。

- 传统挂载路径进展：mount／umount／pivot_root 已接入 FUSE 续行，等待期间释放现有 mount-admin 锁，并保留每个路径参数的原对象。source／target 即使同名也分别接受；bind 后续复用同一次 source 解析，hostfs 参数经历另一个 FUSE 等待后会重新交给现有 native executor。94 项含 hostfs 的 FUSE、默认 794／hostfs 843 项相关 VFS及 4 项调度测试通过，18 项未受后续调整影响的直接文件测试复用；同一扩展 ELF 在原生 Linux和解释器 1／2／4 CPU 通过，原始 FUSE 组保持 22 通过、2 夹具跳过。普通路径长探针中位变化约 -0.09%／-0.87%／-0.83%。未新增协调线程。仍需 FUSE 普通文件 bind 能力、现代 mount-FD 及其他路径调用方、权限／writer 审计、异步 mmap、真实程序和整批验收；不勾选完成。

- FUSE 普通文件 bind 进展：保留原 nodeid／generation，支持路径及 proc-fd 来源、O_PATH fstat、源 FD 关闭／unlink 后重新打开、读写及只读 remount；普通文件也可覆盖 FUSE 文件。修正 GETATTR 等待重放、路径缓存清空后的 ENOENT，以及 proc 元数据投影拖住卸载连接的问题。真实挂载／OFD／FLUSH 共享连接使用权，冷缓存不延长其活动寿命；不新增协调线程。104 项含 hostfs 的 FUSE、默认 804／hostfs 853 项相关 VFS、19 项直接文件及 4 项调度测试通过；同一 ELF 在原生 Linux及解释器 1／2／4 CPU 通过，原始 FUSE 组仍为 22 通过、2 夹具跳过。普通路径长探针中位变化为 stat -0.53%、open/close +2.16%、getcwd +0.19%。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#regular-file-bind-and-connection-lifetime-checkpoint)。仍需现代 mount-FD 及其他路径调用方、权限／writer 审计、异步 mmap、真实程序和整批验收，整项保持未完成。

- 现代 mount-FD 路径进展：open_tree／move_mount／mount_setattr 的目录和普通文件参数已接入同一续行机制，覆盖空路径原 FD、关闭复用、另一参数等待时保留已接受源、属性修改、取消和 native 调度。112 项含 hostfs 的 FUSE、默认 811／hostfs 861 项相关 VFS通过；同一扩展 ELF 在原生 Linux及解释器 1／2／4 CPU 通过。原始 mount／mount_fd 158 项中，153 首跑通过，另两项按既有 userspace ABI 模式复核通过，三个原生也跳过的用例保留；FUSE 组仍为 22 通过、2 夹具跳过。普通路径长探针中位变化 -1.03%／-1.04%／-0.83%。另已取得明确失败对照：NOFOLLOW 的 FUSE 符号链接树及空路径 readlink 探针在 Linux 通过，Trames 在 open_tree 返回 EINVAL；继续补该 inode 能力，不将本阶段等同于全部现代挂载已完成。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#modern-mount-fd-directory-and-regular-file-continuation-checkpoint)。未新增协调线程，整项继续待办。

- 符号链接 inode 进展（2026-11-24）：已修复上轮 NOFOLLOW 差异，支持克隆／非克隆树 FD、O_PATH、空路径 readlink、源链接删除替换和绑定链接的相对／绝对遍历；同时修复 READLINK 等待时父目录 rename／同名替换导致误用新目录的问题。原失败 ELF 原样复测通过；扩展后的同一 ELF 在 Linux及解释器 1／2／4 CPU 通过，120 项含 hostfs 的 FUSE、默认 818／hostfs 869 项相关 VFS通过。原始 FUSE／mount／mount_fd 共 182 个身份按已有 ABI 和 fusectl 夹具复核后，178 通过、4 原始跳过，覆盖全部 177 个原生通过项；额外通过项的未连接 FUSE clone errno 在参考内核为 EPERM、当前／上游预期为 EINVAL，保留待错误审计。修正计时后的普通探针中位变化为 stat -1.16%、open/close +1.23%、getcwd +1.39%，历史长探针百分比已标注失效。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#daemon-leaf-inodes-and-symlink-capabilities-checkpoint-2026-11-24)。未新增协调线程；其他路径消费者、权限／writer、异步 mmap、真实程序和整批验收仍待完成。

- statfs／chroot 进展（2026-11-24）：两者已接入保留原对象的续行，STATFS 使用目标 nodeid 并在回复后读取当前挂载标志；chroot 等待 ACCESS 后只发布原 fs_struct 的 root，保留 cwd 和目录 FD，hostfs 根保留原生目录能力。同时修复退出 CPU 返回记录继续持有文件等待、父任务 waitpid 后 daemon 仍可读到未发请求的竞态，并用确定性失败／成功回归验证。127 项含 hostfs 的 FUSE、默认 824／hostfs 876 项相关 VFS、19 项直接文件、4 项文件调度及 29 项运行队列测试通过；同一扩展 ELF 在 Linux及解释器 1／2／4 CPU 通过。原版 gVisor 301 个身份合并后 296 通过、5 原始跳过，覆盖全部 294 个参考内核通过项；FUSE 另补单核、四核各 23 通过、1 原始跳过。未连接 clone 在隔离 Linux 夹具仍返回 EPERM，根因未定，保留错误审计。普通路径两轮有效计时分别测到 stat +2.06%／+5.99%、open/close +5.03%／+2.26%、getcwd +3.64%／+1.51%，两轮均保留，性能门槛仍待定位。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#statfs-chroot-and-retired-wait-checkpoint-2026-11-24)。未新增协调线程；hardlink／属性／xattr、非普通文件数据行为、权限／writer、异步 mmap、真实程序和整批交付仍待完成。

- 普通路径性能排查（2026-11-24）：补充同一 runner 的 A/A 对照、相同指令／syscall／等待计数诊断，以及工作量加长十倍的有效时钟探针。去掉目录等待对象中重复的 root／cwd 状态，使 ARM64 `resolve_walk` 的局部栈预留从 0xa80 恢复到基线 0xa70；127 项 FUSE、原版 FUSE／chroot 36 项（35 通过、1 原始跳过）、解释器 1／2／4 CPU 同 ELF及 clippy／执行边界检查通过。三版本长探针按六种顺序平衡测量，当前版相对 leaf 基线为 stat +0.61%、open/close -1.27%、getcwd -2.03%，相对前一版为 -0.96%, -2.54%, -0.12%；未复现短探针原先 2%—6% 的同等幅度，但不把全部差异归因于 16 字节，也不替代真实 FUSE 负载和吞吐验收。旧数据完整保留，未新增协调线程。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#ordinary-pathname-controls-and-continuation-state-size-checkpoint-2026-11-24)。下一批继续 hardlink 的原 source inode／空路径 FD 保留、LINK 回复及取消引用清理；整项仍未完成。

- hardlink 进展（2026-11-24）：LINK 已保留原 source inode、挂载及空路径 FD，覆盖等待期间 rename／替换、O_PATH 和数据 OFD、符号链接跟随、保护策略、最后 unlink 后拒绝恢复，以及取消／晚回复 lookup 引用清理。按原生结果修正不同 nodeid 回复和默认权限委托，并修复 ENOSYS 缓存掩盖跨挂载 EXDEV 的错误顺序。最终同一 ELF 在 Linux及解释器 1／2／4 CPU 通过；139 项 FUSE 和当前 clippy／边界检查通过，未受最终窄改影响的 835／888 项 VFS、19 项直接文件和 4 项调度结果复用。本轮重跑原版 FUSE／link／symlink／unlink 共 103 个身份，经既有 ABI 复核后 102 通过、1 原始跳过；合并账本为 380 个身份、375 通过、5 跳过，覆盖全部 373 个参考通过项，其中 277 项复用未受影响的历史结果。普通长探针 getcwd +3.83% 与独立 getcwd +0.91% 均保留；栈帧拆分试验未证实稳定收益，已撤回并保留三版本数据，性能门槛仍开放。未新增协调线程，整批尚未提交、更新 pin 或发布。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#hardlink-original-source-and-protocol-checkpoint-2026-11-24)。下一组继续 SETATTR／xattr 和原 inode 续行；metadata 并发次序、非普通文件数据、权限／writer、异步 mmap、通知、真实程序／吞吐及整批验收仍待完成。

- chmod／chown／utimens 进展（2026-11-24）：路径、数据 FD 和 O_PATH 空路径已向原 inode 发送 SETATTR，保留挂载 writer，覆盖取消、拒绝及畸形回复；修复目录 OFD 在修改属性后仍读到旧缓存的问题。842 项默认 VFS、895 项 hostfs VFS（含 146 项 FUSE）及 clippy／边界检查通过，同一夹具在 Linux和解释器 1／2／4 CPU 通过。本轮原版属性／FUSE 88 个身份为 87 通过、1 原始跳过；合并账本为 444 个身份、439 通过、5 跳过，覆盖全部 437 个参考通过项，其中 356 项复用未受影响结果。普通调用曾测到约 57% 的路径保留开销，第一次调整后仍慢约 39%—46%；两轮均保留，继续移除常规解析中的额外调度预查和共享状态借用后，长样本 chmod／chown／utimens 对照为 -2.83%／+0.13%／-0.92%。该内存后端测量不替代持久存储、真实应用和吞吐验收。未新增协调线程，整批尚未提交／更新 pin／发布。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#chmod-chown-and-timestamp-setattr-checkpoint-2026-11-24)。下一组继续 xattr 的协议、缓冲区容量和原对象保留，其余 FUSE 门槛仍待完成。

- xattr／ACL／capability 进展（2026-11-24）：GET／SET／LIST／REMOVE 已使用原 inode、OFD 和实际缓冲区容量，覆盖查询、长度／列表校验、逐操作 ENOSYS 缓存、取消及 mount writer 生命周期；补齐 legacy ACL 和 chown 清除 capability，并修复空属性名与无效 value 的错误顺序。保留初始 ENOTSUP、四个原生语义差异和 EFAULT／ERANGE 反例，24 个诊断结果已成为严格断言，同一最终 ELF 在 Linux及解释器 1／2／4 CPU 通过。856 项默认 VFS、909 项 hostfs VFS（含 160 项 FUSE）、19 项直接文件和 clippy／边界检查通过。当前 260 个原版身份经三项既有 ABI 复核后为 247 通过、13 原始跳过，覆盖全部 246 个参考通过项；与 356 项未受影响结果合并后共 616 个身份、599 通过、17 跳过，覆盖全部 597 个参考通过项。普通内存后端 3M 迭代长样本 SET／GET／LIST／REMOVE+SET 为 -1.81%／-3.60%／-0.19%／+1.80%，短样本也保留，不替代持久存储和真实应用验收。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#xattr-acl-and-capability-wire-checkpoint-2026-11-24)。未新增协调线程；hostfs xattr 仍返回 EOPNOTSUPP，完整 ACL／权限／错误审计、属性并发、异步 mmap、真实程序／吞吐和整批交付仍待完成。

- hostfs 数据属性进展（2026-11-24）：普通原生 GET／SET／LIST／REMOVE 已移除统一 EOPNOTSUPP，保留实际 inode／FD 和原挂载 writer，覆盖关闭复用、rename／unlink、只读准入、trusted 名称过滤及独立 host chmod 后的权限刷新。默认 856／hostfs 914 项完整 VFS 回归通过，随后新增两项测试的六项 host xattr 定向组通过；Linux 原生 xattr 20、hostfs 23、native 路径 19 项通过（选集有重叠）。同一 FUSE 夹具在解释器 1／2／4 CPU 通过；260 个原版身份复核后仍为 247 通过／13 原始跳过。CLI 未启用 hostfs，其结果证明共享普通／FUSE 路径；hostfs 行为由启用该特性的 Mac／Linux VFS 验证，尚不作为 guest host-mount 或 HVF 验收。保留 symlink ELOOP、交叉链接失败及磁盘保护结果，补齐 musl renameat2 的直接 syscall 路径。普通 xattr 的 3M 迭代长对照 SET／GET／LIST／REMOVE+SET 为 +1.30%／+2.38%／+0.45%／-0.10%，数据保留，真实性能门槛继续开放。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#native-host-data-xattr-interoperability-checkpoint-2026-11-24)。未新增协调线程；host ACL／capability 身份转换、Linux O_PATH 链接属性以及其余 FUSE 门槛仍待完成，整批未提交或发布。

- FUSE 本地 mode 权限进展（2026-11-24）：支持 `default_permissions`，在原 inode／目录和原凭据上完成 search、open、access、chdir、属性／xattr 和父目录变更检查，过期或缓存拒绝时经现有等待路径刷新 GETATTR；补齐受保护硬链接和跨父目录 rename 的写权限。保留 mount EINVAL、open intent 丢失和两项错误放行反例，严格同一 ELF 在 Linux及解释器 1／2／4 CPU 通过，原 delegated 夹具及 24 项属性诊断保持通过。866 项默认 VFS、926 项 hostfs VFS（含 171 项 FUSE）及当前十项定向权限、clippy／格式／执行边界检查通过。当前 535 个原版身份为 522 通过／13 原始跳过，覆盖全部 521 个参考通过项；合并未受影响结果后为 812 个身份、795 通过／17 跳过，覆盖全部 793 个参考通过项。普通长样本 stat／open-close／getcwd 为 +0.84%／+2.09%／+0.75%，RAM 后端 chmod／chown／utimens 为 -1.26%／-0.96%／-0.48%，原始样本保留，性能门槛继续开放。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#local-fuse-mode-permissions-checkpoint-2026-11-24)。未新增协调线程；sticky 目录 O_CREAT、协商 POSIX ACL、属性并发／错误审计、host 权限映射、异步 mmap、真实程序和整批交付仍待完成，02 保持未勾选。

- sticky O_CREAT／proc FD 进展（2026-11-24）：复用已有保护策略，保留最终父目录和原 inode，补齐 FUSE 文件绑定到外部 sticky 目录后的检查；严格区分本地权限与 daemon 委托，保留 O_EXCL 等错误顺序。同时补上降权／不可 dump 后同线程组访问 proc FD 目录的 search 例外，其他进程和 fdinfo 不误放行。保留两项原生前后反例及中间 proc 重开失败，同一最终 ELF 在 Linux 两种模式和解释器 1／2／4 CPU 均通过；旧属性夹具及 24 项诊断保持通过。当前 172 项 FUSE、三项进程凭据及 clippy／格式／runtime 边界检查通过，命名空间选集的一项沙箱网络拒绝单独复跑通过。受影响原版 242 个身份为 239 通过／3 原始跳过，覆盖全部 238 个参考通过项；合并 730 项未受影响结果后，账本为 972 个身份、953 通过／19 跳过，覆盖全部 951 个参考通过项。磁盘保护停止和缓存清理记录均保留，性能沿用上阶段原始观测，不声称当前版完成真实负载门槛。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#sticky-o_creat-and-same-group-proc-fd-search-checkpoint-2026-11-24)。未新增协调线程；完整 POSIX ACL、权限／属性并发审计、非普通数据、异步 mmap、通知、真实程序及整批交付仍待完成，02 保持待办。

- 协商 POSIX ACL 进展（2026-11-24）：INIT 协商后自动启用本地权限，原对象／凭据贯穿初始化、GETATTR 和 ACL 等待；补齐命名用户／mask、映射命名空间、DONT_MASK、扩展 SETXATTR 的 SGID 意图，以及创建、父目录变更、硬链接、移动源目录、NOW 时间和用户 xattr 的检查。按原生反例修正根 inode 不缓存 ACL、GETATTR 失效及晚回复不能恢复旧缓存，未新增协调线程。同一最终扩展 ELF 在 Linux 三种模式和解释器 1／2／4 CPU 的九种组合通过，旧属性夹具 24 项诊断保持通过。最终 FUSE 单测的首次 176 通过／1 项测试预期错误，与随后五项定向通过合并覆盖全部 177 个身份；此前完整默认 898／hostfs 958 项（各一项既有忽略）按未变范围复用，clippy／格式／执行边界通过。受影响原版 695 个身份含六项明确复用的 ABI 结果，共 680 通过／15 跳过，覆盖全部 679 个参考通过项；最终缓存修改重跑 FUSE 23 通过／1 跳过，合并账本仍为 972 个身份、953 通过／19 跳过及全部 951 个参考通过项，非新一轮全量。前一 ACL runner 的普通 stat／open-close／getcwd 长对照为 -0.91%／-2.34%／-1.34%，RAM chmod／chown／utimens 为 +1.01%／+0.72%／-2.59%，保留版本和原样本，不作为最终 runner 或真实 FUSE 吞吐验收。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#negotiated-posix-acl-and-cache-lifetime-checkpoint-2026-11-24)。ACL 错误／命名空间及属性并发审计、host 权限映射、非普通数据、异步 mmap、通知、真实程序及整批交付仍待完成；02 保持待办，未提交／更新 pin／发布。

- ACL 格式／公开缓存进展（2026-11-24）：分开 daemon ACL 解码与设置校验，按 Linux 保留非标准 entry 的读取／权限结果、EIO 传播、所有权与语义校验顺序，以及版本／长度组合错误的优先级。公开 access/default ACL 与权限检查共享原 inode 缓存，全局 ID 按读取者命名空间输出；晚回复不恢复已失效缓存，未新增协调线程。保留两轮原生通过而旧候选失败的同 ELF 反例，最终夹具的 21 类输入在 Linux 三种模式和解释器 1／2／4 CPU 九组组合全部通过，逐项返回值、errno、ID 和查询次数一致；旧属性夹具 24 项诊断保持通过。最终完整 hostfs VFS 961 通过／一项既有忽略，含全部 179 项 FUSE 和七项 ACL，clippy／格式通过；前一默认配置的 901 项及边界检查按未变范围保留。最终原版 xattr／ACL／FUSE 182 个身份经既有 ABI 复核后为 169 通过／13 跳过，覆盖全部 168 个参考通过项；与 790 个未受影响结果合并后仍为 972 个身份、953 通过／19 跳过及全部 951 个参考通过项，非全量重跑。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#acl-backing-format-public-cache-and-namespace-checkpoint-2026-11-24)。下一步用原生探针验证同版本并发填充／取消归属，再推进属性版本和 bad-inode；其余 FUSE 门槛及整批交付仍待完成，02 保持待办。

- ACL 并发填充／取消进展（2026-11-24）：原生握手探针证实同版本并发读取的后回复会误覆盖缓存，已补上每种 ACL 的请求填充归属；跟随请求仍独立发出并使用各自回复，错误／取消仅释放自己的归属，不提升旧跟随者，也不影响替代请求。沿用原 inode 对象锁和失效版本，未新增协调线程。最终 12 场景同 ELF 在 Linux 初始／映射命名空间和解释器 1／2／4 CPU 六组组合通过，覆盖公开读取与权限检查的双向交叠、回复顺序、负缓存、错误、信号取消和替代填充；前一 21 类夹具九组及旧属性 24 项诊断也保持原生一致。当前 182 项 FUSE（含十项 ACL）、clippy／格式／边界检查通过，782 项未变的 VFS 通过结果复用；原版 FUSE 23 通过／1 跳过，覆盖全部 22 个原生通过项。保留旧 runner 的同 ELF 失败及仅生成对象缓存的清理记录；合并账本仍为 972 个身份、953 通过／19 跳过及全部 951 个参考通过项。详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#concurrent-acl-fill-and-cancellation-ownership-checkpoint-2026-11-24)。下一组转向属性请求版本和 bad-inode；其余 FUSE 验收／交付及 03–12 仍待完成。

- 属性版本／bad-inode 进展（2026-11-24）：新增 13 场景原生握手夹具，修复晚到 GETATTR 覆盖新缓存、并发 SETATTR 的过期行为、错误类型／大小回复的持续 EIO，以及 nodeid 代次／类型复用；补齐原 FD 的 `faccessat2(AT_EMPTY_PATH)` 依赖，沿用对象同步和现有等待机制，不增加协调线程。相同 ELF 在原生初始／映射命名空间、解释器 1／2／4 CPU 六组组合逐项一致；ACL 并发六组、前一 ACL 夹具九组和旧属性 24 项诊断也保持原生一致。VFS 合并复测 968 通过／1 忽略，最终 185 项 FUSE、Linux 调用路径、clippy／格式／边界检查通过；原版 FUSE＋access 为 42 通过／1 跳过，累计账本仍为 972 身份、953 通过／19 跳过。普通路径性能及完整证据见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#metadata-request-epochs-and-bad-inode-checkpoint-2026-11-24)。仍需审计 LOOKUP 提前返回、错误优先级、其他元数据／协议边界及整批工具／平台验收；02 和 03–12 保持未完成。

- LOOKUP 提前返回进展（2026-11-24）：原生 16 场景确认 EEXIST／ENOTDIR／ELOOP 和符号链接跟随之前也要发布有效 LOOKUP 的规范属性与身份；候选已调整顺序，并保留坏 inode 的 O_PATH／排他创建优先级，实际操作继续返回 EIO。新夹具和前一属性并发夹具均在初始／映射命名空间、解释器 1／2／4 CPU 六组组合逐项匹配原生；当前 188 项 FUSE、定向 ACL、clippy／格式／边界检查通过，783 项未变的非 FUSE 结果复用。原版 FUSE 与普通文件四组按各自匹配的 ABI 模式合并为 173 通过／1 跳过，累计账本保持 953 通过／19 跳过。未增加协调线程，未更新提交或 runtime 引用。下一组继续审计 root／无效 nodeid、代次及协议／错误边界，整批工具／平台验收仍待完成；详见[阶段记录](../../runtime/docs/validation/fuse-complete-progress-20261123.md#lookup-early-return-publication-checkpoint-2026-11-24)。

### 03 — 现代挂载剩余语义与 OCI 验收

- [ ] 状态：已有部分实现及验证，仍需补齐依赖和实际工具验收。
- 剩余实现重点为 ID-mapped mount，以及 tmpfs 以外的文件系统重配置。`nosymfollow`、传播属性与 tmpfs `fspick`／重配置已合入，仍需整批 OCI 场景验收。
- `mount_setattr` 的已验证递归属性修改，以及 `statmount`／`listmount`／原版 findmnt 查询，不再按未实现项处理。
- 完成标准：挂载局部 UID／GID 映射贯穿权限、所有权查询和创建；检查 namespace 权限、共享／私有存储及失败回滚。重配置正确影响同一文件系统的别名，并保持各挂载属性独立。用实际 runc／OCI 配置验收对应启动路径。
- 依据：[挂载属性](../../runtime/docs/validation/guest-mount-setattr-20261123.md)、[挂载查询](../../runtime/docs/validation/guest-mount-query-20261123.md)、[路线图](syscall-implementation.md#2-modern-mount-administration)。

### 04 — cgroup 资源管理补全

- [ ] 状态：实现＋记账和压力验收。
- CPU／cpuset、匿名内存控制、OOM group、映射 cache／别名及 fork COW 页的基本归属与计费已有实现；剩余重点是非 mmap IO、native extent 的动态计费及实际内存回收行为。
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

- [ ] 状态：基础实现已合入并通过已覆盖检查；原版 helper／自测、真实程序和两后端验收未完成。
- futex2：`futex_waitv`、`futex_wait`、`futex_wake`、`futex_requeue`，复用现有等待队列及 private／shared key。
- rseq：同时处理线程注册、CPU 身份、迁移、抢占、信号中止和线程退出；恢复原始 no-libc 测试依赖后再作为覆盖依据。
- 完成标准：Linux 自测及真实调用程序通过，覆盖唤醒／重排／超时／信号竞态和共享别名；两种 CPU 后端语义一致，并验证旧 futex 正常路径性能。rseq 的优先级结合实际程序收益确定。
- 依据：[线程同步路线图](syscall-implementation.md#4-thread-synchronization)。

### 08 — guest perf 工具链

- [ ] 状态：软件时钟事件、FD／控制及基本采样环等已合入；完整生产者／作用域和原版工具验收未完成。
- 补齐 `perf_event_open` 的剩余选定事件、采样中断／开销预算、CPU／cgroup 作用域及映射／权限依赖；不将基本环和计数回归等同于 perf 工具可用。
- 完成标准：原版 `perf stat`／`perf record` 对任务 CPU 时间、上下文切换等选定事件可用，计数及采样可核对。硬件 PMU 需后端能够准确归属 guest 并通过测量后再宣布支持。
- 依据：[性能工具路线图](syscall-implementation.md#5-guest-performance-tooling)。

## 计划、优化与测量任务

### 09 — 动态任务上限

- [ ] 默认 65,536、可写 `/proc/sys/kernel/threads-max` 及启动配置共享实现已合入，实际 guest 初始化／动态读写／降低上限后拒绝 fork 已通过；实际产品及真机交付验收仍待完成。
- 实现采用 `[kernel.sysctl]` 的 `"kernel.threads-max"` 键及 `--sysctl kernel.threads-max=VALUE`。VFS proc 读写、session 准入与两端启动配置共用一个原子资源；`pid_max`、`RLIMIT_NPROC`、cgroup `pids.max` 保持独立，不预分配资源。
- 完成标准：读写、权限、持久配置初始化及任务准入行为一致，两端均有验证。
- 依据：[配置与候选状态](../guides/ipad-environments.md#task-capacity-and-live-configuration)。

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

所有者于 2026-11-24 修正迭代方式：一轮先完成当前所有已知问题的实现，再统一进入测试验证；问题属于不同领域也先分别实现，不再采用“修一个问题就展开一轮验证”的流程。

1. 开始时核对源码与已有后续记录，统一列出当前全部已知问题、实现缺口、依赖、触发程序及完成标准；不得只挑容易验证的部分作为本轮完成范围。
2. 先完成清单内全部问题及其依赖、调用方和回退路径。跨领域分别落地，实现阶段不穿插构建、原生对照、CPU／命名空间矩阵或性能验收；行为疑点先通过源码和协议资料调查，待验证假设集中记录。
3. 实现全部完成后固定候选源码和产物身份，再按受影响范围集中构建、运行共享 Runtime、Mac 和解释器测试，补 gVisor 未覆盖的真实工具／负载及正常负载性能验证；平台相关项目做真机验收。
4. 验证发现的问题先汇总，作为下一轮实现清单；同样先全部修复，再统一重跑受影响项目和失败项，不退回逐个修复、逐个展开验证的循环。
5. 全程遵守现有 guest 内存、并发及对象级同步原则；阻塞等待交还执行槽，不新增协调／资源服务线程或轮询消息总线。复用未变源码的通过证据；文档、记录及测试等待方式调整不触发完整 CPU、Linux、网络、图形重验。完整检查及全量 gVisor 留在相应集成／发版节点。
6. 本批实现和验证全部完成后更新本页状态、验证记录和遗留限制，提交并推送 `main`；涉及 Runtime 时同步两端配套引用，保留已测试提交身份。

本清单之外的 syscall、CPU 扩展及产品功能继续由各自路线图管理。此处不把所有未实现接口自动提升为本轮必做项。
