# Trames 五仓许可证审查：逐项与发行物核验结果

审查日期：2026-11-26。覆盖 `trames-ipad`、`trames-runtime`、`trames-mac`、`trames-linux`、`trames-cli`。

## 明确结论

**可以商业使用、收费分发，也可以对你确实拥有权利的代码另行提供商业许可。没有发现必须替换的Rust/Swift宽松许可依赖。当前已发布包存在必须补齐的第三方版权声明缺口，不能直接标记为许可证材料齐全。**

**不能把全部组件直接改成自有闭源许可。** glibc、FFmpeg、rootfs中的GPL/LGPL组件以及不属于你版权的代码，各自条款继续生效。新CLA只解决适用贡献的授权，不改变第三方授权。

这次已完成实际逐项检查，不再把“未扫描”当结论：

| 对象 | 已完成检查 | 结果 |
| --- | --- | --- |
| 当前主锁文件 + 辅助锁文件 | 186项Cargo版本/来源组合的包内容、锁文件校验和、完整许可原文；缺文本追到固定上游提交 | 全部取得许可原文；均有允许商业及闭源组合的分支，须遵守所选条件 |
| Swift | 11个实际版本/提交的源码归档，含内部BoringSSL等许可文件 | 全部取得原文；保留MIT/Apache/BSD等正文、例外及NOTICE |
| 历史发行源码中的Rust锁文件 | 59个Cargo.lock记录、145个第三方name/version组合 | 全部落在上述186项已核原文集合中 |
| 已发布GitHub附件 | iPad9个Release、Mac5个Release，83附件全部下载 | 无下载失败；28条已发布SHA-256全部匹配 |
| 实际归档 | 10个IPA、10个Mac ZIP、23个独立源码包，共43个；继续检查其中嵌套源码 | 无归档解析失败；逐个文件索引和license文本均留存 |
| Runtime/Linux/CLI Release | 逐仓读取完整Release列表 | 三仓当前没有Release附件，不存在漏扫的公开二进制包 |
| 原生与guest资源 | 当前暂存11个ELF、78个APK、MoltenVK及其固定源码闭包、glibc/musl/Bionic源码 | 已按宿主链接、guest独立程序、开发测试资源分开记录 |

主锁文件本身为147项，含辅助锁文件后186项。不是把186项都当作生产App链接项。

## 1. 已确认缺口：18个发行物遗漏3项版权声明

已发布二进制中有这些组件的明确符号标记；在二进制包及同版本随附源码包的文本中搜索原版权声明，均未找到：

| 依赖 | 许可 | 缺少的版权主体 | 必须做什么 |
| --- | --- | --- | --- |
| generic-array | MIT | Bartłomiej Kamiński | 随分发保留完整版权与MIT许可 |
| simd-adler32 | MIT | Marvin Countryman | 随分发保留完整版权与MIT许可 |
| libloading | ISC | Simonas Kazlauskas | 随分发保留版权与ISC许可 |

**确定受影响：** iPad从v0.16.0起的8个IPA，以及Mac v1.5.0至v1.5.4的全部10个Stable/Dev ZIP，含当前最新版本。逐个文件列在 [发行物逐项表](RELEASE-BY-RELEASE.md) 与 [原始检查结果](release-copyright-checks.json)。

iPad v0.14.0、v0.15.0的包也已扫描，但未用这三个名字找到可确认的二进制符号；不能把它们按同一条证据统计为上述18项。它们并没有被标成“完整合规通过”。这不会改变当前最新IPA/ZIP已确认缺声明的结论。

**整改结论：补材料，不必为这三项替换依赖或购买许可。** 在实际交付包内聚合完整THIRD_PARTY_NOTICES，并随对应源码分发；GPL根LICENSE不替代这些原版权声明。核对上游OR选择与AND附加条款后，对实际链接闭包统一生成，而不是只补三个名字。

实际原文已存入 [186项逐包决定](dependency-decisions.json) 对应的 `license-texts/`，无需再靠网页猜测版权主体。

## 2. 商业授权可以覆盖什么

| 类别 | 商业收费 | 闭源组合／再授权结论 |
| --- | --- | --- |
| Rust 186项中的MIT/BSD/ISC/Apache/0BSD等所选分支 | 允许 | 宽松许可允许闭源组合；保留各自版权、许可、NOTICE与AND条件，不冒称第三方版权归项目所有 |
| Swift11项，包括BoringSSL等内部组件 | 允许 | 可用于商业产品；保留内部组件许可和Apple Swift类例外，不能只写一个Apache标签 |
| musl、MoltenVK、SPIRV-Cross/Tools、Volk、cereal、Mesa/Xwayland的适用宽松部分 | 允许 | 按具体文件与修改条款使用；Trames自己写的GPL包装/补丁另作权属处理 |
| glibc原始代码、分发的修改版glibc | 允许 | LGPL义务继续生效，不能用项目商业CLA把它变成自有专有代码 |
| FFmpeg LGPL配置 | 允许 | 保留LGPL、精确源码/修改/构建说明及适用的替换权；启用GPL/nonfree会改变结论，必须按实际构建处理 |
| 独立guest中的BusyBox、Debian/Alpine包、Android系统组件 | 允许各自许可下的商业使用 | 独立程序的重分发义务逐包履行，不把整个rootfs换一个项目许可；也不能仅因guest有GPL2就认定宿主GPL3不兼容 |
| 你个人完全拥有版权的项目代码 | 允许 | 可以另发商业许可；既有GPL副本的下游权利不被撤销 |
| 其他贡献者的GPL代码、无额外授权的衍生代码 | 允许GPL下商业使用 | 不可仅靠未来CLA追溯闭源再许可；需要实际授权或保持原许可 |

