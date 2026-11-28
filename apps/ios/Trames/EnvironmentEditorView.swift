// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import TramesKit
import UniformTypeIdentifiers

@MainActor
final class EnvironmentEditor: ObservableObject {
    @Published var document:EnvironmentConfiguration?
    @Published var permissions:[String:Bool]=[:]
    @Published var snapshot:EnvironmentConfigurationSnapshot?
    @Published var failure:String?
    @Published var loading=false
    @Published var saving=false
    @Published var message="保存后在下次启动时生效"
    private var identity:String?
    var dirty:Bool {guard let snapshot else{return false};return document != snapshot.document || permissions != snapshot.permissions}
    func load(_ profile:RootfsEnvironment,workspace:WorkspaceModel) async {
        identity=profile.id;loading=true;failure=nil;document=nil;snapshot=nil
        do {
            guard let store=workspace.environment else {throw TramesError.message(L10n.text("环境存储不可用"))}
            let value=try await store.configurationSnapshot(profile)
            guard !Task.isCancelled,identity==profile.id else{return}
            snapshot=value;document=value.document;permissions=value.permissions;message="保存后在下次启动时生效"
        } catch {if !Task.isCancelled {failure=error.localizedDescription}}
        if identity==profile.id {loading=false}
    }
    func save(_ profile:RootfsEnvironment,workspace:WorkspaceModel) async -> Bool {
        guard !saving,let store=workspace.environment,let document,let snapshot else{return false}
        saving=true;defer {saving=false}
        do {
            let value=try await store.saveConfiguration(profile,document:document,permissions:permissions,replacing:snapshot)
            self.snapshot=value;self.document=value.document;self.permissions=value.permissions;failure=nil;message="配置已保存，下次启动生效"
            try await workspace.refreshEnvironments();return true
        } catch {failure=error.localizedDescription;return false}
    }
    func revert() {document=snapshot?.document;permissions=snapshot?.permissions ?? [:];failure=nil;message="保存后在下次启动时生效"}
    func string(_ path:String)->Binding<String> {Binding(get:{self.document?.string(path) ?? ""},set:{self.document?.set(path,.string($0))})}
    func bool(_ path:String)->Binding<Bool> {Binding(get:{self.document?.bool(path) ?? false},set:{self.document?.set(path,.bool($0))})}
    func component(_ path:String)->Binding<String> {Binding(get:{
        guard let value=self.document?.get(path) else{return "auto"};switch value {case .bool(let b):return b ? "true":"false";case .string(let s):return s;default:return "auto"}
    },set:{self.document?.set(path,$0=="auto" ? .string("auto"):.bool($0=="true"))})}
    func applyTOML(_ text:String,importing:Bool=false) throws {
        guard let current=document else{return}
        var value=try EnvironmentConfiguration(toml:text)
        if importing {
            guard value.string("environment.type")==current.string("environment.type") else {throw TramesError.message(L10n.text("导入配置的系统类型与此环境不一致"))}
            value.set("environment",current.get("environment"));value.set("image",current.get("image"))
        } else {
            for key in ["environment.uuid","environment.name","environment.type","image.distribution"] {
                guard value.get(key)==current.get(key) else {throw TramesError.message(L10n.text("不能通过编辑配置替换环境身份或镜像"))}
            }
        }
        _=try value.toml()
        for key in EnvironmentConfiguration.iPadPermissions {permissions[key]=(permissions[key] ?? false) && value.bool("interop.\(key).enabled")}
        document=value
    }
}

enum EnvironmentPage:String,CaseIterable,Identifiable {
    case overview,configuration,files,permissions,activity
    var id:String {rawValue}
    var title:String {switch self {case .overview:return "概览";case .configuration:return "配置";case .files:return "文件与共享";case .permissions:return "宿主互通";case .activity:return "活动"}}
    var icon:String {switch self {case .overview:return "square.grid.2x2";case .configuration:return "slider.horizontal.3";case .files:return "folder";case .permissions:return "hand.raised";case .activity:return "clock"}}
}

