// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import UniformTypeIdentifiers
import TramesKit

enum ManagerDestination: Hashable {
    case environment(String)
    case settings
}

struct EnvironmentManagerView: View {
    @ObservedObject var workspace: WorkspaceModel
    let initialDestination: ManagerDestination?
    @Environment(\.dismiss) private var dismiss
    @AppStorage(InterfaceStyle.defaultsKey) private var interfaceStyle = "system"
    @StateObject private var editor = EnvironmentEditor()
    @State private var selected: ManagerDestination?
    @State private var creating = false
    @State private var pending: Route?
    @State private var deleting: RootfsEnvironment?
    @State private var resettingTarget: RootfsEnvironment?
    @State private var deletingNow = false
    init(workspace: WorkspaceModel, initialDestination: ManagerDestination? = nil) {
        self.workspace = workspace
        self.initialDestination = initialDestination
    }
    private enum Route { case close, select(ManagerDestination), create, open(RootfsEnvironment, Bool), restart(RootfsEnvironment), reset(RootfsEnvironment), delete(RootfsEnvironment) }
    private var selectedEnvironmentID: String? {
        if case .environment(let id) = selected { return id }
        return nil
    }
    private var visibleProfiles: [RootfsEnvironment] {
        #if TRAMES_ENABLE_ANDROID
        workspace.profiles
        #else
        workspace.profiles.filter { $0.bootTarget != "android" }
        #endif
    }
    var body: some View {
        NavigationSplitView {
            List(selection: Binding(get: { selected }, set: { if let target = $0, target != selected { request(.select(target)) } })) {
                Section(L10n.text("环境")) {
                    ForEach(visibleProfiles) { profile in
                        NavigationLink(value: ManagerDestination.environment(profile.id)) {
                            HStack(spacing: 12) {
                                Image(systemName: profile.bootTarget == "android" ? "apps.iphone" : "terminal").font(.title3).foregroundStyle(.tint).frame(width: 26)
                                VStack(alignment: .leading, spacing: 4) {
                                    Text(profile.name).font(.headline).lineLimit(1)
                                    Text("\(profile.distro.capitalized) · \(profile.bootTarget == "android" ? "Android" : profile.boot)").font(.caption).foregroundStyle(.secondary).lineLimit(1)
                                }
                                Spacer(minLength: 0)
                                if workspace.selectedEnvironmentID == profile.id, workspace.local?.local != nil {
                                    Circle().fill(.green).frame(width: 7, height: 7).accessibilityLabel(L10n.text("当前环境"))
                                }
                            }.padding(.vertical, 7)
                        }
                        .tag(ManagerDestination.environment(profile.id))
                        .accessibilityIdentifier("environment-row-" + profile.distro)
                        .contextMenu {
                            Button(L10n.text("重置环境…"), systemImage: "arrow.counterclockwise", role: .destructive) { request(.reset(profile)) }
                            Button(L10n.text("删除环境…"), systemImage: "trash", role: .destructive) { request(.delete(profile)) }
                        }
                    }
                }
                Section(L10n.text("工具与设置")) {
                    NavigationLink(value: ManagerDestination.settings) {
                        Label(L10n.text("通用设置"), systemImage: "gearshape").padding(.vertical, 4)
                    }
                    .tag(ManagerDestination.settings)
                    .accessibilityIdentifier("manager-row-settings")
                }
            }
            .listStyle(.sidebar)
            .navigationTitle(L10n.text("环境与设置"))
            .navigationSplitViewColumnWidth(min: 240, ideal: 280, max: 340)
            .safeAreaInset(edge: .bottom) {
                Button { request(.create) } label: {
                    Label(L10n.text("新建环境"), systemImage: "plus").frame(maxWidth: .infinity)
                }
                .buttonStyle(.bordered)
                .padding()
                .accessibilityIdentifier("create-linux-environment")
            }
            .toolbar {
                ToolbarItem(placement: .topBarLeading) {
                    Button(L10n.text("完成")) { request(.close) }
                }
            }
        } detail: {
            switch selected {
            case .environment(let id):
                if let profile = workspace.profiles.first(where: { $0.id == id }) {
                    EnvironmentDetailView(
                        workspace: workspace,
                        editor: editor,
                        profile: profile,
                        open: { maintenance in request(.open(profile, maintenance)) },
                        restart: { request(.restart(profile)) },
                        stop: { workspace.stopEnvironment(profile) },
                        reset: { request(.reset(profile)) },
                        delete: { request(.delete(profile)) }
                    )
                } else {
                    ContentUnavailableView(L10n.text("选择一个环境"), systemImage: "square.stack.3d.up", description: Text(L10n.text("独立管理每个环境的资源、组件与访问权限。")))
                }
            case .settings:
                AppSettingsContent(workspace: workspace)
            case nil:
                ContentUnavailableView(L10n.text("选择一个环境"), systemImage: "square.stack.3d.up", description: Text(L10n.text("独立管理每个环境的资源、组件与访问权限。")))
            }
        }
        .navigationSplitViewStyle(.balanced)
        // This cover is presented by the nested UIKit hosting controller.
        // Update its color scheme live when Interface Style changes.
        .preferredColorScheme((InterfaceStyle(rawValue: interfaceStyle) ?? .system).colorScheme)
        .task {
            do {
                try await workspace.refreshEnvironments()
                if selected == nil {
                    if let initialDestination {
                        selected = initialDestination
                    } else if !workspace.selectedEnvironmentID.isEmpty {
                        selected = .environment(workspace.selectedEnvironmentID)
                    } else if let first = workspace.profiles.first {
                        selected = .environment(first.id)
                    } else {
                        selected = .settings
                    }
                }
            } catch {
                editor.failure = error.localizedDescription
            }
        }
        .sheet(isPresented: $creating) {
            CreateEnvironmentView(workspace: workspace) { profile in
                selected = .environment(profile.id)
                creating = false
            }
        }
        .alert(L10n.text("有未保存的更改"), isPresented: Binding(get: { pending != nil }, set: { if !$0 { pending = nil } })) {
            Button(L10n.text("保存并继续")) {
                guard let route = pending,
                      let id = selectedEnvironmentID,
                      let profile = workspace.profiles.first(where: { $0.id == id }) else { return }
                pending = nil
                Task { if await editor.save(profile, workspace: workspace) { perform(route) } }
            }
            Button(L10n.text("放弃更改"), role: .destructive) {
                if let route = pending { pending = nil; editor.revert(); perform(route) }
            }
            Button(L10n.text("取消"), role: .cancel) { pending = nil }
        }
        .presentationDetents([.large])
        .alert(L10n.text("重置 Linux 环境？"), isPresented: Binding(get: { resettingTarget != nil }, set: { if !$0 { resettingTarget = nil } }), presenting: resettingTarget) { profile in
            Button(L10n.text("删除并重新初始化"), role: .destructive) {
                resettingTarget = nil
                workspace.resetEnvironment(profile)
            }
            Button(L10n.text("取消"), role: .cancel) { resettingTarget = nil }
        } message: { profile in
            Text(L10n.text("将重置「%@」中的软件、文件和配置。其他环境和 Files 目录授权会保留。", profile.name))
        }
        .confirmationDialog(L10n.text("删除环境？"), isPresented: Binding(get: { deleting != nil }, set: { if !$0 { deleting = nil } }), titleVisibility: .visible, presenting: deleting) { profile in
            Button(L10n.text("删除环境"), role: .destructive) {
                deletingNow = true
                Task {
                    defer { deletingNow = false }
                    do {
                        try await workspace.deleteEnvironment(profile)
                        if selectedEnvironmentID == profile.id {
                            editor.revert()
                            selected = workspace.profiles.first.map { .environment($0.id) } ?? .settings
                        }
                    } catch { editor.failure = error.localizedDescription }
                }
            }
            Button(L10n.text("取消"), role: .cancel) {}
        } message: { profile in
            Text(L10n.text("将永久删除「%@」的软件、文件和配置，且无法恢复。正在运行的会话将先关闭；其他环境不受影响。", profile.name))
        }
        .disabled(deletingNow || workspace.resetting)
        .interactiveDismissDisabled(deletingNow || workspace.resetting)
        .overlay {
            if deletingNow {
                ProgressView(L10n.text("正在删除环境…")).padding().background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
            }
        }
    }

