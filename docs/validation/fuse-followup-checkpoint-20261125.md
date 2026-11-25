# FUSE 候选第二次集中检查点（2026-11-25）

本检查点验证 02 FUSE 及必要的 Memory／Kernel／Linux／Session 依赖。结果仍有失败，02 和 01–12 总目标均未关闭，候选未提交、推送或更新 Runtime pin。后续按下列分组集中实施，再统一验证受影响项。

## 源码和证据

- 产品 HEAD：`71cdad49c1ab16480b7a259f7d95a03085fb9a94`。
- Runtime HEAD：`26384a8746933ea636888f57ac49e62d978a6612`；已提交的产品 pin 与之相符。本次验证的是该主线上的未提交候选，不能用 HEAD 代替候选身份。
- Mac 原始证据：`target/validation-fuse-followup-20261125-1500`。
- Linux 原始证据镜像：`target/local-ci/io/validation-fuse-followup-20261125-1500`；独立构建目录为 Linux 的 `/mnt/trames-output/fuse-runtime-target-20261125`。
- `initial-source.json` 保存编译前清单；`test-source.json` 保存编译修正后的产品 12 个、Runtime 261 个修改／未跟踪文件的哈希。后验 `post-tests-source-comparison.json` 确认这些文件和两个 HEAD 在测试期间没有再变化。
- 每个阶段有独立 `.json` 退出回执和 `.log`；用例身份及汇总见 `mac-affected-results.json`、`linux-affected-results.json`、`product-results.json`、`failure-identities.json`。三份结果清单的身份／计数核对均无异常。

编译修正集中处理了 `PathRetry` 的引用传递、原 FUSE 设备测试通过公开 byte 能力接收回复、测试中的 FD 控制调用参数宽度及未使用导入；原子更新使用当前固定工具链的 `try_update`。没有新增协调／服务线程或轮询总线，也没有绕开平台开关。

## 结果和范围

| 检查 | 结果 | 耗时 |
| --- | --- | --- |
| Runtime Mac，全工作区／全部目标／全部功能编译 | 通过 | 38.074 秒 |
| Runtime Linux ARM64，同上 | 通过 | 29.768 秒 |
| Mac，受影响四库单元及集成测试 | 2040 通过、18 失败、5 忽略 | 149.265 秒，含编译 |
| Linux ARM64，同上 | 2049 通过、18 失败、5 忽略 | 171.341 秒，含编译 |
| 产品工作区单元及集成测试，含解释器接入和 FFI | 618 通过、1 失败、1 中止、7 忽略 | 首轮 184.937 秒；遗漏身份隔离补跑 6.555 秒 |
| Mac／Linux 严格 Clippy，五个受影响库 | 均在同一 VFS 指针比较诊断处失败 | 34.796／27.434 秒 |
| Runtime 格式检查 | 通过 | 1.925 秒 |
| Runtime 执行／宿主／依赖边界检查 | 通过 | 0.950 秒 |

四库为 `trames-vfs`、`trames-linux`、`trames-kernel`、`trames-memory`。两端各有七个单元／集成测试目标，失败身份完全相同；额外的 Linux 用例不作为新的 gVisor 身份。文档测试不计入上述数量。此处不是 Runtime 全工作区测试总数，不能与[第一次检查点](fuse-checkpoint-20261125.md)的全工作区数量直接比较。

产品 conformance 首轮因析构期间再次 panic 而中止。保留已完成身份，只对尚未完成的身份逐项隔离执行，每项期限 30 秒；最终覆盖同一产物的全部 543 个身份：538 通过、1 失败、1 中止、3 忽略。与另外八个完整目标合并得到产品总数 618／1／1／7，未把中止后未运行的用例计作通过。测试二进制哈希保存在 `product-results.json`。

严格 Clippy 当前报告 `fuse/mount/entry.rs` 的 raw pointer 比较应使用 `std::ptr::eq`。VFS 在此处阻止依赖它的目标继续检查，不能将本轮当作已经收齐所有后续 lint。已有 vendor 警告保留原始日志。

产品顶层执行边界脚本要求 Runtime 没有未提交修改，因此在 WIP 检查处停止；没有删除或隐藏修改来通过它。已直接运行实际 Runtime 边界检查并通过。正式交付时仍须运行顶层 pin／干净状态检查。产品后续全目标编译阶段曾因磁盘下限提前停止，不能记作通过；本轮产品测试构建仅证明实际覆盖的测试目标可编译。

## 下一批失败清单

共 20 个独立 Rust 失败身份，其中 Runtime 18 个在两个宿主重复出现。以下为同领域修复组，不是逐项测试轮次；断言差异须先区分生产问题与旧夹具假设。

### FUSE 名称、重查和协议生命周期（11 项）

以下省略共同前缀 `fuse::`：

