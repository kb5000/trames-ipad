# FUSE 阻塞传输与复制续执行（2026-11-25）

[真实工具检查点](fuse-real-tools-20261125.md)的两个阻塞已修复：低层 libfuse 不再因阻塞 splice 错返 EAGAIN 而退出，BusyBox 复制不再把中间元数据回复交给最终 copyout。扩展场景另发现的源端 FUSE 短入口遗漏、普通文件位置等待问题也已集中处理。

原版 libfuse 三种配置在解释器 1／2／4 CPU 的九个组合全部通过；受影响原版 gVisor 按身份合并为 411 通过、10 跳过，覆盖全部 410 个 Linux 参考通过项。02 的协商写回缓存、release 性能和最终交付门槛仍开放，03–12 按原顺序保留。本批未提交、推送或更新配对 pin。

## 候选与来源

- 产品主线 `d84dd19d497700cd82953fef5b202429aa54be9b`，Runtime 主线 `76f860287368605e3697bd934816c18490967c2f`；现有已提交 pin 配对相符。实际验证包括其上的未提交候选。
- 证据根目录为 `target/validation-fuse-transfer-20261125`，Linux 与真实工具证据在 `target/local-ci/io/fuse-transfer-20261125`。沿用本任务独立 Linux 构建目录，没有重启共享 VM、修改全局挂载或清理其他任务产物。
- `initial-source.json` 与初始补丁保存进入本批前的 WIP，`test-source.json` 保存第一阶段，`refined-source.json` 保存最终生产候选。`source-comparison.json` 核对 HEAD 与已记录的非文档文件：最后只有一项回归夹具变化，生产源码没有漂移。
- 第一阶段 runner SHA-256 为 `eb91da2c1b4595cfb488a2105427b8b2d59cede78b98d4fd3f1345c4de7fe736`；最终 `trames-run-final` 为 `2c343b763828c0ca3821c5c0342c1a38404b88f93e5db049cdccdf9f68828ff8`。构建来源见 `runner.json`、`final-runner.json`，原始失败与最终产物分别保存。

## 实现

