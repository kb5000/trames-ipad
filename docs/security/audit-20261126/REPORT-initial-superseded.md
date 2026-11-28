> 已被 [逐项核验后的最终报告](REPORT.md) 替代。初步风险中关于历史CLI源码和FFmpeg的推测以最终报告为准。

# Trames 五仓依赖与许可证审查 — 2026-11-26

本报告是当前源码、锁文件、构建配方及选定本地产物的静态审查；不修改 LICENSE、CLA、源码或分支规则，不构成已取得商业发行／App Store 法律许可的确认。没有构建、运行产品或重跑 CPU／设备验收。

## 主要结论

- 五仓主 Cargo.lock 并集为 **147 个第三方 Rust 包版本／来源组合**，全部取得包声明许可；加上3个辅助锁文件，总并集为 **186**。没有在主锁文件中发现必须选择 GPL/AGPL/商业专有许可的第三方 crate；多数有宽松分支，但这不表示无 notice/专利条款，也不替代源码核查。
- Swift 共 **11 条组件记录**：Package.resolved 8条、2个本地fork、Xcode独立固定的SwiftTerm。Rust表无法覆盖这些或原生库。
- 审查额外收集 **78个本地暂存Alpine包**、**11个暂存ELF的DT_NEEDED和许可字符串**、**9项Vulkan原生源码输入**、Android输入清单、4项固定Python构建工具、5个GitHub Actions项目（7种引用），以及32份notice证据。
- 最先处理：**Mac源码归档遗漏CLI子模块的自动闭包、glibc来源/转写的权属边界、FFmpeg准确源码与分发义务、正式iPad商业/商店发行的许可路径**。
- 没有证据支持“项目完全不能商业化”；也没有证据支持“只加CLA就能把全部依赖闭源再授权”。GPL本身允许收费和商业使用，额外商业再授权只覆盖实际拥有的权利。

## 范围、版本与证据等级

| Repo | Audited working checkout HEAD | Main-lock third-party entries |
| --- | --- | --- |
| ipad | `0192b6227be94ba2a569084b4510875c72bbd0b9` | 132 |
| runtime | `00fb019c9a3e3b3f8663d821c1e5e66b41e7f0c5` | 132 |
| mac | `808a62b271e6cdf0a8f2adf3da3b6bbb8018e5e2` | 132 |
| linux | `d1ac3875de30347d4ab7c2c4edee89334833c431` | 136 |
| cli | `ac88faf29786196b6c933824ce419147f4cd888f` | 12 |

完整工作树状态和子模块状态见 [snapshots.json](snapshots.json)。审计开始时 iPad 工作区包含独立的内存/存储实验；子模块工作副本可能比产品已提交pin新。Cargo metadata描述**当前工作副本**，不是某个历史IPA的精确依赖。当前源码commit、产品gitlink和运行进程版本不能互换。

证据分级：**已确认**表示在文件/脚本/本地产物中复现；**待核**表示缺少准确发行物、权属或法律解释。本文不把待核项写成已发生的侵权。许可证元数据不是文件级法律全文扫描；没有进行恶意代码/CVE审计。

## 风险清单

### L01 · 高 · 源码打包没有自动包含 cli 子模块

**状态：** 已确认脚本缺口；实际发行包待核。**涉及：** mac。

scripts/package-mac.py 仅 git archive 产品与 runtime；git archive 不展开子模块，cli 的 GPL 源码不会由这两个 archive 自动带入。--corresponding-source 可额外补充，故不能据此断言所有已发布 ZIP 都违规。

**建议：** 逐个核对发行包 Sources 与 BUILD.json；显式提供匹配 cli revision 的源码及完整依赖/构建输入。

### L02 · 高 · glibc 原始源码、Rust 转写及 guest 衍生库需要分别审查

**状态：** 待权属复核。**涉及：** ipad, runtime。

