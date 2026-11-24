# 现有候选验证检查点：2026-11-24

所有者在完整实现批次中要求先验证已经积累的候选。因此暂停新功能扩展，进行本次构建／测试检查点；原 12 项目标及未完成项保持不变。本记录不能用来宣布整批功能交付。

原始日志和源码清单：`target/validation-existing-20261124-150910`。初始产品 HEAD 为 `310de0813c5384c9d9d1b5c1ea9df7fbbb974b48`，Runtime HEAD 为 `378b5158d8a4b515dc4461d8f258d252cb8145ec`，均含大量未提交候选。`initial-source.json` 保存初始修改／未跟踪文件的逐文件哈希；后续编译修正不沿用该初始指纹。

## 首轮发现与编译修正

- 初始全 workspace Clippy 未通过：perf 使用了未声明的 `parking_lot`；跨模块 truncate 方法可见性不正确；descriptor owner 路径移动原 FD 后再次使用；少量测试仍使用旧接口／缺少 mutable 绑定。集中补齐直接依赖、最小可见性、原 FD 克隆及测试接口。
- 进一步检查发现 nonbyte owner 分支的局部 `target` 遮蔽了 IoWake 参数，已改名，保持传入原通知对象。
- Runtime／产品 Cargo.lock 只新增 VFS 对既有 `parking_lot` 包的依赖边，未升级版本。该锁文件变化触发 guest CLI 来源校验；已通过正式 `build-guest-cli.py` 刷新，未手写伪造产物清单。
- 初始格式检查失败，已集中应用 Runtime rustfmt；之后仍需最终检查。
- 执行／宿主／session 完成边界检查通过。没有新增协调或资源服务线程。

## 环境失败与恢复

第一次 Runtime 测试构建和产品 check 在磁盘不足时退出，未得到行为测试结果。监控包装器也因磁盘耗尽未能写出最终 JSON；原始日志保留，并用单独的 `*-infrastructure.json` 标注，未补造 Cargo 退出码。

磁盘初始约剩 4.8 GiB，两个 `target` 目录合计约 300 GiB，包含缓存、测试环境、镜像及发布产物，不能全部视作缓存。最初仅回收两个 `debug/incremental`。所有者随后要求优先清盘：取得 Cargo 排他锁，保留本轮 7 个失败 target 的可执行文件和 SHA256 后，清理两个 debug 缓存及另外 36 个 Cargo 中间目录，后两轮累计实际释放 95.9 GiB。guest store、Linux CI 虚拟机磁盘、日志、源码和顶层 release 产物保留；清理明细见 `disk-cleanup*.json`。

恢复验证时关闭增量编译、使用编译 jobs=2，原 debug profile 的断言和溢出检查保持开启。磁盘下限为 1.5 GiB，Rust／gVisor 阶段分别设定独立期限。未为清理或测试启动 Linux CI 虚拟机。

## 本轮结果

| 项目 | 当前结果 |
| --- | --- |
| Runtime 执行／宿主／session 边界检查 | 重新通过；产品外层 pin 检查因 Runtime WIP 未提交而未通过，不能视作可发布 |
| guest CLI 来源校验／构建 | 通过 |
| gVisor 构建图准备回归 | 1 项通过 |
| 新增／修改 Python 驱动语法 | 7 文件通过 AST 语法检查；不代表负载运行通过 |
| Runtime 全 workspace／all-features 测试 | 首轮 2108 通过、110 失败、31 忽略；只对 110 个失败解除沙箱复跑，88 通过、22 仍失败，约 8.5 秒。合并为 2196 通过／22 失败／31 忽略；另有共享文件偏移的间歇失败，见下文 |
| 产品 workspace check | 通过，41.6 秒 |
| 产品解释器／FFI／CLI 测试 | 533 通过、0 失败、7 忽略；含解释器 conformance 509、FFI 24，构建及执行合计 108.1 秒；CLI 自身没有单元用例 |
| 本轮源码的 CLI 构建 | 通过，11.1 秒；用于后续原版 gVisor 和端到端检查 |
| 原版 gVisor 受影响 5 组 | futex／rlimits／stat／statfs／fuse 共 166 个身份：163 通过、3 跳过，159.4 秒，无超时；全部 162 个 Linux 参考通过项均通过 |
| threads-max CLI 端到端 | 未通过：指定启动值 1024，直接运行 guest cat 仍读到 65536；非法值 0 正确拒绝 |
| 最终格式 | Runtime 通过；产品仅一处 FFI 换行格式差异，应用 rustfmt 后通过，没有行为修改 |
| Runtime／产品严格 Clippy | 均未通过；未接入 FUSE 缓存的 dead-code、弃用 API 和其他 lint 阻塞保留，未加 allow 掩盖 |
| Mac HVF、完整 254 组 gVisor、实际工具／性能、物理 iPad | 本检查点未执行，不沿用旧结果覆盖新增源码 |

统计校正：初版汇总把内存测试的 4 条子进程小结重复相加。现按每个 Cargo target 的最后一条小结计数；原日志／旧 JSON 保留，修正结果见 `runtime-tests-serial-target-summary.json`。失败清单、退出状态及 gVisor 对照不受影响。

