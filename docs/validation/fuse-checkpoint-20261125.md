# FUSE 累积候选验证检查点（2026-11-25）

所有者要求先验证已积累候选，暂停功能扩展；同时明确正在实现 Linux 宿主版本，Apple 专用实现沿用现有 target cfg 和平台接口隔离。该约定已加入两个仓库的 AGENTS.md。本检查点不表示 02 FUSE 或完整 12 项目标完成。

开始时产品为 `68e4b7cc`，Runtime 为 `5b12cc9`，配对 pin 相符。原始及结束时源码指纹、逐阶段命令、日志、时限及退出码位于 `target/validation-fuse-20261125-0952`；Linux 日志位于 `target/local-ci/io/validation-fuse-20261125-0952`。最终按身份合并的结果为 `final-results.json`。所有当前候选仍未提交或推送。

收尾核对发现 Runtime 主线由独立工作前进到 `6ebd4e0`；相对开始时只新增／修改 acceptance harness、CI 和文档，未改变本次 Rust 源码。保留这些提交及产品当前 `5b12cc9` pin，不替其他工作的验收改动背书，也不因该文档／测试驱动变更重复本次矩阵。`final-source.json` 记录结束时主线与实际候选指纹；初始指纹不能代替本次编译修正后的源码。

## 已取得的结果

| 范围 | 当前结果 | 证据与边界 |
| --- | --- | --- |
| Runtime 全工作区／全部目标／全部功能编译 | Mac、Linux ARM64 均通过 | 最终检查分别约 33 秒／41 秒；后续 ioctl 测试初始化修正也在两端重新编译并运行 |
| Runtime 全工作区 Rust 测试：Mac | **2452 通过、25 失败、31 忽略** | 非 VFS 为 1398／20／30；VFS 为 1054／5／1 |
| Runtime 全工作区 Rust 测试：Linux ARM64 | **2459 通过、27 失败、15 忽略** | 非 VFS 为 1399／20／14；VFS 为 1060／7／1 |
| 产品工作区／全部目标编译 | 通过 | 解释器、FFI、CLI 对当前 Runtime API 的接入；约 44 秒 |
| 产品工作区 Rust 测试 | **603 通过、15 失败、1 中止、7 忽略** | 内核接入 542 个身份全部有记录，含 523／15／1／3；其他二进制为 80／0／0／4 |
| Runtime 格式与执行边界 | 通过 | 最终 fmt；执行边界脚本连同宿主执行、session completion 检查通过 |
| 受影响库严格 Clippy | 两宿主均未通过 | VFS 首批诊断：Mac 5 条、Linux 7 条，涉及 is_multiple_of、冗余借用、byte string 和 Linux 同类型转换；依赖被中止后的库不能计作检查通过 |

上述均为本地 Rust 用例，**不是新增 gVisor 通过数**。原版 gVisor、两执行器实际 guest、真实 FUSE 程序和性能尚无本检查点的新结果。

汇总方式保留首次失败记录，只用受影响项的后续结果更新相同身份：Mac VFS 首轮 1041／18／1，在允许宿主接口的环境用同一二进制复核原 18 个失败，13 通过、5 失败；Linux VFS 首轮 1059／8／1，ACL 测试身份修正后 HostFS 48／48 通过，仅将原失败的一个身份改为通过。两端其余 Runtime 首轮各 38 个失败，经测试初始化与 ABI 预期修正后各保留 20 个；不把重复运行的通过项累加。

产品内核接入二进制首轮因 worker panic 及清理时再次 panic 触发 SIGABRT，未得到完整汇总。保留首轮已完成及已中止身份，将尚未完成的 361 个身份以独立进程运行，单项限时 30 秒、并发 4，约 6 秒完成；未再触发整套矩阵。最终 542 个身份全部记账，中止项仍记为中止，不视为忽略或通过。

## 本检查点的编译修复