struct EnvironmentDetailView:View {
    @ObservedObject var workspace:WorkspaceModel
    @ObservedObject var editor:EnvironmentEditor
    let profile:RootfsEnvironment
    let open:(Bool)->Void
    let restart:()->Void
    let stop:()->Void
    let reset:()->Void
    let delete:()->Void
    @State private var page:EnvironmentPage = .overview
    @State private var editingTOML=false
    @State private var importing=false
    @State private var export:URL?
    @State private var toml=""
    @State private var mountConfig = EnvironmentMountConfiguration()
    @State private var failedMountIDs: Set<UUID> = []
    @State private var pickingMountFolder = false
    @State private var mountError: String?
    @State private var mountNotice: String?
    @State private var renamingMount: EnvironmentMount?
    @State private var renameText = ""
    @State private var copiedPath: String?
    var body:some View {
        VStack(spacing:0) {
            HStack(alignment:.center, spacing:12) {
                VStack(alignment:.leading,spacing:5) {
                    Text(profile.name).font(.title2.bold()).accessibilityIdentifier("environment-detail-name")
                    Text("\(profile.distro.capitalized) · \(editor.document?.string("boot.mode") ?? profile.boot)").font(.subheadline).foregroundStyle(.secondary)
                }
                Spacer(minLength:12)
                HStack(alignment:.center, spacing:8) {
                    Button(L10n.text(workspace.selectedEnvironmentID==profile.id && workspace.local != nil ? "打开终端":"打开环境"),systemImage:"terminal") {open(false)}
                        .buttonStyle(.borderedProminent)
                        .accessibilityIdentifier("open-environment")
                    Menu {
                        if workspace.selectedEnvironmentID==profile.id,workspace.local != nil {
                            Button(L10n.text("重新启动环境"),systemImage:"arrow.clockwise") {restart()}
                            Button(L10n.text("停止环境"),systemImage:"stop.circle",role:.destructive) {stop()}
                        }
                        if profile.bootTarget=="linux" {Button(L10n.text("打开维护终端")) {open(true)}}
                        Divider()
                        Button(L10n.text("重置环境…"),systemImage:"arrow.counterclockwise",role:.destructive) {reset()}.accessibilityIdentifier("reset-environment")
                        Button(L10n.text("删除环境…"),systemImage:"trash",role:.destructive) {delete()}.accessibilityIdentifier("delete-environment")
                    } label:{
                        Image(systemName:"ellipsis")
                            .font(.body.weight(.semibold))
                            .frame(minWidth:20,minHeight:20)
                    }
                    .buttonStyle(.bordered)
                    .accessibilityLabel(L10n.text("更多"))
                }
            }.padding()
            ScrollView(.horizontal) {
                HStack(spacing:6) {ForEach(EnvironmentPage.allCases) {item in
                    Button {page=item} label:{Label(L10n.text(item.title),systemImage:item.icon).padding(.horizontal,12).padding(.vertical,8).background(page==item ? Color.accentColor.opacity(0.14):Color.clear,in:RoundedRectangle(cornerRadius:9))}
                        .buttonStyle(.plain).foregroundStyle(page==item ? Color.accentColor:Color.primary).accessibilityIdentifier("environment-tab-"+item.rawValue).accessibilityAddTraits(page==item ? .isSelected:[])
                }}.padding(.horizontal)
            }.scrollIndicators(.hidden).padding(.bottom,8)
            Divider()
            if editor.loading {Spacer();ProgressView(L10n.text("正在读取配置…"));Spacer()}
            else if let document=editor.document {
                Group {switch page {
                case .overview:overview(document)
                case .configuration:configuration(document)
                case .files:files
                case .permissions:permissions
                case .activity:activity
                }}.disabled(editor.saving)
            } else {ContentUnavailableView(L10n.text("无法读取配置"),systemImage:"exclamationmark.triangle",description:Text(editor.failure ?? ""))}
            if let failure=editor.failure,editor.document != nil {Text(failure).font(.callout).foregroundStyle(.red).textSelection(.enabled).padding().accessibilityIdentifier("environment-config-error")}
            HStack {
                Text(L10n.text(editor.message)).font(.caption).foregroundStyle(.secondary).accessibilityIdentifier("environment-save-status")
                Spacer()
                Button(L10n.text("还原草稿")) {editor.revert()}.disabled(!editor.dirty || editor.saving)
                Button(L10n.text("保存配置")) {Task {_=await editor.save(profile,workspace:workspace)}}.buttonStyle(.borderedProminent).disabled(!editor.dirty || editor.saving).accessibilityIdentifier("save-environment-config")
            }.padding().background(.bar)
        }.task(id:profile.id) {
            page = .overview; export = nil; mountError = nil; mountNotice = nil
            await editor.load(profile, workspace: workspace)
            await reloadMounts()
        }
            .sheet(isPresented:$editingTOML) {TOMLEditor(text:toml) {text in try editor.applyTOML(text)}}
            .fileImporter(isPresented:$importing,allowedContentTypes:[.plainText,.data]) {result in
                do {
                    let url=try result.get(),access=url.startAccessingSecurityScopedResource();defer {if access {url.stopAccessingSecurityScopedResource()}}
                    guard (try url.resourceValues(forKeys:[.fileSizeKey])).fileSize ?? Int.max <= 65536 else {throw TramesError.message(L10n.text("配置文件不能超过 64 KiB"))}
                    try editor.applyTOML(String(contentsOf:url,encoding:.utf8),importing:true)
                } catch {editor.failure=error.localizedDescription}
            }
            .fileImporter(isPresented: $pickingMountFolder, allowedContentTypes: [.folder]) { result in
                Task {
                    do {
                        guard let store = workspace.mounts else { return }
                        let url = try result.get()
                        let updated = try await store.add(selected: url, readonly: false, for: profile.id)
                        mountConfig = updated
                        mountError = nil
                        if let added = updated.mounts.last {
                            await applyLiveMount(id: added.id)
                        }
                    } catch {
                        mountError = error.localizedDescription
                    }
                }
            }
            .alert(L10n.text("重命名挂载点"), isPresented: Binding(get: { renamingMount != nil }, set: { if !$0 { renamingMount = nil } })) {
                TextField(L10n.text("目录名称"), text: $renameText)
                    .autocorrectionDisabled()
                    .textInputAutocapitalization(.never)
                Button(L10n.text("保存")) {
                    guard let target = renamingMount else { return }
                    let oldPath = target.guestPath
                    let newName = renameText
                    renamingMount = nil
                    Task {
                        do {
                            guard let store = workspace.mounts else { return }
                            mountConfig = try await store.rename(newName, id: target.id, for: profile.id)
                            mountError = nil
                            await applyLiveRemount(oldPath: oldPath, id: target.id)
                        } catch {
                            mountError = error.localizedDescription
                        }
                    }
                }
                Button(L10n.text("取消"), role: .cancel) { renamingMount = nil }
            } message: {
                Text(L10n.text("目录将挂载到 /mnt/iPad/<名称>，不能命名为 Shared。"))
            }
    }
    private func overview(_ document:EnvironmentConfiguration)->some View {
        Form {
            Section(L10n.text("环境信息")) {
                LabeledContent(L10n.text("名称"),value:profile.name)
                LabeledContent(L10n.text("系统"),value:profile.bootTarget=="android" ? "Android":"Linux")
                LabeledContent(L10n.text("发行版"),value:profile.distro.capitalized)
                LabeledContent("UUID") {Text(document.string("environment.uuid")).font(.caption.monospaced()).textSelection(.enabled)}
                LabeledContent(L10n.text("状态"),value:L10n.text(workspace.selectedEnvironmentID==profile.id && workspace.local != nil ? "当前环境":"未打开"))
            }
            Section(L10n.text("资源配置")) {
                LabeledContent(L10n.text("CPU 核心"),value:resourceText(document.get("resources.cpus")))
                LabeledContent(L10n.text("内存"),value:document.string("resources.memory")=="auto" ? L10n.text("自动（按需使用）"):document.string("resources.memory"))
                LabeledContent(L10n.text("网络"),value:L10n.text(document.string("network.mode")=="host" ? "宿主网络":"网络已隔离"))
            }
            Section(L10n.text("配置文件")) {
                LabeledContent(L10n.text("配置来源"),value:L10n.text(editor.snapshot?.configurationData == nil ? "默认配置":"此设备上的环境配置"))
                HStack {Button(L10n.text("编辑 TOML")) {showTOML()}.accessibilityIdentifier("edit-environment-toml");Spacer();Button(L10n.text("导入 TOML…")) {importing=true}}
                Button(L10n.text("导出 TOML…")) {Task {do {export=try await workspace.environment?.exportedConfiguration(profile,document:document)}catch{editor.failure=error.localizedDescription}}}
                if let export {ShareLink(item:export) {Label(L10n.text("共享配置文件"),systemImage:"square.and.arrow.up")}}
                Text(L10n.text("TOML 与 Mac 使用同一格式。导入保留当前环境身份，不会获得新的设备访问权限。")).font(.caption).foregroundStyle(.secondary)
            }
        }
    }
    private func resourceText(_ value:BridgeValue)->String {switch value {case .number(let n):return String(Int(n));case .string("all"):return L10n.text("全部可用核心");default:return value.text ?? ""}}
    private func configuration(_ document:EnvironmentConfiguration)->some View {
        Form {
            Section(L10n.text("计算资源")) {
                Picker(L10n.text("CPU 核心"),selection:Binding(get:{resourceText(document.get("resources.cpus"))==L10n.text("全部可用核心") ? "all":resourceText(document.get("resources.cpus"))},set:{editor.document?.set("resources.cpus",$0=="all" ? .string("all"):.number(Double($0) ?? 1))})) {
                    Text(L10n.text("全部可用核心")).tag("all")
                    ForEach(1...max(1,min(32,ProcessInfo.processInfo.activeProcessorCount)),id:\.self) {Text(String($0)).tag(String($0))}
                }.accessibilityIdentifier("environment-cpus")
                EnvironmentMemoryPicker(title:"内存",value:Binding(get:{
                    let limit = editor.document?.string("resources.memory_limit") ?? ""
                    return limit.isEmpty ? (editor.document?.string("resources.memory") ?? "auto") : limit
                },set:{
                    editor.document?.set("resources.memory",.string($0))
                    editor.document?.set("resources.memory_limit",.null)
                }),automatic:true).accessibilityIdentifier("environment-memory")
                let availableBytes = WorkspaceModel.currentAvailableMemoryBytes()
                if availableBytes > 0 {
                    LabeledContent(L10n.text("当前设备可用内存"), value: ByteCountFormatter.string(fromByteCount: Int64(availableBytes), countStyle: .binary))
                }
                Text(L10n.text("内存按需分配，不会预先占满。iPadOS 上会自动保留安全余量以防止超限闪退。")).font(.caption).foregroundStyle(.secondary)
            }
            Section(L10n.text("启动与会话")) {
                if profile.bootTarget=="android" {LabeledContent(L10n.text("启动方式"),value:"Android init")}
                else {
                    Picker(L10n.text("启动方式"),selection:editor.string("boot.mode")) {Text("Shell").tag("shell");if profile.distro != "alpine" {Text("systemd").tag("systemd")}}
                    if profile.distro=="debian" {Text(L10n.text("选择 systemd 后，下次启动会自动准备所需的软件包。")).font(.caption).foregroundStyle(.secondary)}
                    HStack {
                        Text(L10n.text("登录用户"))
                        TextField("root",text:editor.string("session.user"))
                            .multilineTextAlignment(.trailing)
                            .autocorrectionDisabled()
                            .textInputAutocapitalization(.never)
                            .accessibilityIdentifier("environment-user")
                    }
                    HStack {
                        Text(L10n.text("工作目录"))
                        TextField(L10n.text("自动（用户主目录）"),text:editor.string("session.cwd"))
                            .multilineTextAlignment(.trailing)
                            .autocorrectionDisabled()
                            .textInputAutocapitalization(.never)
                            .accessibilityIdentifier("environment-cwd")
                    }
                    let args=document.strings("session.shell")
                    if args.isEmpty {
                        LabeledContent(L10n.text("启动 Shell"), value: L10n.text("跟随登录用户（/etc/passwd）"))
                        Button(L10n.text("自定义启动命令"),systemImage:"plus") {
                            editor.document?.set("session.shell",.array([.string("/bin/sh"), .string("-l")]))
                        }
                    } else {
                        ForEach(Array(args.enumerated()),id:\.offset) {index,_ in
                            HStack {
                                TextField(L10n.text(index==0 ? "Shell 路径":"参数"),text:Binding(get:{editor.document?.strings("session.shell").indices.contains(index)==true ? editor.document!.strings("session.shell")[index]:""},set:{var args=editor.document?.strings("session.shell") ?? [];if args.indices.contains(index) {args[index]=$0;editor.document?.set("session.shell",.array(args.map(BridgeValue.string)))}})).textInputAutocapitalization(.never).autocorrectionDisabled()
                                Button(role:.destructive) {
                                    var args=editor.document?.strings("session.shell") ?? []
                                    if args.indices.contains(index) {
                                        if index == 0 {
                                            args.removeAll()
                                        } else {
                                            args.remove(at:index)
                                        }
                                        editor.document?.set("session.shell",.array(args.map(BridgeValue.string)))
                                    }
                                } label:{Image(systemName:"minus.circle")}.accessibilityLabel(L10n.text("删除参数"))
                            }
                        }
                        HStack {
                            Button(L10n.text("添加参数"),systemImage:"plus") {var args=editor.document?.strings("session.shell") ?? [];args.append("");editor.document?.set("session.shell",.array(args.map(BridgeValue.string)))}
                            Spacer()
                            Button(L10n.text("跟随登录用户")) {
                                editor.document?.set("session.shell",.array([]))
                            }
                            .foregroundStyle(.secondary)
                        }
                    }
                    Text(L10n.text("留空时自动读取 /etc/passwd 中该用户的登录 Shell 与主目录（例如 Debian 默认 bash，chsh 修改后新终端即生效）。")).font(.caption).foregroundStyle(.secondary)
                }
            }
            Section(L10n.text("原生组件")) {
                Toggle(L10n.text("应用窗口"),isOn:editor.bool("display.windows"))
                component("Vulkan 图形与计算","components.vulkan")
                component("视频加速（VA-API）","components.vaapi")
                component("原生函数加速","components.thunking")
                Toggle(L10n.text("音频输出"),isOn:editor.bool("components.audio"))
                Toggle(L10n.text("麦克风输入"),isOn:editor.bool("components.audio_input"))
                Text(L10n.text("原生组件仍需环境内对应的加载器。麦克风还受 iPadOS 权限控制。")).font(.caption).foregroundStyle(.secondary)
            }
            Section(L10n.text("网络连接")) {
                Picker(L10n.text("网络模式"),selection:editor.string("network.mode")) {
                    Text(L10n.text("宿主网络")).tag("host");Text(L10n.text("无网络")).tag("none")
                    if document.string("network.mode")=="bridge" {Text(L10n.text("Trames bridge（此平台暂不支持）")).tag("bridge").disabled(true)}
                }.onChange(of:editor.document?.string("network.mode")) {_,mode in if mode != "bridge" {editor.document?.set("network.bridge",.null);editor.document?.set("network.publish",.array([]))}}
                Text(L10n.text("宿主网络可直接访问网络及监听端口。Trames bridge 与端口发布目前仅在 Mac 提供。")).font(.caption).foregroundStyle(.secondary)
            }
            Section {Button(L10n.text("编辑 TOML")) {showTOML()}}
        }
    }
    private func component(_ title:String,_ path:String)->some View {
        Picker(L10n.text(title),selection:editor.component(path)) {Text(L10n.text("自动")).tag("auto");Text(L10n.text("启用")).tag("true");Text(L10n.text("禁用")).tag("false")}
    }
    private var permissions:some View {
        Form {
            Section {
                permission("剪贴板","与 iPad 交换文字","clipboard")
                permission("文件传输","仅访问 Files 中已授权的目录","files")
                permission("宿主界面操作","从环境打开网址、管理器和设备信息","host_ui")
            } header:{Text(L10n.text("环境权限"))} footer:{Text(L10n.text("允许后按环境权限直接执行，iPadOS 系统权限仍然有效。保存后下次启动生效。"))}
        }
    }
    private func permission(_ title:String,_ detail:String,_ key:String)->some View {
        Toggle(isOn:Binding(get:{editor.permissions[key] ?? false},set:{editor.permissions[key]=$0;editor.document?.set("interop.\(key).enabled",.bool($0))})) {VStack(alignment:.leading) {Text(L10n.text(title));Text(L10n.text(detail)).font(.caption).foregroundStyle(.secondary)}}.accessibilityIdentifier("environment-permission-"+key)
    }
    private var files:some View {
        Form {
            Section {
                Toggle(L10n.text("挂载「我的 iPad → Trames」"), isOn: Binding(
                    get: { mountConfig.mountSharedDirectory },
                    set: { enabled in
                        mountConfig.mountSharedDirectory = enabled
                        Task {
                            do {
                                guard let store = workspace.mounts else { return }
                                mountConfig = try await store.setMountSharedDirectory(enabled, for: profile.id)
                                mountError = nil
                                await applyLiveSharedDirectory(enabled: enabled)
                            } catch {
                                mountError = error.localizedDescription
                            }
                        }
                    }
                ))
                .accessibilityIdentifier("toggle-shared-directory")
                if mountConfig.mountSharedDirectory {
                    HStack {
                        VStack(alignment: .leading, spacing: 2) {
                            Text(L10n.text("挂载路径")).font(.subheadline)
                            Text(MountStore.sharedGuestPath).font(.caption.monospaced()).foregroundStyle(.secondary)
                        }
                        Spacer()
                        Button {
                            copyGuestPath(MountStore.sharedGuestPath)
                        } label: {
                            Image(systemName: copiedPath == MountStore.sharedGuestPath ? "checkmark" : "doc.on.doc")
                        }
                        .buttonStyle(.borderless)
                        .accessibilityLabel(L10n.text("复制路径"))
                    }
                }
            } header: {
                Text(L10n.text("内置共享目录"))
            } footer: {
                Text(L10n.text("开启后，Linux 内可直接读写「我的 iPad → Trames」与 USB 访达文件共享目录。从其他 App 分享到 Trames 的文件保存在 /mnt/iPad/Shared/Imports。"))
            }

            Section {
                if mountConfig.mounts.isEmpty {
                    Text(L10n.text("尚未添加自定义目录")).foregroundStyle(.secondary)
                } else {
                    ForEach(mountConfig.mounts) { mount in
                        VStack(alignment: .leading, spacing: 6) {
                            HStack(alignment: .center, spacing: 10) {
                                Image(systemName: "folder.fill").foregroundStyle(.tint)
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(mount.name).font(.body.weight(.medium))
                                    Text(mount.guestPath).font(.caption.monospaced()).foregroundStyle(.secondary)
                                }
                                Spacer()
                                Button {
                                    copyGuestPath(mount.guestPath)
                                } label: {
                                    Image(systemName: copiedPath == mount.guestPath ? "checkmark" : "doc.on.doc")
                                }
                                .buttonStyle(.borderless)
                                .accessibilityLabel(L10n.text("复制路径"))
                            }
                            Toggle(L10n.text("只读挂载"), isOn: Binding(
                                get: { mount.readonly },
                                set: { readonly in
                                    Task {
                                        do {
                                            guard let store = workspace.mounts else { return }
                                            mountConfig = try await store.setReadonly(readonly, id: mount.id, for: profile.id)
                                            mountError = nil
                                            await applyLiveRemount(oldPath: mount.guestPath, id: mount.id)
                                        } catch {
                                            mountError = error.localizedDescription
                                        }
                                    }
                                }
                            ))
                            .font(.subheadline)
                            if mount.isCloudBacked {
                                Label(L10n.text("云端目录可能会在访问时按需下载文件。"), systemImage: "icloud")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                            if failedMountIDs.contains(mount.id) {
                                Label(L10n.text("上次启动时无法访问此目录，请确认目录存在或重新添加。"), systemImage: "exclamationmark.triangle")
                                    .font(.caption)
                                    .foregroundStyle(.orange)
                            }
                        }
                        .padding(.vertical, 2)
                        .contextMenu {
                            Button(L10n.text("重命名挂载点…"), systemImage: "pencil") {
                                renameText = mount.name
                                renamingMount = mount
                            }
                            Button(L10n.text("移除"), systemImage: "trash", role: .destructive) {
                                removeMount(mount.id)
                            }
                        }
                        .swipeActions(edge: .trailing) {
                            Button(L10n.text("移除"), role: .destructive) {
                                removeMount(mount.id)
                            }
                        }
                    }
                }
                Button(L10n.text("添加文件夹…"), systemImage: "folder.badge.plus") {
                    pickingMountFolder = true
                }
                .accessibilityIdentifier("add-mounted-folder")
                if let mountNotice {
                    Text(mountNotice).font(.caption).foregroundStyle(.secondary)
                }
                if let mountError {
                    Text(mountError).font(.caption).foregroundStyle(.red)
                }
            } header: {
                Text(L10n.text("自定义挂载目录"))
            } footer: {
                Text(L10n.text("新添加的目录默认以读写模式挂载到 /mnt/iPad/<名称>，运行中的环境会立即生效。同名目录会自动编号去重。"))
            }

            Section(L10n.text("环境根目录")) {
                Text(L10n.text("在「文件」App 左侧栏开启「Trames」位置可直接浏览此环境的 Linux 根目录。")).font(.callout)
                LabeledContent(L10n.text("环境目录")) {Text(profile.store).font(.caption.monospaced()).textSelection(.enabled)}
            }
        }
    }
    private func reloadMounts() async {
        guard let store = workspace.mounts else { return }
        mountConfig = await store.configuration(for: profile.id)
        if let env = workspace.environment {
            failedMountIDs = await env.failedMountIDs(for: profile.id)
        }
    }
    private var activeLocalSession: LocalSession? {
        guard workspace.selectedEnvironmentID == profile.id else { return nil }
        return workspace.local?.local
    }
    private func applyLiveSharedDirectory(enabled: Bool) async {
        guard let local = activeLocalSession else { return }
        do {
            if enabled {
                let dir = try SharedInboxBridge.sharedDirectory()
                try await Task.detached(priority: .userInitiated) {
                    try local.mountHost(LocalHostMount(target: MountStore.sharedGuestPath, host: dir, readonly: false))
                }.value
                mountNotice = L10n.text("已实时挂载 %@", MountStore.sharedGuestPath)
            } else {
                try await Task.detached(priority: .userInitiated) {
                    try local.unmountHost(target: MountStore.sharedGuestPath)
                }.value
                mountNotice = L10n.text("已实时卸载 %@", MountStore.sharedGuestPath)
            }
            mountError = nil
        } catch {
            mountNotice = L10n.text("目录当前正被使用，关闭相关进程或下次启动时生效。")
        }
    }
    private func applyLiveMount(id: UUID) async {
        guard let local = activeLocalSession, let store = workspace.mounts else { return }
        guard let resolved = await store.resolveSingleMount(id: id, for: profile.id) else { return }
        do {
            try await Task.detached(priority: .userInitiated) {
                try local.mountHost(resolved.mount, scopedURL: resolved.scopedURL)
            }.value
            failedMountIDs.remove(id)
            mountError = nil
            mountNotice = L10n.text("已实时挂载 %@", resolved.mount.target)
        } catch {
            mountError = error.localizedDescription
        }
    }
    private func applyLiveRemount(oldPath: String, id: UUID) async {
        guard let local = activeLocalSession, let store = workspace.mounts else { return }
        do {
            try await Task.detached(priority: .userInitiated) {
                try local.unmountHost(target: oldPath)
            }.value
            guard let resolved = await store.resolveSingleMount(id: id, for: profile.id) else { return }
            try await Task.detached(priority: .userInitiated) {
                try local.mountHost(resolved.mount, scopedURL: resolved.scopedURL)
            }.value
            mountError = nil
            mountNotice = L10n.text("已实时更新挂载 %@", resolved.mount.target)
        } catch {
            mountNotice = L10n.text("目录当前正被使用，关闭相关进程或下次启动时生效。")
        }
    }
    private func removeMount(_ id: UUID) {
        let oldPath = mountConfig.mounts.first(where: { $0.id == id })?.guestPath
        Task {
            do {
                guard let store = workspace.mounts else { return }
                mountConfig = try await store.remove(id: id, for: profile.id)
                failedMountIDs.remove(id)
                mountError = nil
                if let oldPath, let local = activeLocalSession {
                    do {
                        try await Task.detached(priority: .userInitiated) {
                            try local.unmountHost(target: oldPath)
                        }.value
                        mountNotice = L10n.text("已实时卸载 %@", oldPath)
                    } catch {
                        mountNotice = L10n.text("目录当前正被使用，关闭相关进程或下次启动时生效。")
                    }
                }
            } catch {
                mountError = error.localizedDescription
            }
        }
    }
    private func copyGuestPath(_ path: String) {
        UIPasteboard.general.string = path
        copiedPath = path
        Task { @MainActor in
            try? await Task.sleep(for: .seconds(1.5))
            if copiedPath == path { copiedPath = nil }
        }
    }
    private var activity:some View {
        Form {
            Section(L10n.text("当前状态")) {Text(L10n.text(workspace.selectedEnvironmentID==profile.id && workspace.local != nil ? "此环境当前已打开":"此环境未打开"));Text(L10n.text(editor.message)).foregroundStyle(.secondary)}
            if FileManager.default.fileExists(atPath:PreparationDiagnostics.logURL.path) {Section(L10n.text("诊断")) {ShareLink(item:PreparationDiagnostics.logURL) {Label(L10n.text("共享环境准备日志"),systemImage:"square.and.arrow.up")};Text(L10n.text("日志来自最近一次环境准备操作。")).font(.caption).foregroundStyle(.secondary)}}
        }
    }
    private func showTOML() {do {if let data=try editor.document?.toml() {toml=String(decoding:data,as:UTF8.self);editingTOML=true}}catch{editor.failure=error.localizedDescription}}
}