每个阶段保留命令、时限、日志和实际退出状态。行为失败在本轮汇总，不以旧通过结果掩盖，也不因局部通过把剩余功能标为完成。

## 本轮待修问题

下面是失败用例／现象的分组，不把用例数量当作独立 bug 数量。22 个稳定复现用例的完整名字、panic 和断言保存在 `runtime-reproduced-failures.json`。

| 范围 | 失败用例数 | 观察到的现象／下一批依赖 |
| --- | ---: | --- |
| FUSE | 15 | 零长度／同步写仍返回未完成；父目录路径请求出现 LOOKUP／GETATTR 次序差异；迟到 root 回复未更新预期属性；取消／中断回复得到 EINVAL。需一起核对传输接收、属性共享副作用、namespace 续接及旧夹具的协议假设 |
| 内存／cgroup COW | 2 | retained CPU／硬件映射没有返回预期 COW 准入等待；异步页测试触发 `mapping changes require joined commit`。需连同页表提交和 retained View 的权限更新一起处理 |
| 普通路径输出续接 | 1 | readlink／stat 系列测试在准备阶段返回 EFAULT，没有进入预期的原结果 copyout 等待 |
| 挂载／原 OFD | 2 | 目录 atime 未前进；只读重挂载后原 pathname OFD 测试意外 EROFS |
| O_PATH symlink 输出资源 | 1 | 打开 symlink 的 O_PATH／NOFOLLOW 用例得到 ENOENT，尚未进入输出资源生命周期断言 |
| perf 动态控制 | 1 | 已有 descriptor／namespace 别名测试读控制文件得到 EBADF；需检查 worker 路径及原资源保留 |

另有两项不能因单次通过关闭：

- 共享文件偏移：`file_io::tests::dup_fork_and_joined_io_use_one_atomic_offset_transaction` 首轮出现 EAGAIN，解除沙箱后通过，但另外 5 次原可执行文件复测中有 1 次因 `original OFD wait timed out` 失败。该项仍是开放的并发稳定性问题。日志为 `file-offset-stability-*.log`，不能把首轮全部 88 项恢复通过都归因于沙箱。
- threads-max：CLI 启动配置 1024，在无 shell 子进程的 `/bin/cat /proc/sys/kernel/threads-max` 中仍返回 65536；shell 写入 128 后重新读取及 exec 后读取仍为 65536。源码检查发现 `Vfs::path_queries` 从 `Vfs::empty` 构造 worker view，未带入原 `task_limit`；而普通 VFS clone 已共享该资源。下一批应连同 proc 打开、动态写入、既有 worker／FD 及实际任务准入统一修正和验收，不能只补一次读回。有效证据为 `sysctl-cli-direct.log`、`sysctl-cli-block-read.log` 和 `sysctl-cli-live-diagnostic.log`；最后一个仅收集值，进程退出 0 不代表断言通过。

严格静态检查的原诊断保存在 `static-diagnostics.json`：Runtime 包括 `resource_limits` 的弃用 `fetch_update`、FUSE 未接入缓存／actor、perf ring 参数及初始化形式；解释器 conformance 包括未使用的 CpuMemory import、futex 的 `ok().expect()` 和 futex2 参数个数。此次不通过屏蔽 warning 把未接入实现标为完成。

## 原版 gVisor 对照范围

原始 ELF 使用现有 Linux/ARM64 参考产物，驱动校验 executable SHA256；runner 为本次源码刚构建的 `target/debug/trames-run`。全部 166 个身份及原 XML 保存在 `gvisor-affected`，没有抽样替代所选套件。三项跳过与 Linux 参考一致：`RlimitTest.RlimitNProc`、`FstatfsTest.BlocksFree`、`FuseTest.LookupUpdatesInode`。Linux 原先失败的 `FuseTest.CloneFromUnconnectedDeviceFails` 在解释器通过，单独保留，不计入 162 个有效参考通过项。

`gvisor-reference-selection/selection-provenance.json` 保存完整参考 plan／results 的 SHA256，并只按这 166 个身份筛选，未改写原始参考结果。仓库正式 compare 驱动得到 `outcomes_on_native_passed = {passed: 162}`，详见 `gvisor-comparison/comparison.json`。这不是新增源码的全量 14,530 项或 Mac HVF 验收。

所选 futex 是原版传统 futex 套件。新增原版 futex2／rseq guest runfile 尚未执行；本轮单元和状态机结果不代替该端到端门槛。

## 源码与证据使用

`compiled-candidate-source.json` 记录编译修正后、开始本轮复查时的源码。最终产品 rustfmt 仅调整 `crates/trames-ffi/src/session.rs` 中 Kernel.apply 的换行，不因此重跑已通过行为测试；最终记录文件变化另存 `checkpoint-final-source.json`。测试期间没有扩展功能、修复上述行为失败或更新发布 pin。

