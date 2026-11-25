# FUSE 协商写回缓存检查点（2026-11-25）

`WRITEBACK_CACHE` 已从未协商状态接入普通缓存读写及现有页回写执行器。原版 libfuse 的 writeback 配置在解释器 1／2／4 CPU 均确认实际激活；低层、writeback、高层三种配置共九组全部通过。本页承接[传输／复制检查点](fuse-transfer-20261125.md)，不据此关闭 02 整个领域或 01–12 目标。

本 FUSE 批次尚未提交、推送或更新 runtime pin；仍需匹配 release 性能、完整领域审计及最终平台／交付门槛。没有新增协调／资源服务线程或轮询总线。

## 产物与源码

- 证据根目录：`target/validation-fuse-writeback-20261125`。首轮真实工具证据在 `target/local-ci/io/fuse-writeback-20261125`，最终在 `target/local-ci/io/fuse-writeback-final-20261125`，原始失败未覆盖。
- 开始时产品 HEAD 为 `d84dd19d497700cd82953fef5b202429aa54be9b`，实际 Runtime HEAD 为 `7fe39ef60b0bd04a6a66556d24df1c1e749ad7a2`。验证包含其上的未提交候选。`initial-source.json` 与初始补丁保存进入本批前的其他 WIP。
- 共享主线期间更新到产品 `d61ac3d3273a93946f71b05e069c66c9acf87181`、Runtime `542b14f9e3dff51d89d61043d5a77916b8417c25`，已提交 pin 配对一致。Runtime 的新增提交只修改 `tests/acceptance/cli.json`，产品为文档／pin 更新，没有改变本批编译实现；没有重置其他任务的修改。
- 第一阶段 `trames-run` SHA-256：`2655a00f2260ac01a3710b47e46fd8f41b88c6cedb27b90e305e50e533baa5e2`。最终 `trames-run-final`：`625ee231b86b8f3d7d4021718d215e42f7d028d9ac9fb5976042410b9a079c60`。
- `tested-source.json`、`refined-source.json`、`final-source.json` 区分各候选。最终非文档源码指纹没有漂移，见 `final-source-comparison.json`。`audit.py` 重读收集结果、原始 XML、客户端检查标记及产物哈希，输出 `final-results.json`。

## 实现范围

1. INIT 广告并按协议版本接受写回缓存位；未协商的挂载继续使用既有写穿路径。Apple 专用 API、类型、依赖或链接项没有进入共享 FUSE 实现，继续使用 Runtime 已有的平台接口与 target cfg。
2. 缓存写入使用原 inode 页。局部覆盖先读入未覆盖的原数据，包括仅写打开的 FH；完整覆盖所有已有字节或 EOF 后的新页可直接初始化。缓存保留稀疏零值、脏代次、内存归属和原 OFD／FH，不通过数字 FD 重新定位资源。
3. 普通缓存读取复用相同页面，短 daemon READ 在写回模式下提供空洞零值，不缩小本地 inode。读取保存接受时的调用身份和范围，跨原线程／布局 worker 恢复仍使用原调用者；文件随后增长不会扩大已接受的输出。
4. 本地大小及 mtime／ctime 在写回模式下保持权威，迟到或较新的 GETATTR 都不能用旧 server 值覆盖它们。显式 truncate、chmod／chown 等分别处理被更改的属性；truncate 保留脏前缀并丢弃新 EOF 之后的数据。
5. 回写复用现有 SyncExecutor、原 inode／布局准入和 transport reply receiver。数据 WRITE 携带 `FUSE_WRITE_CACHE` 及原 FH，随后回传修改时间，再推进 fsync／FLUSH；同步写保留位置直到完成。后台错误通过原 inode 错误序列交给对应 OFD，未增加全局算法锁或普通访存屏障。
6. 即使 daemon 不支持 FLUSH，关闭仍排空缓存。缓存仅弱记住可用原 OFD，真正接收的回写任务持有其生命周期，避免缓存与文件自持有循环。direct I/O 绕过缓存，并在需要时先排空脏数据。