private struct EnvironmentMemoryPicker:View {
    let title:String
    @Binding var value:String
    let automatic:Bool
    @State private var custom=false
    @State private var text=""
    private var options:[String] {(automatic ? ["auto"]:[""])+["256MiB","512MiB","1GiB","2GiB","4GiB","8GiB","16GiB"]}
    var body:some View {
        Picker(L10n.text(title),selection:Binding(get:{value},set:{if $0=="custom" {text=value;custom=true}else{value=$0}})) {
            ForEach(options,id:\.self) {option in Text(option=="auto" ? L10n.text("自动（按需使用）"):option.isEmpty ? L10n.text("无额外上限"):option).tag(option)}
            if !options.contains(value) {Text(value).tag(value)}
            Text(L10n.text("自定义…")).tag("custom")
        }.alert(L10n.text(title),isPresented:$custom) {
            TextField(L10n.text("例如 6GiB 或 768MiB"),text:$text).textInputAutocapitalization(.never).autocorrectionDisabled()
            Button(L10n.text("确定")) {value=text.trimmingCharacters(in:.whitespacesAndNewlines)}
            Button(L10n.text("取消"),role:.cancel) {}
        }
    }
}
private struct TOMLEditor:View {
    @Environment(\.dismiss) private var dismiss
    @State var text:String
    let apply:(String)throws->Void
    @State private var failure:String?
    var body:some View {
        NavigationStack {
            VStack(spacing:0) {
                TextEditor(text:$text).font(.system(.callout,design:.monospaced)).textInputAutocapitalization(.never).autocorrectionDisabled().accessibilityIdentifier("environment-toml-editor").padding(8)
                if let failure {Text(failure).foregroundStyle(.red).padding().accessibilityIdentifier("environment-toml-error")}
                Text(L10n.text("应用后仍需保存配置。本机访问权限请在宿主互通中设置。")).font(.caption).foregroundStyle(.secondary).padding()
            }.navigationTitle(L10n.text("编辑 TOML")).toolbar {
                ToolbarItem(placement:.cancellationAction) {Button(L10n.text("取消")) {dismiss()}}
                ToolbarItem(placement:.confirmationAction) {Button(L10n.text("应用到草稿")) {do {try apply(text);dismiss()}catch{failure=error.localizedDescription}}.accessibilityIdentifier("apply-environment-toml")}
            }
        }.presentationDetents([.large])
    }
}