runtime/crates/trames-native/upstream/glibc-2.41 含 LGPL-2.1-or-later 文件和 COPYING.LIB；provenance.json 写明 integer grammar translated to Rust；src/glibc.rs 自身标 GPL。build.rs 未直接编译这些 glibc C 文件，因此不能推断整个 App 静态链接 glibc。独立实现行为也不自动构成衍生作品。

**建议：** 针对转写的表达/代码来源做定向对照；分别确定原始 vendored 源、宿主实现、guest libc/补丁的许可和分发方式，勿直接把全部代码按自有商业许可处理。

### L03 · 高 · FFmpeg 修改与源码/替换义务

**状态：** 分发闭环待核；本地产物证据已核。**涉及：** ipad, runtime, mac。

7.1.1 脚本启用 shared，实际暂存 libavcodec/libavutil 自报 LGPL 2.1+，参数未见 GPL/nonfree。prepare-ios-video.py 复制 LGPL 文本和补丁，但没有单独证明最终 Release 的准确源码、配置/补丁及库替换途径齐备。runtime patch 无独立 SPDX 不能单凭顶层 COPYING 判定所有新增代码许可。

**建议：** 将确切源码、补丁、完整 configure 与 toolchain 记录绑定到四个 .so；确认补丁授权、允许的替换/逆向修改方式、About/EULA 提示。检查每个实际发行版本而非只看当前脚本。

### L04 · 高 · CLA 不能清理旧贡献和第三方权利，也不能包办 App Store

**状态：** 发行前法律/权属门槛。**涉及：** ipad, runtime。

五仓库当前均 GPL-3.0-or-later；CLA 草案尚未生效。glibc/FFmpeg 等并不因签署新 CLA 失去各自许可义务。App Store/签名/分发条款应与实际 GPL/LGPL 组件和用户修改权逐项对照。

**建议：** 区分自有代码商业再授权和第三方合规；做历史贡献者权属表与交付闭包。不要宣传为商业许可可替代所有第三方许可；尚不能给出商店发行已获许可的结论。

### L05 · 中 · Wayland notices 与当前 Cargo.lock 不一致

**状态：** 已确认校验失败；影响有限。**涉及：** ipad。

prepare-wayland-notices.py --check 失败；审计副本生成 diff 显示 windows-sys 0.60.2/旧 windows-targets 转为 0.61.2。主要是 Windows-only 闭包，不能直接证明 iPad 运行时漏声明。

**建议：** 按实际发行 target/features 刷新并验证 notices；保留单独的构建/其他目标附录，避免用全锁文件替代实际交付清单。

### L06 · 中 · 缺少五仓统一的实际交付 license 清单与检查

**状态：** 文档覆盖缺口/待产物验证。**涉及：** ipad, runtime, mac, linux, cli。

现有 dependencies.md 是历史分段记录，未覆盖本轮 147 个主锁文件包的完整当前表；iPad 已有多份 Swift/Wayland/MoltenVK notices，但没有从这些证据得出所有实际链接 crate 都被聚合到 About/发行包。Mac 本地 bundle Resources 未见统一 notices 目录，包外 source 仍可能含许可证。

**建议：** 从实际 binary/target/features 生成 THIRD_PARTY_NOTICES 与 source manifest，校验每个闭包；MIT/BSD/Apache/Unicode 等声明不得只用 GPL 根 LICENSE 代替。

### L07 · 中 · SPDX AND 与多许可分支不能简化为 MIT

**状态：** 已确认清单层面的多许可条件。**涉及：** ipad, runtime, mac, linux。

encoding_rs 为 (Apache-2.0 OR MIT) AND BSD-3-Clause；unicode-ident 为 (MIT OR Apache-2.0) AND Unicode-3.0。r-efi 有可选 LGPL，但也可选 MIT/Apache，并非必须 LGPL。

**建议：** 逐包记录选择的 OR 分支，保留 AND 附加许可；以实际目标筛除无关 Windows/WASI/EFI 包。

### L08 · 中 · 许可证文本补充要有来源证据