    private func request(_ route: Route) {
        if selectedEnvironmentID != nil && editor.dirty { pending = route }
        else { perform(route) }
    }

    private func perform(_ route: Route) {
        switch route {
        case .close: dismiss()
        case .select(let target): selected = target
        case .create: creating = true
        case .reset(let profile): resettingTarget = profile
        case .delete(let profile): deleting = profile
        case .restart(let profile): workspace.openEnvironment(profile); dismiss()
        case .open(let profile, let maintenance):
            if !maintenance, workspace.selectedEnvironmentID == profile.id, workspace.local?.local != nil { workspace.openLocal() }
            else { workspace.openEnvironment(profile, maintenance: maintenance) }
            dismiss()
        }
    }
}

private struct AppSettingsContent: View {
    @ObservedObject var workspace: WorkspaceModel
    @AppStorage(InterfaceStyle.defaultsKey) private var interfaceStyle = "system"
    @State private var collectingAndroidDiagnostics = false
    @State private var androidDiagnosticsURL: URL?
    @State private var diagnosticsError: String?

    var body: some View {
        Form {
            Section(L10n.text("外观")) {
                Picker(L10n.text("界面模式"), selection: $interfaceStyle) {
                    ForEach(InterfaceStyle.allCases) { style in
                        Text(L10n.text(style.title)).tag(style.rawValue)
                    }
                }.accessibilityIdentifier("interface-style")
                Text(L10n.text("仅改变 Trames 界面，不改变环境内应用的主题。")).font(.caption).foregroundStyle(.secondary)
            }
            Section(L10n.text("内存与后台")) {
                let available = WorkspaceModel.currentAvailableMemoryBytes()
                if available > 0 {
                    LabeledContent(L10n.text("当前设备可用内存"), value: ByteCountFormatter.string(fromByteCount: Int64(available), countStyle: .binary))
                }
                Text(L10n.text("进入后台后暂停本地 Linux；返回前台后继续。当设备可用内存不足时，会自动保留安全余量或保护性停止环境以防止闪退。")).font(.caption).foregroundStyle(.secondary)
            }
            if FileManager.default.fileExists(atPath: PreparationDiagnostics.logURL.path) {
                Section(L10n.text("诊断")) {
                    ShareLink(item: PreparationDiagnostics.logURL) {
                        Label(L10n.text("共享环境准备日志"), systemImage: "square.and.arrow.up")
                    }
                    if workspace.selectedEnvironment.bootTarget == "android", let root = RootfsLocations.sharedRoot {
                        Button(L10n.text("采集 Android 诊断日志")) {
                            let profile = workspace.selectedEnvironment
                            let local = workspace.local?.local
                            collectingAndroidDiagnostics = true
                            Task {
                                defer { collectingAndroidDiagnostics = false }
                                do {
                                    androidDiagnosticsURL = try await Task.detached(priority: .utility) {
                                        var warning: String?
                                        do { try local?.captureAndroidLogcat() }
                                        catch { warning = error.localizedDescription }
                                        return try AndroidDiagnostics.capture(root: root, environment: profile, logcatError: warning)
                                    }.value
                                } catch { diagnosticsError = error.localizedDescription }
                            }
                        }.disabled(collectingAndroidDiagnostics)
                        if collectingAndroidDiagnostics { ProgressView() }
                        if let androidDiagnosticsURL {
                            ShareLink(item: androidDiagnosticsURL) {
                                Label(L10n.text("共享 Android 诊断日志"), systemImage: "square.and.arrow.up")
                            }
                        }
                        if let diagnosticsError { Text(diagnosticsError).foregroundStyle(.red) }
                    }
                }
            }
        }
        .navigationTitle(L10n.text("通用设置"))
    }
}

