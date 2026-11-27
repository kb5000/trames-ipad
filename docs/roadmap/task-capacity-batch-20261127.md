# 动态任务上限产品交付批次（2026-11-27）

09 接续已发布 Runtime `10dcf23`。共享原子准入资源、proc 动态读写、manifest／CLI／FFI 启动应用已有实现，本轮补产品入口与实际产品验收。按领域完成实现和调用方后统一验证，不新增生产协调／资源服务线程。

- 修复 Mac `config get/set kernel.sysctl.kernel.threads-max` 及 global defaults 的字面 sysctl 键；继续由共享 Kernel schema 校验名称和范围，失败不发布配置。
- 修复 Swift 配置模型对同一字面键的 get/set；TOML、原生 codec、环境管理器保存和下次启动继续共用现有路径。
- 实际 Mac 签名应用的公共 CLI／HVF：默认值、持久配置、无效值、global defaults、重启恢复声明值、动态写入与准入、1/4 CPU、四轮 512 线程、低层最小／最大声明值。
- 实际 iPad 模拟器应用测试宿主：通过 EnvironmentStore 的安装／保存／重新打开／启动路径，验证 1/4 CPU、默认与持久配置、动态准入、zombie 回收和四轮 512 线程。只记录实际执行的模拟器结果，不将真机记作通过。
- `task-limit.c` 只修改独立 Trames 环境内的资源，不修改 Linux 宿主的全局 threads-max。已有共享资源、并发／权限测试和原版 pthread 参考按源码未变范围复用。
- 不重复展开未改动的 CPU、网络、图形、gVisor 全量和性能矩阵；本轮没有更改生产准入热路径。

所有者随后明确：本轮先完成配置交付，可写 per-UID `RLIMIT_NPROC` 单列后续。现有 NPROC 是只读兼容值，并非已实现的独立 per-UID 准入控制；扩展探针的原始失败保留，不记作通过。

状态：约定的全局资源范围实现与集中验收完成。Mac 公共产品 1／4 CPU、四项配置回归、严格 Clippy、实际 iPad 模拟器三个测试身份／八个阶段，以及解释器补测均通过。详见[回执](../validation/task-capacity-product-20261127.md)。按 main 提交／推送，Runtime pin 保持 `10dcf23`。其他领域和文档整理的未提交修改保留，不随本批提交。