**状态：** 存在声明但部分包未附顶层许可文件。**涉及：** ipad, runtime, mac, linux。

crc-catalog、defmt shim/parser、r-efi、xim 子包部分 registry archives 没有顶层 LICENSE。xim 的上游 MIT 已由现有脚本使用固定哈希 COPYING.xim-rs 补齐，不应再记为完全无许可。

**建议：** 对其它缺文本项定位同 revision 上游文件，保留 copyright/NOTICE；不是把声明缺文本误判为无授权。

### L09 · 中 · rootfs/APK/Android 镜像是独立多许可证集合

**状态：** 已发现暂存包；最终分发范围不同。**涉及：** ipad, mac, linux。

当前暂存图形资源中解析到 78 个 Alpine 包；含 GPL-2.0-only busybox、LGPL 库和字体。package-ios.py 明确排除 GuestVulkan/wayland-acceptance，因此这78项不能全部算进生产IPA。Android/AOSP 镜像也非单一Apache许可。

**建议：** 分别为内置资源、测试包、随包rootfs、首次下载镜像维护包级SBOM/源代码义务；不要混淆运行外部guest软件与宿主链接，不要声称GPL2 guest自动使GPL3宿主不兼容。

### L10 · 中 · Mesa/Swift crypto/系统库不能只看项目总标签

**状态：** 组件级条款/使用边界待核。**涉及：** ipad, runtime, mac, linux。

Mesa 存在 MIT/SGI-B/BSL 等文件级许可；Swift crypto 带 BoringSSL 及独立 notices；libgcc/libstdc++ 的 runtime exception、LLVM exception、字体和X11混合许可须看具体文件。

**建议：** 保留组件内 notice 与适用例外；系统SDK/动态宿主库、guest重分发库、静态链接库分别判断。

### L11 · 中 · CI mutable refs 与 Python 间接依赖未全锁

**状态：** 构建依赖可复现与供应链风险。**涉及：** ipad, runtime, mac, linux, cli。

GitHub Actions 使用 @v4 和 dtolnay/rust-toolchain@master；上游5项目均MIT，但可变ref不是不可变证据。4个图形工具虽固定版本，Mako requires MarkupSafe>=2.0 未固定实际版本。

**建议：** 保存实际执行的 action SHA 和 build工具完整锁文件/哈希；这属于供应链与审计可追踪性问题，不是已确认许可证冲突。

### L12 · 低至中 · CoreMark 与多媒体专利/商标独立于代码许可

**状态：** 用途取决于发布内容。**涉及：** ipad, mac。

CoreMark 源码含 Apache-2.0，但 LICENSE.md 同时有商标可接受使用条件。FFmpeg 的编解码器专利风险不被 LGPL/MIT 清零。

**建议：** 对外 CoreMark 成绩保留原始算法/参数和可比性；正式视频商业发行按地区、编解码路径另行判断专利，不把软件许可当专利许可。

## 非 Cargo 组件与许可