private struct CreateEnvironmentView: View {
    @ObservedObject var workspace: WorkspaceModel
    let created: (RootfsEnvironment) -> Void
    @Environment(\.dismiss) private var dismiss
    @State private var imported: EnvironmentConfiguration?
    @State private var choosingTOML = false
    @State private var name = "Debian"
    @State private var distribution = "debian"
    @State private var target = "linux"
    @State private var boot = "shell"
    @State private var packages = "standard"
    @State private var archive: URL?
    @State private var sha256 = ""
    @State private var choosing = false
    @State private var busy = false
    @State private var failure: String?
    var body: some View {
        NavigationStack {
            Form {
                Section {
                    Button(L10n.text("从 TOML 配置创建…"), systemImage: "doc.badge.plus") { choosingTOML = true }
                    if imported != nil { Label(L10n.text("已导入配置；创建后可继续调整"), systemImage: "checkmark.circle").foregroundStyle(.secondary) }
                    TextField(L10n.text("环境名称"), text: $name).accessibilityIdentifier("environment-name")
                    #if TRAMES_ENABLE_ANDROID
                    Picker(L10n.text("系统"), selection: $target) { Text("Linux").tag("linux"); Text("Android").tag("android") }.disabled(imported != nil)
                        .onChange(of: target) { _, value in guard imported == nil else { return }; distribution = value == "android" ? "android" : "debian"; name = value == "android" ? "Android" : "Debian"; boot = "shell"; archive = nil }
                    #endif
                    if target == "linux" {
                        Picker(L10n.text("发行版"), selection: $distribution) { Text("Alpine").tag("alpine"); Text("Debian 13").tag("debian"); Text(L10n.text("自定义 rootfs")).tag("custom") }.disabled(imported != nil)
                            .onChange(of: distribution) { _, value in guard imported == nil else { return }; name = value == "custom" ? "Linux" : value.capitalized; if value == "alpine" { boot = "shell" } }
                    }
                    if target == "linux", distribution != "alpine" {
                        Picker(L10n.text("启动方式"), selection: $boot) { Text("Shell").tag("shell"); Text("systemd").tag("systemd") }
                    }
                    if distribution == "debian" {
                        Picker(L10n.text("预装软件"), selection: $packages) { Text(L10n.text("日常工具")).tag("standard"); Text(L10n.text("开发工具")).tag("development"); Text(L10n.text("基础环境")).tag("minimal") }
                        Text(L10n.text("默认包含 Git、SSH 客户端、编辑器、下载和网络工具。选择 systemd 后，会自动安装 systemd 和 D-Bus。")).font(.caption).foregroundStyle(.secondary)
                    }
                    if ["custom", "android"].contains(distribution) {
                        Button(archive?.lastPathComponent ?? L10n.text("从 Files 选择 rootfs")) { choosing = true }
                        TextField(L10n.text("SHA-256（可选）"), text: $sha256).textInputAutocapitalization(.never).autocorrectionDisabled()
                        Text(distribution == "android" ? L10n.text("导入 Trames Android ARM64 适配包，首次安装会下载官方 Waydroid 镜像。Android 桌面仍为实验性；iPad 真机兼容性待验证。") : L10n.text("支持 ARM64 Linux 的 tar / tar.gz，需包含 /bin/sh、setsid 及其运行库。systemd 模式还需预装 systemd 和 D-Bus。导入使用独立副本。")).font(.caption).foregroundStyle(.secondary)
                    }
                }
                if let failure { Section { Text(failure).foregroundStyle(.red) } }
            }.safeAreaInset(edge: .bottom) {
                Button(busy ? L10n.text("正在导入…") : L10n.text("创建环境")) {
                    busy = true
                    Task { do { let profile = try await workspace.createEnvironment(name: name, distribution: distribution, initMode: boot, packages: packages, archive: archive, sha256: sha256, configuration: imported); dismiss(); created(profile) } catch { failure = error.localizedDescription; busy = false } }
                }.disabled(busy || name.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty || ["custom", "android"].contains(distribution) && archive == nil).accessibilityIdentifier("confirm-create-environment")
                    .buttonStyle(.borderedProminent).frame(maxWidth: .infinity).padding().background(.bar)
            }.disabled(busy).navigationTitle(L10n.text("新建环境")).toolbar { Button(L10n.text("取消")) { dismiss() }.disabled(busy) }
                .fileImporter(isPresented: $choosingTOML, allowedContentTypes: [.plainText, .data]) { result in
                    do {
                        let url = try result.get(), access = url.startAccessingSecurityScopedResource(); defer { if access { url.stopAccessingSecurityScopedResource() } }
                        guard (try url.resourceValues(forKeys: [.fileSizeKey])).fileSize ?? Int.max <= 65536 else { throw TramesError.message(L10n.text("配置文件不能超过 64 KiB")) }
                        let value = try EnvironmentConfiguration(toml: String(contentsOf: url, encoding: .utf8))
                        let distro = value.string("image.distribution")
                        #if TRAMES_ENABLE_ANDROID
                        let allowedDistros = ["alpine", "debian", "custom", "android"]
                        #else
                        let allowedDistros = ["alpine", "debian", "custom"]
                        #endif
                        guard allowedDistros.contains(distro) else { throw TramesError.message(L10n.text("环境配置无效")) }
                        imported = value; name = value.string("environment.name"); sha256 = value.string("image.sha256"); distribution = distro; target = value.string("environment.type"); boot = value.string("boot.mode") == "systemd" ? "systemd" : "shell"
                    } catch { failure = error.localizedDescription }
                }
                .fileImporter(isPresented: $choosing, allowedContentTypes: [.data, .archive, .item]) { result in
                    do { archive = try result.get() } catch { failure = error.localizedDescription }
                }
        }.interactiveDismissDisabled(busy).presentationDetents([.large])
    }
}
