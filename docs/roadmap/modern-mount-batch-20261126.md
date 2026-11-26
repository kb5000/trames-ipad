# 现代挂载实现批次（2026-11-26）

当前领域为实施清单 03。FUSE 已按所有者接受的解释器／模拟器范围收尾；不以未执行的物理设备测试记作通过。本批没有新增协调／资源服务线程，Apple 专用接口继续通过现有 target cfg 隔离。

## 实现与依赖

1. 挂载映射资源：导入 `mount_attr.userns_fd`，保留原 namespace FD／映射快照；实现仅 detached tree 可设置、不可重复设置、文件系统支持／所有权／writer 检查、递归预验证和失败不发布。bind、open_tree、namespace clone、投影与卸载后的 OFD 保留原映射。
2. 所有权语义：inode／ACL 的共享缓存继续保存文件系统身份。按实际路径／OFD 的挂载映射转换 stat／statx、DAC／ACL、sticky、chmod／chown、utimens、已支持的创建路径／SGID 继承、硬链接／rename、执行权限／SUID／文件 capability，以及 xattr。未映射 ID 的显示、权限及 EOVERFLOW 分别处理，不通过修改 inode 全局所有权实现视图。独立的 O_TMPFILE 创建支持不属于本批已有调用方，不把它计作通过。
3. 重配置：以原 filesystem/context 能力选择目标，补齐 tmpfs 以外已支持文件系统的合法重配置；统一只读与 writer 准入、bind 别名可见性、挂载局部属性和参数错误回滚。保留 filesystem owner／user namespace 权限，不能把 proc／sysfs 的逻辑 superblock 与底层私有存储混为一体。
4. 调用方与实际工具：普通 syscall、原路径等待／native 回退、AIO／io_uring 及 proc FD 路径使用同一资源；对照原版 mount／findmnt 和 runc／OCI 的 recursive 属性与 ID-mapped bind 配置。

## 统一验证

完成上述能实现的内容及必要调用方后，再集中执行格式／编译、Apple 与 Linux 受影响回归、原生同 ELF 对照、原版 gVisor／工具与 OCI 验收，以及普通路径／挂载负载性能。复用未变通过证据，保存每阶段输入、时限、退出值和原始失败；不逐个小改动重跑完整 CPU／网络／图形矩阵。调试使用解释器／模拟器，不等待物理设备。

实现与集中验证见[验证记录](../../runtime/docs/validation/modern-mounts-20261126.md)。最终两宿主按身份合并 1693／1701 通过、各 2 忽略；同 ELF 234 项及原版 runc 三种 OCI 输入均在原生／解释器／HVF 通过。受影响原版 gVisor 为 453 身份、449 通过／4 跳过，覆盖全部 445 个原生通过项；两执行器各三组 libfuse 工具通过。最终 2 CPU 四配对块共 10 次／100 行有效记录：metadata −0.45%、写入＋同步 +0.61%、同步 +0.74%；读取仍有 +3.25%／+3.70%、管道 +2.74%，配对波动和原始失败均保留。实现按 main 配对交付，性能后续与扩展范围继续留在清单。

明确后续：overlay 自身可映射，已映射 backing layer 的 copy-up 仍受共享 upper inode 设计限制，当前返回 EOPNOTSUPP；未实现的 fsopen 类型／参数及独立 O_TMPFILE 不计作已支持。03 的延伸范围不因本批工具通过自动关闭。

依据：[Linux idmapping 模型](https://docs.kernel.org/filesystems/idmappings.html)、[Linux v6.12 挂载属性实现](https://github.com/torvalds/linux/blob/v6.12/fs/namespace.c)、[OCI 1.2.1 挂载配置](https://github.com/opencontainers/runtime-spec/blob/v1.2.1/config.md)。

配对 Runtime 提交：`7617d2aaead28232ed503e31432bea60a9121012`（main）；iPad／Mac 均固定此身份，保留已验证源码及既有依赖许可提交。