**因此：商业化可行；“把整个依赖树全部当自有代码闭源”不可行。** 当前首先要补发行物版权声明，再决定商业交付包里哪些组件按开源条款保留。

## 3. 逐项扫描后撤回的初步推测

### Mac历史包并未因CLI拆分漏源码

当前 `scripts/package-mac.py` 的确没有自动归档后来独立的 `cli/` 子模块；这是**下次使用当前脚本发布时必须修的源码闭包缺口**。

但已逐个打开全部10个Mac ZIP及其中源码归档。历史发布源码中没有 `trames-cli-common` 的依赖引用；它们早于这次拆分。因此**不把已发布Mac ZIP报告为缺CLI源码**。v1.5.4包实际包含产品、Runtime、iPad相关源码和原生Vulkan源码，见归档索引。

### 历史发行包没有FFmpeg

全部20个已发布IPA/ZIP都未包含 `libavcodec`、`libavutil` 等FFmpeg库。**历史Release不存在本次所怀疑的FFmpeg随包源码缺口。**

当前本地暂存FFmpeg库则确实存在，版本7.1.1，自报LGPL2.1+，嵌入构建参数未启用 `--enable-gpl` 或 `--enable-nonfree`。它属于**下一次包含该组件的发行**需要完整处理的LGPL对象，不反推已发布包违规。

### r-efi、crc-catalog、defmt、xim不是无许可证

已经逐个补取原文：r-efi的完整授权在AUTHORS；crc-catalog在固定提交的LICENSES目录；defmt缺省许可在仓库根；xim在原始固定提交的LICENSE。这些项从“原文缺失”列表移除。r-efi可选MIT，无需选择LGPL。

## 4. 其它确定处理项

1. **Wayland notices漂移：** 现有check确实失败，差异为Windows-only闭包版本变化；更新该清单，但不说iPad生产代码链接了Windows库。
2. **SPDX AND：** encoding_rs的BSD附加许可、unicode-ident的Unicode附加许可必须保留；不能只选MIT后删掉AND部分。
3. **glibc源与宿主Rust实现分开：** build.rs没有把vendored glibc C文件编进原生库；宿主转换实际调用gdtoa并实现自己的Rust整数处理。故撤回“宿主必然静态链接glibc”的暗示。原始LGPL文件仍按LGPL分发，Rust转写不因只换语言就自动取得第三方版权。没有证据支持现在把全部Runtime判成LGPL。
4. **源归档不是许可证豁免：** 包含Source.tar不免除MIT/ISC明确要求的原版权声明。18项F01通过实际文本和二进制符号对应建立，不是仅从脚本推测。
5. **构建工具：** Actions五个项目均MIT，但引用可变；Python固定4项不代表锁住Mako的MarkupSafe传递版本。该项应按构建可追踪性修正，不定性为许可证冲突。
6. **CoreMark和编解码器：** CoreMark的Apache代码许可与商标使用条款分开；视频算法/专利许可不由源码宽松许可提供。不要在代码许可审查中冒称已获得商标/专利授权。

## 5. 交付文件与复核路径

- [每个Cargo包的选用分支和结论](DEPENDENCY-DECISIONS.md) / [完整机器记录](dependency-decisions.json)
- [Swift逐项原文及源码哈希](swift-license-text-review.json)
- [每个Release归档的结果](RELEASE-BY-RELEASE.md) / [完整归档索引](release-archive-scans.json)
- [83个下载附件、大小和SHA-256](downloaded-release-assets.json)
- [28个发布checksum逐项结果](published-checksum-verification.json)
- [18项确认缺口及当前脚本问题](confirmed-findings.json)
- [逐个包的版权检索与二进制标记](release-copyright-checks.json)
- [原生库DT_NEEDED/许可字符串](staged-elf-dependencies.json)、[APK包级许可](staged-apk-packages.json)
- 完整原文副本保存在 `license-texts/`；不是仅存许可证名称。

本次只新增审计证据和报告，没有修许可证、CLA、notice或源码，也没有提交/发布。临时下载的实际归档保存在 `/private/tmp/trames-license-audit-20261126/releases/`，每个哈希和原URL都有记录。

## 许可依据

结论以实际源文件许可证原文为主，参照[GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html)、[GNU FAQ](https://www.gnu.org/licenses/gpl-faq.html)、[LGPL2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)、[FFmpeg官方许可说明](https://ffmpeg.org/legal.html)与[Mesa文件级许可说明](https://docs.mesa3d.org/license.html)。GPL/LGPL下的商业使用和收费并不被禁止；另发专有许可需要实际版权或相应授权。
