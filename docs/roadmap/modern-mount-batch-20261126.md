# 现代挂载实现批次（2026-11-26）

领域 03 已按约定功能及遗留性能范围收尾，下一领域为 04 cgroup。FUSE 与现代挂载均按所有者接受的解释器／模拟器范围验收；不以未执行的物理设备测试记作通过。没有新增协调／资源服务线程，Apple 专用接口继续通过现有 target cfg 隔离。下列前两批记录保留当时状态，最终结果见第三批。

## 实现与依赖（第一批）

1. 挂载映射资源：导入 `mount_attr.userns_fd`，保留原 namespace FD／映射快照；实现仅 detached tree 可设置、不可重复设置、文件系统支持／所有权／writer 检查、递归预验证和失败不发布。bind、open_tree、namespace clone、投影与卸载后的 OFD 保留原映射。
2. 所有权语义：inode／ACL 的共享缓存继续保存文件系统身份。按实际路径／OFD 的挂载映射转换 stat／statx、DAC／ACL、sticky、chmod／chown、utimens、已支持的创建路径／SGID 继承、硬链接／rename、执行权限／SUID／文件 capability，以及 xattr。未映射 ID 的显示、权限及 EOVERFLOW 分别处理，不通过修改 inode 全局所有权实现视图。独立的 O_TMPFILE 创建支持不属于本批已有调用方，不把它计作通过。
3. 重配置：以原 filesystem/context 能力选择目标，补齐 tmpfs 以外已支持文件系统的合法重配置；统一只读与 writer 准入、bind 别名可见性、挂载局部属性和参数错误回滚。保留 filesystem owner／user namespace 权限，不能把 proc／sysfs 的逻辑 superblock 与底层私有存储混为一体。
4. 调用方与实际工具：普通 syscall、原路径等待／native 回退、AIO／io_uring 及 proc FD 路径使用同一资源；对照原版 mount／findmnt 和 runc／OCI 的 recursive 属性与 ID-mapped bind 配置。

## 统一验证（第一批）

完成上述能实现的内容及必要调用方后，再集中执行格式／编译、Apple 与 Linux 受影响回归、原生同 ELF 对照、原版 gVisor／工具与 OCI 验收，以及普通路径／挂载负载性能。复用未变通过证据，保存每阶段输入、时限、退出值和原始失败；不逐个小改动重跑完整 CPU／网络／图形矩阵。调试使用解释器／模拟器，不等待物理设备。

实现与集中验证见[验证记录](../../runtime/docs/validation/modern-mounts-20261126.md)。最终两宿主按身份合并 1693／1701 通过、各 2 忽略；同 ELF 234 项及原版 runc 三种 OCI 输入均在原生／解释器／HVF 通过。受影响原版 gVisor 为 453 身份、449 通过／4 跳过，覆盖全部 445 个原生通过项；两执行器各三组 libfuse 工具通过。最终 2 CPU 四配对块共 10 次／100 行有效记录：metadata −0.45%、写入＋同步 +0.61%、同步 +0.74%；读取仍有 +3.25%／+3.70%、管道 +2.74%，配对波动和原始失败均保留。实现按 main 配对交付，性能后续与扩展范围继续留在清单。

第一批交付时的后续：已映射 backing layer、扩展 fsopen 类型及独立 O_TMPFILE 当时未支持；这些在下述第二批实现。没有声明全部 Linux filesystem 参数均已支持。