行为依据：[Linux FUSE I/O 模式](https://www.kernel.org/doc/html/latest/filesystems/fuse/fuse-io.html)、[v6.12 inode 属性权威处理](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/inode.c)、[v6.12 文件回写／同步实现](https://github.com/torvalds/linux/blob/v6.12/fs/fuse/file.c)。

## 集中验证

| 范围 | 最终结果 |
| --- | --- |
| Apple，VFS 与 Linux 调用库 | 1640 通过、0 失败、2 忽略 |
| Linux ARM64，相同范围 | 1648 通过、0 失败、2 忽略 |
| 产品解释器接入与 FFI | 565 通过、0 失败／中止、7 忽略 |
| 原生 Linux 原版工具 | 3／3 配置通过 |
| Trames 原版工具 × 1／2／4 CPU | 9／9 配置通过 |
| guest 功能／关闭检查 | 81／81 功能、27／27 卸载／停止／退出 |

Rust 结果按身份覆盖：先完整两库，再复验全部 285 个 FUSE 身份，最后只复验 11 个写回缓存身份。新增两项验证脱离线程本地 actor 后的原调用者保留、冷读及并发增长边界；两端最终均通过。其余通过身份直接复用，不相加重复执行次数。收集器检查实际二进制的测试清单与完整摘要，`mac-merged-results.json`、`linux-merged-results.json` 保留来源。

产品实际完整调用覆盖 543 个 conformance 身份（540 通过、3 忽略）和 25 项 FFI，总阶段 135.286 秒。最后窄改仅进入协商缓存的调用身份／版本／输出范围路径，产品普通模式结果直接复用，最终协商缓存由真实工具覆盖；不额外计入前批未变的 55 项 CPU／compute 测试。

两端两库全目标／全功能严格 Clippy 通过，最后窄改后再次检查最终 VFS 全目标／全功能。格式、shell 语法与执行／宿主／项目边界检查通过。Linux 继续复用本任务独立输出目录，没有重启共享 VM、改变其他任务挂载或清理其产物。

## 原版工具与首轮失败

继续使用未修改的 libfuse 3.14.0 `passthrough_ll`／`passthrough`。相同 ARM64 ELF、BusyBox rootfs 和普通客户端先在 Linux 原生验证，再在 guest 三种 CPU 配置运行。工具来源和包哈希沿用前批，最终各输入哈希见工具目录的 `inputs.json`。

客户端在原有 BusyBox、共享／私有映射、截断 SIGBUS、跨页输出、AIO、复制和 8 MiB 内容校验之外，新增仅写局部覆盖、追加 pwrite 的偏移与大小、稀疏空洞、同步写、时间落盘及 io_uring 三请求身份／结果／内容检查。

首轮已经真实激活 writeback，但三种 CPU 的 writeback 配置各有映射读取 EIO、AIO 返回错误两项失败。原因是缓存读取恢复到布局 worker 后仍要求线程本地 actor；改为保留调用接受时的原身份。低层和高层其余六组首轮全部通过，原失败 stdout／stderr 与 daemon trace 保留。

集中修复后的九组全部通过，且三份 `guest-writeback-{1,2,4}-fuse-daemon.stderr` 都包含原工具的激活日志。最终工具阶段 130.806 秒包含九次导入／启动，不是吞吐验收；所有单项与外层期限内结束，没有超时、中止或磁盘停止。

## 原版 gVisor 与完成顺序前提

本轮实际统一执行 aio、iouring、mmap、msync、signalfd、timerfd、fuse、copy_file_range、sendfile、sendfile_socket、splice，共 421 个身份。2 CPU 原始结果为 **410 通过、10 跳过、1 失败**。

唯一失败 `IOUringTest.ThreeREADVSingleEnterTest` 按 CQE 数组位置要求 8／17／26 的提交顺序；实际第二项先完成了长度 26 的请求。原测试源码及原始失败保留，运行时没有为此增加固定完成顺序。[Linux io_uring 文档](https://man7.org/linux/man-pages/man7/io_uring.7.html) 明确允许乱序，调用方应根据 `user_data` 关联结果。

仅该原版身份在 **1 CPU 前提**下复核通过；新增普通客户端对三项完成逐个检查 `user_data`、长度、内容和不重复，在原生三种配置及 guest 九种组合均通过。最终按身份及明确 CPU 前提合并为 **411 通过、10 跳过、0 失败**，覆盖全部 **410／410** 原生通过项。不能把这写成 2 CPU 原始整组无失败，也不能把完成顺序当作运行时承诺。

原生参考复用前批相同 kernel／ELF 的逐项来源；16 份 XML 共 422 次执行均重读核对。`gvisor-merged` 记录各项 runner 与 CPU 来源，严格比较见 `gvisor-comparison/comparison.json`。本轮只覆盖受影响子集，不增加历史 254 组全量的累计通过数。

## 保留门槛

02 仍需匹配 release 产物的 FUSE 吞吐／普通负载测量、完整领域协议／缓存／权限审计，以及最终 HVF／物理 iPad／产品／完整 CI 验收和配对提交推送。03–12 继续按原顺序保留；本检查点不替代整体交付。