| 组件 | 版本/来源 | 许可与审查要点 | 证据 |
| --- | --- | --- | --- |
| musl / Arm routines | 1.2.5 | MIT及原文件copyright；本地改造/包装单独保留 | runtime/crates/trames-native/upstream/musl-1.2.5/COPYRIGHT；scripts/build-musl-hybrid.sh |
| Bionic / gdtoa | Android13 + Lucent gdtoa | Apache/BSD式/MIT式等混合；原文件与NOTICE均需保留 | runtime/crates/trames-native/upstream/bionic-android13/NOTICE.txt |
| glibc reference/translation | 2.41及Debian2.41-12+deb13u4 | 原C LGPL-2.1+；不可按纯自有Rust一句话清掉，转写衍生性待核 | upstream/glibc-2.41/COPYING.LIB、provenance.json、src/glibc.rs |
| glibc guest derivative | 同上 | 原guest库/补丁/尾部payload分别确定权利；App仅打包payload不等于原库也进入IPA | scripts/build-glibc-hybrid.py；scripts/build-ios.sh |
| FFmpeg | 7.1.1 | 本地配置与字串支持LGPL2.1+，仍需准确源码/修改/替换闭环 | staged-elf-dependencies.json；runtime/scripts/build-video-ffmpeg.sh |
| Mesa / Zink / EGL | 25.1.9 | 多数MIT，含SGI-B/BSL等；文件级条款与构建选项决定闭包 | scripts/build-zink-egl.py；staged-apk-packages.json |
| Xwayland | 24.1.9 | Xorg的MIT/X11及文件级版权；项目patch另计 | scripts/build-xwayland-native.py |
| Wayland protocol/keymap | Cargo及XML固定版本 | MIT/X11式协议与键盘数据copyright；保留生成数据来源 | scripts/prepare-wayland-notices.py；notice-evidence.json |
| MoltenVK | 1.4.2，固定commit | Apache-2.0，修改记录/NOTICE/源闭包 | vulkan-upstream.json；prepare-vulkan.py |
| SPIRV-Cross / SPIRV-Tools | 固定commit | Apache-2.0，原文在本地source cache验证 | vulkan-upstream.json |
| Vulkan-Headers / SPIRV-Headers | 多个固定commit | 按各LICENSE中的文件类别使用MIT/Apache等分支 | vulkan-upstream.json |
| Volk | 固定commit | MIT | vulkan-upstream.json |
| Vulkan-Tools | 固定commit | Apache-2.0；主要属源码/工具闭包，不自动全链接进App | vulkan-upstream.json |
| cereal | 固定commit | BSD-3-Clause | vulkan-upstream.json |
| SwiftNIO / crypto / collections等 | 见11项Swift表 | Apple Apache-2.0及上游例外/NOTICE；crypto的BoringSSL另有组件条款 | swift-dependencies.json；apps/ios/Trames/Notices |
| Citadel / SwiftTerm / BigInt | 0.12.1 / 1.20.0 / 5.7.0 | MIT；Citadel bcrypt保留BSD源声明 | swift-dependencies.json；vendor/README.md |
| CoreMark | 原EEMBC固定commit | Apache-2.0软件许可 + CoreMark商标使用约定；基准/测试用途 | apps/ios/NativeCoreMark/LICENSE.md、README.md |
| Android NDK / AOSP / libc++ / VNDK | r27d/API33/android13固定输入 | 多许可与工具/headers/库分层；不能一律写Apache | android-upstream.json |
| libva/libdrm/X11/ALSA | 取决于目标SDK/rootfs/系统 | 多为MIT式/部分LGPL，按实际装载库与是否重分发分别判断 | staged-elf-dependencies.json；各平台audio/presentation文档 |
| Alpine/Debian/Waydroid及用户安装软件 | 分发/下载/用户安装时各自决定 | 包级GPL/LGPL/BSD/MIT/字体等，不等于宿主链接；任意rootfs无固定完整闭包 | staged-apk-packages.json；distribution loading源码 |
| gVisor / QEMU / GNU工具 / Xcode / SDK | 按具体测试/构建输入 | 测试参照与工具许可证不自动传染构建输出；重分发工具/拷入代码另审 | build-input-urls.json；third-party-source-headers.json |

## 可验证的正面结果

1. 147个主锁文件第三方包没有缺失许可声明；vendored smoltcp(0BSD)、wayland-backend(MIT)、ext4-view(MIT OR Apache-2.0)均有原许可文件。
2. 固定MoltenVK源码及ExternalRevisions依赖留有哈希，源缓存包含各上游许可证；不是只保留预编译驱动链接。
3. iPad已提交Swift/Bionic/CoreMark/Wayland等notice，guest musl也有COPYRIGHT。
4. 本地FFmpeg实际二进制自报LGPL2.1+，其构建字串没有启用GPL/nonfree；仍需对正式交付版本逐个验证。
5. 部分crate源包不含许可文本的问题中，xim两子包已有固定hash的补偿机制，不能再报告为完全漏声明。
6. 生产IPA打包脚本明确排除Wayland acceptance APK离线仓库。这缓解了测试rootfs混入生产的风险，但其它测试/构建路径仍需以实际产物为准。