1. `mount::entry::tests::weak_name_index_does_not_retain_original_ofd_inode_or_lookup_lease`：关闭原 FD 后原 OFD 仍被持有。
2. `mount::notify::tests::open_tree_path_and_cloned_leaf_keep_the_original_name_without_opening_another_handle`：原名称／inode 的最终引用未释放。
3. `mount::notify::tests::delete_preserves_a_directory_with_a_held_child_and_can_delete_after_that_child_is_released`：子对象释放时收到 opcode 25，夹具预期不同；须核对 FLUSH／RELEASE 完整顺序。
4. `tests::create_tests::atomic_create_masks_only_in_the_live_callback_and_resamples_on_fallback`：预期 daemon 请求时收到 EAGAIN。
5. `tests::nested_lookup_stat_returns_ino_and_forgets_each_reference`：FORGET 的 node 23／22 顺序不同；需核对父对象保留和实际引用计数，不能额外要求未承诺的固定顺序。
6. `tests::retry_tests::pathname_lookup_stale_is_bounded_and_an_open_keeps_its_original_reserved_slot`。
7. `tests::retry_tests::repeated_lookup_stale_returns_the_second_error_without_another_request`。
8. `tests::retry_tests::nested_stale_rewalks_the_original_full_name_and_forgets_the_failed_parent`。
9. `tests::retry_tests::stat_getattr_stale_has_one_outer_retry_but_descriptor_stat_does_not_rewalk`。
10. `tests::retry_tests::mkdir_callback_stale_rewalks_parent_once_and_does_not_retry_failed_filename_create`。
11. `tests::topology_tests::identical_source_and_target_spellings_remain_separate_accepted_operands`。

第 6–10 项在预期继续等待原 daemon 请求时提前得到 ENOENT。需统一检查 PathRetry 的原 namespace／root／cwd 能力、重新投影后的路径查找、原数字 FD 的显式重查边界和预留槽位，不通过更改 errno 或增加重试次数掩盖问题。第 11 项还需保留同名但独立的两个已接受操作数。

### Linux 原调用及执行资源生命周期（5 项）

- `direct::memory_sync::tests::mapped_sync_cancelled_wait_and_dropped_layout_owner_do_not_form_a_resource_cycle`：已经交给原执行器的 native sync 没有运行，调用数 0／预期 1。必须区别“未提交队列任务被取消”和“已经接受的 native 工作继续完成”；不能为破除异步回引用环而取消后者。
- `direct::tests::shared_anonymous_completion_uses_accepted_type_when_callbacks_replace_the_slot`：原类型错误结果丢失，得到 None／预期 EINVAL。
- `direct::timer_control::tests::set_effect_and_old_snapshot_are_not_replayed_by_output_fault_or_fd_reuse`：计数 2／预期 1，核对原 set 副作用和旧快照的消费边界。
- `direct::uring::owned_tests::asynchronous::registered_descriptor_ring_keeps_original_pin_and_controls_stay_local`：得到 EINVAL／预期成功。
- `direct::uring::owned_tests::solo_accepted_signalfd_keeps_original_reader_across_foreign_capture_and_source_pump`：solo 导入信号权限计数 0／预期 1。

### 原信号帧和事件等待（4 项）

- `signal::frame_copy_tests::sigreturn_permanent_tail_failure_preserves_restored_scalars_and_finishes_the_original_continuation`：frame fault 地址 7600／预期 12288。
- `signal::return_tests::failed_scalar_is_zeroed_later_scalars_still_restore_and_mask_survives`：地址 65536／预期 70224。
- `process::tests::malformed_signal_frame_keeps_restored_registers_before_forcing_sigsegv`：地址 32176／预期 36864。与前两项一起核对原 frame 地址及已恢复 GPR，不能仅为通过夹具回退先前的生产修正。
- `process::seccomp::tests::masked_event_waits_apply_entry_policy_once_before_inputs_and_across_parking`：`cohort stopped before I/O park` 后，析构再次因 poisoned lock panic 导致进程中止；独立补跑同样复现。需核对原 cohort／worker 的实际生存期及接收方，保留每次 syscall 的单次 seccomp 决策。

## 已通过但仍有范围限制的内容

新增的真实执行资源结束／通知等待释放、未运行 queued sync 的失败一次、AIO 迟到完成保留已接受 native 工作、空路径与纯 FD 的重查区别、AT_FDCWD live cwd 回归均已通过。但 mapped sync 的已提交任务仍有上述反例，不能据局部通过宣布所有取消／退出路径闭环。其余新增身份以结果清单为准，未将全部准备回归自动标为通过。

共享实现使用现有平台接口及 Rust 通用能力；Apple 专用入口继续用已有 target cfg／条件依赖隔离。本轮实际在 Apple 和 Linux ARM64 编译了全部 Runtime 目标／功能。该证据不替代实际 Mac/HVF 产品、物理 iPad 或 Linux 宿主应用验收。

## 执行成本和后续门槛

最初同时使用默认 debug 信息编译时触发了 1.5 GiB 磁盘下限，相关阶段已停止并保留退出记录，未产生有效测试结论。随后仅清理两个本任务构建目录中可再生的 Rust `*.rcgu.o` 中间文件，2028 个、约 4.69 GiB；未删除最终产物、VM、用户镜像、日志或其他 Linux 任务的构建目录，清单见 `rust-intermediate-cleanup.json`。

有效测试阶段改为逐个平台编译、单个编译任务、关闭 test／dev debug 信息；每个阶段都有期限及磁盘下限，保留原失败。实际测试执行耗时为秒级，大部分阶段时间用于编译，没有长时间无期限等待。后续复用未受影响的通过结果，集中完成本批失败及调用方／回退后再测。

本轮未运行原版 gVisor、真实 FUSE 工具／程序、性能测量、实际 HVF／物理 iPad 验收，gVisor 台账没有新增通过数。纯 native 且没有 FUSE 暂停的 pathname ESTALE 继续归属 03 的共享路径／现代挂载核对；本批已经进入 FUSE 的路径及跨 native 回退仍留在 02。后续领域顺序和所有交付门槛保持开放。