首次 shell 探针用逐字节 read 未完成 sysctl 整块读回，后改用 cat；一次重复导入已有 rootfs 的诊断被发行版安装保护拒绝。这些无效尝试保留原日志，不算生产行为失败，也不用于证明通过。Runtime 边界脚本最初误从产品目录调用，已在实际 Runtime 目录重新执行并保留通过证据；产品 pin／dirty gate 保持未通过，不绕过。

下一实施批次先汇总以上各领域及其依赖后一起修改，再集中验证受影响项目和失败项。12 项总清单及未完成的 FUSE mmap、IDMAP、Android、perf／真实性能和真机门槛仍然开放；整批未提交、推送或发布。


## 当前问题修复与集中验证（2026-11-24）

本节覆盖上文待修状态，保留首轮失败作为历史证据。新证据目录为 `target/validation-fixes-20261124-161212`。先实现全部已知问题及依赖，再集中验证；未新增协调线程、资源服务线程、轮询总线或全局算法锁。

实现修正：

- worker PathQueries 共享原 TaskLimit；启动值、proc 写入和实际 task Capacity 不再分离。
- stat 输入解析不提前预检输出页；已接受的元数据由 FileCopyout 保留并跨页等待，不重查路径或 FD。
- 异步共享页在 worker 首次变为 resident 后，joined commit 识别原 PageSource，不再把尚未在 root 创建字节别名误当成布局变化。
- 普通文件位置等待先订阅原 OFD readiness 再判断可运行；并发 read 的位置准入竞争不再泄漏成 guest EAGAIN。FUSE 自己的等待仍使用其原位置／协议通知。
- FUSE 取消 GETATTR／SETATTR 后保留属性回复接收对象；本地零长度 IO 和已缓存 ENOSYS 的同步操作直接完成。清页候选补齐保留原 FD 的 FileTarget.page_source 能力接口，错误、短读、取消及重复 load 复用原请求；不据此开放 mmap 或共享写回。
- 集中处理原静态检查阻塞，未增加 dead-code／warnings 屏蔽；保留原原子内存次序。

测试前提修正：ARM64 O_NOFOLLOW 使用仓库常量；fork 后新增 FD 不自动出现于子进程私有表；按 mount 而非共享 superblock 改只读策略时使用 MS_BIND；硬件映射版本测试先声明硬件 Registry；FUSE 夹具遵循有效回复长度、有效属性缓存以及先查 leaf 再检查父目录修改权限的次序。原行为断言保持严格，没有把未知错误变成跳过。

| 项目 | 修复后结果 |
| --- | --- |
| 原 22 个稳定失败 | 全部关闭；包含测试前提与生产实现两类修正 |
| 受影响四库：memory／VFS／Linux／kernel | 合并本轮未受后续修改影响的通过结果及失败项复查，1760 通过、0 失败、5 忽略；VFS 新增 1 个页来源回归 |
| 解释器／FFI／CLI target | 533 通过、0 失败、7 忽略；CLI target 本身没有单元用例 |
| 共享文件偏移间歇问题 | 新可执行文件连续 20 次并发回归全部通过；原失败日志继续保留 |
| threads-max 实际 guest 场景 | 1024 初始化、128／1024 动态读写通过；写入 1 后新 fork 被拒绝。同一固定场景在保留旧 runner 上错误放行，形成前后反例 |
| 严格 Clippy | Runtime 全 workspace／all-targets／all-features 与产品 core／FFI／CLI 全 target 均通过；既有 vendor 警告不等于本仓库 lint 失败 |
| 原版 gVisor 5 组 | 166 个身份，163 通过／3 参考一致跳过，约 159 秒；全部 162 个 Linux 参考通过项通过，无超时 |
| Mac/HVF 接口兼容 | 独立源码快照使用同一 Runtime 候选，全 workspace／all-targets／all-features check 通过；不是 HVF 实际行为矩阵 |
| 普通文件工作负载 | 固定 BusyBox cp／cat／stat／mv／rm／sync，6 次轮换顺序均正确；dev profile 中位数旧 2.614821s、新 2.646939s（+1.23%），样本范围有重叠，不作发布性能结论 |
| 执行边界 | Runtime 执行／宿主／session 完成检查通过 |

工作负载首轮两版均触及默认 10M 指令额度，记录为无效测量；正式对照统一改为 1B 额度，不使用失败耗时计算性能。导入和 APFS clone 不计入负载时间，guest 启动计入；未运行 release、真实包安装／容器吞吐或物理 iPad 性能验收。Mac 实际 HVF、完整 254 组 gVisor 和新增原版 futex2／rseq runfile 的验收门槛仍按总清单保留。

这次关闭的是当前失败批次，完整 12 项范围仍未完成。后续实现继续按领域连同依赖成批推进，再集中验证。


当前已验证的公共 Runtime 基线提交为 `f75b8e34f6e59aa797da9fa072e1ce14a65ba4d3`，同时同步 iPad／Mac 的精确 pin。将要提交的产品 Rust 文件已逐文件核对为实际测试源码；独立发布树及 Mac 原生仓库的 pin／执行边界检查通过。未验证的 Swift 测量场景、allocator／持久性测量驱动和原版工具准备文件保留为 WIP，不据此标记功能完成；未安装或替换现有 Stable／Dev 应用。