- 补齐 SyncRange 新错误返回、原挂载 identity、原 SharedFd 和状态借用的接线；修复等待输入续接误放在计数收尾函数的问题。
- FileCommand 的 transfer／copy 进度从单线程 RefCell 改为共享的原调用 Arc／Mutex。克隆命令保留同一个进度与租约，既不克隆已消费资源，也不重新接受 FD；锁限定在该调用的状态。该不匹配经 PreparedPath／proc renderer 扩散成大量 Send／Sync 诊断，是本次初期诊断变慢的共同根因。
- 定位过程中用于缩短递归类型诊断的显式 unsafe Send／Sync 及字段断言已全部删除；最终使用 Rust 自动检查共享边界，没有保留绕过字段检查的 marker。
- Binder 的 typed 交接传递原 OFD；FUSE clone 使用原设备 Arc；io_uring 保留原 ring 的强引用完成 ownership 绑定；补齐辅助接口的可见性与原 context 捕获。Session 的复制续接、结果类型及退出通知返回值同步更新。
- 旧 io_uring 注册回归补齐当前 source／wake 参数；修复测试数字类型与 trait 导入。Linux ACL 的“无权限用户”原误用了已获授权的命名 GID，修正身份后 HostFS 48 项通过。
- ioctl 新测试原缺少设备文件系统启用及 PID 7 的进程元数据，补齐统一初始化后，两宿主 ioctl 22 项均通过；不将这些测试初始化错误登记为生产功能缺陷。
- 旧 waitid 测试改为核对实际写出的六个字段并保留 padding；sched_setaffinity 改为先核对输入地址错误，再用有效地址核对权限／PID 错误。依据为 Linux v6.12 的 [waitid 实现](https://github.com/torvalds/linux/blob/v6.12/kernel/exit.c)和 [sched_setaffinity 实现](https://github.com/torvalds/linux/blob/v6.12/kernel/sched/syscalls.c)，两端两项回归均通过。
- 已停止重复类型诊断的进程组；每阶段限时，编译并发限制为 2，产品检查为 1。没有增加生产协调／服务线程。

## 待下一批处理的失败分组

1. **Session 原调用／事件等待接线**：先处理 masked event wait 在 `syscall_dispatch.rs` 的 worker panic 及 run queue 析构再次 panic。其余产品失败涉及 seccomp、FD 控制、原任务／proc 资源、信号和时间观察、deadline、Binder、AIO 退出及旧 mm 退役；集中核对同一原调用能力和完成归属。
2. **AIO／io_uring 原资源及输出续接**：两宿主相同的 20 项非 VFS 失败包括 destroyed-context 的 getevents、读取位置、FD 控制、socket 复制中断、CPU 时钟、共享匿名完成类型、timerfd 旧值，以及 13 项 io_uring ownership、取消、NOWAIT、partial copy、注册／CQ 及原 submitter 能力。
3. **epoll 并发准入／提交**：Mac 的 live registration、nested graph churn 与 TCP promoted interest；Linux 的并发 oneshot 与 nested graph churn。出现原控制调用的 EAGAIN、事件计数不符或后续线程等待超时，保留为真实失败，不用环境问题或重复测试掩盖。
4. **Binder FD 导入失败收尾**：两宿主原 EMFILE 场景返回等待状态，需核对输入效果、失败清理、read-consumed 输出及取消顺序。
5. **FUSE proc FD reopen**：两宿主原重新打开路径返回 EOPNOTSUPP，需接通原 inode／新 OPEN 的实际入口。
6. **Linux HostFS proc anchor**：3 项原目录 stat／open／chdir、父挂载移动及命名空间变更用例返回 ENOENT；需核对 retained parent 与 native ACL 刷新共同使用的 inode 能力。ACL 刷新使用路径 key 而未延续原 parent 是待核实方向，尚不作为已确认根因。
7. **静态检查**：集中处理两宿主 Clippy 诊断，再完成其后因依赖失败而未检查完的受影响库。

以上仍属于 02 FUSE 及它的必要依赖；下一轮先集中完成当前可实现的失败组、调用方和回退，再统一验证。已有通过证据按代码影响范围复用。Apple 专用接口继续由现有 target cfg 隔离，共享代码使用现有原生资源接口；当前双宿主编译通过不替代实际 Apple SDK、HVF 或物理 iPad 验收。

## 最终失败身份

以下是本检查点按身份合并后的未通过项；完整命令、原始输出与修正身份见 `final-results.json`。

### Runtime 非 VFS：两宿主相同的 20 项

- `direct::aio::tests::parked_getevents_retains_destroyed_context_across_numeric_reuse`
- `direct::file::copyout_tests::proc_cgroup_and_mqueue_reads_commit_only_copied_bytes_and_wake_original_position_waiters`
- `direct::file::tests::fd_control_and_epoll_errors_complete_after_their_original_parameter_preludes`
- `direct::ip_message::tests::interrupted_socket_copy_returns_the_slot_and_resumes_the_original_socket_after_fd_reuse`
- `direct::metadata::tests::cpu_clock_keeps_original_observation_and_copied_prefix_through_page_wait`
- `direct::tests::shared_anonymous_completion_uses_accepted_type_when_callbacks_replace_the_slot`
- `direct::timer_control::tests::set_effect_and_old_snapshot_are_not_replayed_by_output_fault_or_fd_reuse`
- `direct::uring::maintenance_tests::byte_cqe_waits_for_original_copy_even_with_nowait_and_fd_reuse`
- `direct::uring::maintenance_tests::probe_output_retains_its_completed_prefix_across_a_page_wait`
- `direct::uring::owned_tests::accepted_shared_file_releases_at_completion_or_cancel_after_handoff_and_reuse`
- `direct::uring::owned_tests::asynchronous::invalid_selected_data_waits_for_readiness_without_consuming_pool_or_bytes`
- `direct::uring::owned_tests::linked_eagain_and_drain_survive_owner_handoff_without_early_effects`
- `direct::uring::owned_tests::shared_pending_pipe_keeps_original_across_handoff_and_releases_at_completion`
- `direct::uring::subsystem_tests::capability_less_embedding_defers_before_import_and_original_caller_can_complete`
- `direct::uring::subsystem_tests::corrupt_cq_during_accepted_enter_returns_error_without_replay_or_owner_pump`
- `direct::uring::subsystem_tests::native_device_scalar_vector_and_positioned_errors_share_the_original_abi`
- `direct::uring::subsystem_tests::queued_disk_sync_keeps_the_actual_ofd_after_ring_close_until_host_work_finishes`
- `direct::uring::tests::local_uring_handoff_retains_closed_ofd_and_never_resubmits_batch`
- `direct::uring::tests::local_uring_parked_enter_is_adopted_without_resubmitting_on_next_epoch`
- `direct::uring::subsystem_tests::mapped_layout_sqe_uses_source_resource_and_keeps_accepted_work_after_cancellation`

### Mac VFS：5 项

- `binder::tests::failed_fd_import_reclaims_async_slot_and_signal_cancellation_keeps_write_effects`
- `direct_io::tests::concurrent_live_epoll_registration_has_one_winner_and_one_oneshot`
- `epoll::graph_tests::nested_graph_churn_and_flat_mod_do_not_lock_recursive_watch_state`
- `fuse::tests::proc_walk_tests::proc_fd_reopen_retains_the_fuse_inode_and_gets_a_fresh_handle`
- `network_io::tests::promoted_tcp_interest_preserves_oneshot_aliases_and_worker_control`

### Linux ARM64 VFS：7 项

- `binder::tests::failed_fd_import_reclaims_async_slot_and_signal_cancellation_keeps_write_effects`
- `direct_io::tests::concurrent_epoll_oneshot_has_only_one_delivery_and_rotates_duplicate_watches`
- `epoll::graph_tests::nested_graph_churn_and_flat_mod_do_not_lock_recursive_watch_state`
- `fuse::tests::native_walk_tests::namespace_mutations_keep_the_proc_host_parent_after_fd_reuse_and_rename`
- `fuse::tests::native_walk_tests::proc_host_anchor_follows_mount_identity_after_its_parent_moves`
- `fuse::tests::native_walk_tests::proc_host_directory_components_keep_the_original_anchor_for_stat_open_and_chdir`
- `fuse::tests::proc_walk_tests::proc_fd_reopen_retains_the_fuse_inode_and_gets_a_fresh_handle`

### 产品接入：15 失败、1 中止

- `process::compat::tests::file_size_limit_changes_are_atomic_and_hard_limit_is_privileged`：failed
- `process::dormant_tests::enrolled_deadlines_do_not_poll_stale_projections_or_hide_legacy_waits`：failed
- `process::ioctl_tests::binder_call_reply_parks_original_workers_with_one_or_many_cpu_slots`：failed
- `process::proc_resources_tests::complete_task_proc_memory_family_and_uring_use_original_resources_without_host_statistics`：failed
- `process::seccomp::tests::entropy_completes_on_sources_with_original_memory_and_one_policy_decision`：failed
- `process::seccomp::tests::masked_event_waits_apply_entry_policy_once_before_inputs_and_across_parking`：aborted
- `process::seccomp::tests::worker_fd_status_controls_obey_seccomp_before_atomic_effects`：failed
- `process::proc::tests::actual_live_proc_image_svcs_pause_only_the_target_mm_and_use_no_owner_service`：failed
- `process::socket_wait_tests::joined_sigsuspend_preserves_temporary_handler_mask_and_restores_original_mask`：failed
- `process::tests::worker_poll_deadlines_do_not_force_unrelated_cpus_to_checkpoint`：failed
- `process::time_tests::time_observations_complete_on_source_with_faults_nulls_overlap_and_filter`：failed
- `retired_memory::close_tests::original_close_survives_driver_replacement_and_mm_destruction`：failed
- `retired_memory::tests::cancelled_mapping_on_a_shared_old_mm_uses_weak_custody_and_original_idle_wake`：failed
- `workers::tests::aio_exit_tests::fatal_aio_wait_releases_the_only_slot_without_requeueing_exit_or_discarding_original_request`：failed
- `workers::tests::masked_event_wait_signal_and_sigreturn_need_no_owner_service`：failed
- `workers::tests::signal_delivery_tests::bad_sigreturn_forces_original_fault_info_and_returns_zero_at_the_syscall_boundary`：failed