## 建议处理顺序

1. 先做Mac release source闭包检查（尤其cli子模块），并核对最近发行ZIP的Sources。不要动生产代码来掩盖打包证据缺失。
2. 对glibc转写与FFmpeg补丁做小范围来源/许可复核，明确商业发行中哪些组件继续按LGPL/GPL分发，哪些确有额外授权。
3. 为实际iPad/Mac包生成target/features级notices、源码索引和库摘要；原锁文件超集继续保留作追踪。
4. 将FFmpeg/glibc源码和构建配置与产物hash绑定，明确修改与替换方式，再评估商店与最终EULA。
5. 更新Wayland notice漂移，并锁住Python传递构建依赖和Actions SHA。
6. 最后再启用CLA：CLA解决今后贡献授权，不替代以上第三方义务或历史权属。

## 尚未取得的证据

- 未逐个下载/解包所有历史GitHub Release IPA/ZIP/对应源码包；未作“最近发行已违反许可证”的结论。
- 未审计用户任意导入rootfs、未来首次下载的所有包或运行时自行安装的任意程序。78项只是可读的本地暂存APK集合。
- 未将147项全部宣称为某一App静态链接项。所有target/optional/dev依赖均需在实际交付闭包裁剪。
- 辅助锁文件部分许可来自crates.io精确版本声明，未独立获取每项许可原文；完整状态在JSON记录。
- 未进行所有原创文件的抄袭溯源、专利检索、法域意见或二进制反向重建。glibc translation仅凭文字来源说明不能直接断定衍生性。

## 复现与附件

- [Rust完整表](RUST-INVENTORY.md) / [主锁文件JSON](rust-dependencies.json) / [辅助锁文件JSON](auxiliary-rust-dependencies.json)
- [Swift](swift-dependencies.json)、[本地APK](staged-apk-packages.json)、[暂存ELF动态依赖](staged-elf-dependencies.json)
- [native源码输入](vulkan-upstream.json)、[Android输入](android-upstream.json)、[Python构建工具](python-build-dependencies.json)
- [Actions引用](github-actions.json) / [Actions上游许可](github-action-licenses.json)
- [notice文本哈希](notice-evidence.json)、[Wayland notice差异](wayland-notices.diff)、[源文件header线索](third-party-source-headers.json)
- [源码快照](snapshots.json)、[构建URL表达式](build-input-urls.json)、[机器风险表](risks.json)
- 各仓 `*-cargo-graph.json` 保存cargo metadata的完整依赖边和target/build/dev条件。

主要读取命令：`cargo metadata --format-version 1 --locked --offline --all-features`；Linux仅为缺失的zerocopy-derive下载执行了一次联网metadata，无构建。`prepare-wayland-notices.py --check` 在系统Python3.9缺tomllib时先失败，临时导入tomli兼容层后复现notice断言失败；未修改源码脚本或notice。预期notice仅输出至临时目录，diff保存在报告目录。

## 外部原始依据

- [GPLv3原文](https://www.gnu.org/licenses/gpl-3.0.en.html)：对应源码、下游权利、额外限制及适用条件。
- [GNU FAQ](https://www.gnu.org/licenses/gpl-faq.html)：商业使用、解释器/数据区别及多重授权权利。
- [LGPL2.1原文](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)：库修改/链接与用户替换等条件。
- [FFmpeg官方许可说明](https://ffmpeg.org/legal.html)：构建配置、LGPL/GPL、确切源码与编译说明。
- [Mesa官方许可说明](https://docs.mesa3d.org/license.html)：按文件/组件许可，不是全库一个标签。
- [Apple开发者协议](https://developer.apple.com/support/terms/apple-developer-program-license-agreement/)：应以实际签署版本和实际发行条款核对，本文未代用户接受条款。