依据：[Linux idmapping 模型](https://docs.kernel.org/filesystems/idmappings.html)、[Linux v6.12 挂载属性实现](https://github.com/torvalds/linux/blob/v6.12/fs/namespace.c)、[OCI 1.2.1 挂载配置](https://github.com/opencontainers/runtime-spec/blob/v1.2.1/config.md)。

配对 Runtime 提交：`7617d2aaead28232ed503e31432bea60a9121012`（main）；iPad／Mac 均固定此身份，保留已验证源码及既有依赖许可提交。

## 第二批：映射 overlay、现代构建上下文及匿名文件

所有者已授权继续本领域：实现映射 lower／upper 的独立 inode 视图及双向 copy-up、ACL／capability、硬链接、原 FD／映射／底层 writer 生命周期与失败回滚；补齐 overlay／devpts／mqueue／cgroup2 的现代构建入口，保留设置参数时接受的目录和原 namespace 资源；实现真正未命名的 O_TMPFILE 及一次性可链接权限。性能改动将文件系统不变资源合并为共享 handle，减少普通读写及 worker 复制中的多重引用计数，派生文件系统仍保留独立参数绑定。不新增协调线程，继续沿用 Apple／Linux 编译隔离。

本批实现与集中验证已完成：Apple／Linux 按身份为 1700／1708 通过、各 2 忽略；同一 342 项新增 ELF、234 项原 ELF 和四种实际 runc OCI 在两执行器通过，原生新增同 ELF／组合 OCI 通过。原版 gVisor 941 身份为 931 通过／10 原始跳过，覆盖全部 926 个原生通过项；最终两执行器各三组 FUSE 工具、各 39 项通过。Linux 全工作区、两宿主受影响严格检查及 CLI／Mac HVF／iOS release 构建通过。没有新增协调线程，原始失败与明确 Linux 模式均保留。

四配对块 10 次／100 行性能对照全部有效。相对上一版，写入＋同步 −0.68%、同步 −0.91%、保持打开读取 +0.20%、重新打开读取 −0.45%、管道 +0.20%；metadata 总体 +2.87%，配对块为 −3.25%／+1.20%／−1.66%／+4.37%。引用计数整理尚未证明消除了上一批读取／管道差距，这部分继续留作性能后续，不把有效样本等同于零回退。详见[第二批验证](../../runtime/docs/validation/modern-mount-overlay-20261126.md)，原始目录 `target/validation-modern-mount-overlay-20261126`。功能修复按 main 配对交付；03 的性能后续仍公开保留。

第二批配对 Runtime：`7f80b14c0a134f1878e058c98ab43aa7749ac5df`（main）。功能范围已交付；性能未收敛项按上述原始对照保留，不重复计入全量台账。

## 第三批：性能收尾

采样确认默认构建没有启用原本被 hostfs 开关限制的挂载遍历快速标记，将这些平台无关的原对象标记统一到默认路径；同时内联共享页区域掩码与小型访问辅助，让编译器处理固定宽度和重复范围计算，保留全部原检查和同步。只涉及三个 Rust 文件，没有扩大实现领域或引入新线程。

两宿主按身份为 Apple 1942／Linux 1952 通过、各 5 原有忽略，默认与 hostfs 配置均通过；原版 gVisor 941 身份为 931 通过／10 原始跳过，覆盖全部 926 个原生通过项。最终两执行器的同一 342＋234 项 ELF、四种 OCI 及各 39 项 FUSE 工具通过；CLI／Mac HVF／iOS 构建与两宿主严格检查通过。

直接对照现代挂载之前的 FUSE `7d4a9711`：四配对块、10 次／100 行样本全部有效。metadata 总体 −31.03%，两种读取 −14.41%／−14.95%，管道 −14.01%，上述四项在每个配对块均改善；写入＋同步 −1.28%、同步 −0.48%。计算和写入存在明显块间波动，原始结果及诊断尺度校正全部公开，不声明每块均更快。约定范围与遗留差距据此收尾，详见[最终验证](../../runtime/docs/validation/modern-mount-closeout-20261126.md)。未支持的 filesystem／index／metacopy 参数仍按明确边界拒绝。

最终收尾 Runtime：`235118768f41f919bb9e38d0109038ce872c15a4`（main），iPad／Mac 配对固定同一身份。按当前约定范围关闭 03，后续进入 04 cgroup。
