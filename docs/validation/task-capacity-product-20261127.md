# 动态任务上限：产品配置交付（2026-11-27）

09 的全局 `threads-max` 配置与产品验收完成。Mac 的实际签名应用／HVF 与 iPad 模拟器应用均在 1／4 CPU 通过；支持范围和机器可读证据见 [JSON 回执](task-capacity-product-20261127.json)。所有者明确将可写 per-UID `RLIMIT_NPROC` 单列后续，没有将该功能记作通过。

## 实现

- Mac 公共 `config get/set kernel.sysctl.kernel.threads-max` 将 sysctl 名称作为字面 map key，global defaults 同样可读写。写入沿用共享 Kernel 的名称／范围校验，失败不发布配置；补 CLI 与低层启动帮助。
- Swift 配置模型的 get/set 保留同一字面名称。原 TOML codec、EnvironmentStore 保存／重新打开／启动路径继续共用现有资源与校验。
- 本轮没有改动共享准入热路径，没有新增生产协调／定时／资源服务线程；Apple 专用路径仍在现有产品边界内。Runtime pin 保持已发布的 `10dcf23`，不为记录或产品入口修正制造新 Runtime 提交。

使用方法见[动态任务上限](../guides/task-capacity.md)。

## 实际产品验收

| 路径 | 结果 |
| --- | --- |
| 正常构建并签名的 Mac 应用 | 公共 CLI／HVF 两种 CPU 配置、74 条命令完成；包含 10 次预期拒绝的无效配置尝试 |
| Mac 配置单元与严格检查 | 4／4 通过；all-target／all-feature Clippy `-D warnings` 通过 |
| iPad 模拟器实际应用 | 3 个测试身份通过，零跳过：codec 首轮通过后复用，产品两项最终重跑通过 |
| iPad 模拟器终端证据 | 8 个附件逐项核对，两种 CPU 各覆盖 default／configured／restart／capacity |
| 未变解释器产物补测 | 同一动态准入 ELF 在 1／4 CPU 通过，实际 baseline=1、最终值=65 |

Mac 测试使用正式 `scripts/build-mac.sh` 生成的 `Trames Dev.app`，实际二进制 SHA-256 为 `36c64d7b94d227a9f26b6667ea53c9f5eb2b7a081171424e2ee0b0ad499fd31f`。公共 CLI 验证了默认 65,536、持久声明 64、运行时变为 65 但宿主 TOML 保持 64、停止／启动恢复 64、全局默认 42 的新环境继承，以及低层 1／1,073,741,823 的首任务启动。无效名称、0、负数、小数及超范围写入均拒绝且不改变原文件。

iPad 使用 iPad Pro 11-inch (M5) 模拟器、iOS 27.0。测试通过应用的 EnvironmentStore 完成独立安装、保存配置、重新打开和正常多终端启动；guest 自己检查 CPU 数。两种配置都验证了默认 65,536、配置 64、非特权调用者不能写入已由 root 打开的 proc FD、降低上限不撤销现有任务、耗尽时 fork 被拒绝、升高后可用、未回收 zombie 占名额、回收后复用，以及重启恢复持久值。

Mac 与模拟器均在上限 600 下，四轮各创建并 join 512 个并发线程。这里的 512 是工作负载线程数，环境的 init／shell 另计；模拟器动态探针实际观察到 baseline=3。只执行实际的模拟器验收，未将物理 iPad 计作通过。

测试使用同一 ARM64 静态 ELF，来自 [task-limit.c](../../tests/fixtures/task-limit.c) 和原有 [task-capacity.c](../../tests/fixtures/task-capacity.c)。官方 Alpine 3.22.6 根镜像 SHA-256 为 `821565fa8f3953eefd12497b166b4b50add2f7c57fb312e75862f5867e06fefe`。Mac 驱动只为独立验收注册表添加两个 ELF，原始镜像保持不变。所有 guest sysctl 写入均在独立 Trames 会话内，没有修改 Linux 宿主的全局 threads-max。

## 反证与过程记录

- 保留的修正前 HVF 产物对同一注册表副本执行字面键 get/set，分别报 `unknown configuration field/table`；当前实际产品的对应操作通过。
- 扩展探针发现 NPROC 修改尚不支持，1／4 CPU 均保留原始失败。源码确认资源 6 仍走 FixedLimits，并未实现独立 per-UID 准入；所有者明确选择本轮先完成产品配置，将该功能单列后续。
- 模拟器首轮 codec 通过，两个产品测试已输出真实的 `TASK_LIMIT_HOST_DONE:0`，但测试未正确处理终端 CRLF，空等至各自期限。修正测试的标记匹配后，仅重跑两项失败身份，最终 2／2 通过、零跳过，八个实际终端附件全部通过语义核对。
- 并行 FFI 内存修改触发了源码指纹检查。本轮使用已提交产品 `2e157d22` 加本轮修改的隔离检出；复用来源一致的 Rust 静态库，指纹校验正常执行。没有将其他未提交修改混入验收或提交。
- 清理了已结束验收的可重建中间对象以恢复磁盘空间；基线、最终产物、原始测试输出及回执保留。该清理不触发未变代码的完整重测。
- XCTest 保留了同步冷安装等待的 QoS 诊断；Xcode 附加诊断收集还报过缺少默认 `simctl` 路径。实际测试结果、零跳过计数与八个终端附件完整，不据这些诊断改写通过结果。

原始证据位于产品工作区 `target/validation-task-capacity-20261127`，包含每阶段独立期限、首轮失败、最终 `.xcresult`、导出附件、签名产物／源码指纹及 [公共 Mac 验收驱动](../../tests/integration/task-capacity-product.py)。共享 Runtime 与 Linux 路径的源码未变，按既有检查点复用通过证据；未新增 gVisor 用例身份，不重复展开 CPU、网络、图形和性能矩阵。