1. StreamTransfer 准入原 FUSE 设备／文件与 pipe，使用已接受的 OFD、原 pipe 前缀／容量预留及设备 readiness。阻塞设备没有请求时等待原连接；保留非阻塞、容量不足、关闭和取消分支。反向 pipe 回复也经过原设备的通知／协议处理。
2. 内部 `transfer_io` 保存中间回复与实际等待对象。FileWork 接收 GETATTR／READ／WRITE 结果后回到原复制步骤，不把它们当作 syscall 最终输出。FileCopy 保留原元数据、初始偏移、已读字节和已写长度；恢复偏移不能再次执行完成的写入。源与目标映射不同，下一步骤重新取得对应布局准入。
3. 源端需要 daemon 的传输也进入完整 FileWork。旧短路径只检查输出端，FUSE→pipe 可能把请求留在临时视图后以空视图恢复。现已统一 inline、已有 owner 和队列入口；VFS 冷回退查询保留整个工作，不只保存最后的元数据请求。
4. 原普通文件位置被并发 read／copyout 占用时，传输保留原位置并释放执行槽等待。读取前等待不提前消费输入；输出完成后的等待不重写输出。没有新增协调／服务线程、轮询总线、全局复制锁或普通访存屏障。
5. 对照 [Linux do_sendfile](https://github.com/torvalds/linux/blob/v6.12/fs/read_write.c) 的原资源与局部偏移处理，不把复制升级为全局事务，不承诺并发偏移更新的确定性获胜顺序。原文件大小、权限、显式偏移、短传输和布局检查继续保留。

## 真实工具

继续使用未修改的 libfuse 3.14.0 `passthrough_ll`／`passthrough`。工具 ELF、包与源码来源沿用前一检查点；扩展普通客户端加入 sendfile、copy_file_range、FUSE 文件↔pipe splice、FUSE↔普通 tmpfs 文件复制，以及 EOF／偏移断言。原生和 guest 使用同一客户端 ELF、工具和 BusyBox rootfs，哈希在工具目录的 `inputs.json`。

| 范围 | 最终结果 |
| --- | --- |
| Linux ARM64：低层、请求 writeback、高层 | 3／3 配置通过 |
| Trames：相同配置 × 1／2／4 CPU | 9／9 配置通过 |
| guest：BusyBox、映射、pipe 输出、标量输出、AIO、复制、8 MiB 传输 | 63／63 功能检查通过 |
| guest：卸载、daemon 停止、退出 | 27／27 检查通过 |

共享／私有映射、截断 SIGBUS、关闭／删除名字后的原映射同步、跨页输出与 AIO 内容断言全部保留。复制检查显式偏移不改变 OFD 偏移、返回偏移、双向内容与 EOF。每项有独立期限，最终没有失败、中止、外层超时或磁盘停止；工具阶段 132.403 秒包含九次导入和启动，不能据此计算 FUSE 吞吐。

第一阶段已关闭旧挂载／BusyBox 阻塞，但九个新增复制场景均在 FUSE→pipe 返回 EIO。完整失败先收齐，再连同并发位置问题处理。`guest-results.json`、`final-guest-results.json` 和各自 stdout／stderr、daemon trace 均保留。

## Rust 与平台检查

| 范围 | 最终按身份合并 |
| --- | --- |
| Apple VFS／Linux 调用库 | 1629 通过、0 失败、2 忽略 |
| Linux ARM64，同上 | 1637 通过、0 失败、2 忽略 |
| 产品解释器接入／FFI，本次实际运行 | 565 通过、0 失败／中止、7 忽略 |

完整两库运行最后各有一项新夹具预期错误：在读取前让输出位置等待，却预期输入已被读取。夹具改为先接受 FUSE READ，再让输出位置等待，继续严格检查旧字节、原偏移和一次性读取。最后只重跑本批十个原资源回归，两端全部通过；按同名身份覆盖旧结果，其他通过结果直接复用。来源见 `mac-merged-results.json`、`linux-merged-results.json`，不是全工作区数量，不累加重复运行。

十个回归覆盖设备等待／反向回复、容量与取消、FD 复用和短写、双 FUSE 文件、迟到读取、普通偏移等待、回执遇到 pipe 满，以及 canonical／inline／owner 三种入口。夹具编译错误与旧预期失败日志没有删除。

产品一次调用覆盖全部 543 个 conformance 身份（540 通过／3 忽略）及 25 项 FFI，无中止后遗漏；总耗时 112.414 秒，conformance 执行 20.13 秒。子进程 trace 会打断 libtest 行，收集器按可执行文件清单、完整摘要与全部失败／忽略身份规范化；其中五个通过身份由完整摘要约束补齐，不计入嵌套子测试。

两端受影响库全目标／全功能严格 Clippy 通过；最后只改夹具后又窄查 VFS。格式、shell 语法和 Runtime 执行／宿主／项目边界检查通过。共享修复没有引入 Apple API／类型；平台专用依赖和链接继续沿用现有 target cfg。

## 原版 gVisor

初次统一运行 aio、copy_file_range、iouring、mmap、msync、sendfile、sendfile_socket、splice、signalfd、timerfd、fuse，共 421 个身份：407 通过／10 跳过／4 失败。三个 socket 失败来自显式关闭网络时的 EACCES；另一个为 `SpliceTest.FromPipeWithConcurrentIo` 中原位置等待泄漏的 EAGAIN。

最后只重跑受影响四组 82 项，为 socket 启用隔离 loopback，结果 80 通过／2 跳过，耗时 55.928 秒。其余七组 339 项按未受第二阶段窄改影响的范围复用；`gvisor-merged` 按身份覆盖后为 **411 通过／10 跳过／0 失败**，覆盖全部 **410／410** 原生参考通过项。原生失败的 FUSE clone 身份在 Trames 通过，不放入原生通过分母。

原生参考复用 `gvisor-full-native-20261123-final`，每行保留原证据位置；原 ELF 哈希一致，使用普通 Linux ABI，未设置 `TEST_ON_GVISOR`。`gvisor-xml-audit.json` 重读全部 21 份 XML，核对两阶段共 503 次执行；身份集合仍是 421 个。合并结果保留每行实际 runner／网络来源，不假称都在最后产物重跑。严格比较见 `gvisor-comparison/comparison.json`，不增加固定全量台账的历史累计通过数。

## 仍开放

原生请求 writeback 时打印激活日志，guest 仍未协商 `WRITEBACK_CACHE`；请求该选项或 mmap 同步通过不能证明协商缓存已实现。继续完成该能力及真实工具验收，再做匹配 release 产物的吞吐／普通负载对照，以及最终 HVF／物理 iPad、完整 CI 和配对提交推送。02 与完整 01–12 目标均未关闭。
