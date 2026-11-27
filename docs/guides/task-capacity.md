# 动态任务上限

Linux 与 Android 环境默认允许 65,536 个任务。这里按 task 计数，包含线程；不是宿主线程池大小，也不预分配同等数量的对象、内存或宿主线程。

## 持久启动配置

环境 TOML 使用共享字段：

```toml
[kernel.sysctl]
"kernel.threads-max" = 65536
```

允许值为 `1` 到 `1,073,741,823`。目前只接受这个 sysctl 名称；无效名称、范围或类型会报错。过低的值可能无法启动需要多个任务的 shell／init。

Mac 公共 CLI 的 sysctl 名称作为完整字面键处理：

```sh
trames -e work config set kernel.sysctl.kernel.threads-max 65536
trames -e work config get kernel.sysctl.kernel.threads-max
trames env stop work
trames env start work
```

全局默认声明可用于新建环境，已有独立配置保留自己的值：

```sh
trames config --global set defaults.kernel.sysctl.kernel.threads-max 65536
```

iPad 在环境详情的 **概览 → 编辑 TOML** 中添加同一声明，保存配置后重启环境。Swift 配置模型的 `kernel.sysctl.kernel.threads-max` 路径同样保留字面 sysctl 名称。公共 CLI 和应用均在首个任务准入前应用配置；嵌入方原有初始上限仍可用，显式 TOML 声明优先。

## 运行时调整

初始 user namespace 的 root 可直接访问：

```sh
cat /proc/sys/kernel/threads-max
printf '600\n' > /proc/sys/kernel/threads-max
```

proc 文件和任务准入使用同一个原子资源。调低值不终止已存在的任务；达到上限后，新建任务返回 `EAGAIN`。调高值立即允许后续准入。构造中的任务和未回收的 zombie 仍占名额，放弃构造或完成回收只退款一次。proc 状态刷新不会覆盖运行中的值。

guest 写入是当前会话的修改，不回写宿主 TOML。停止并重新启动后，重新应用持久声明。不同环境使用独立资源；cgroup `pids.max` 继续单独生效。

当前 `RLIMIT_NPROC` 仍是固定只读兼容值，读数沿用会话任务上限，未实现可写的 per-UID 准入控制。所有者明确将这部分单列后续，不能将它描述为已经可用的独立限制。

实际 Mac/HVF、iPad 模拟器与同 ELF 验收结果见[本轮记录](../validation/task-capacity-product-20261127.md)。未运行的物理设备测试不计作通过。
