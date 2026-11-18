// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import TramesKit
@main
struct TramesApp: App {
    @StateObject private var workspace = WorkspaceModel()
    @Environment(\.scenePhase) private var scenePhase
    var body: some Scene {
        WindowGroup {
            GeometryReader { proxy in
                WorkspaceKeyboardContainer(workspace: workspace)
                    .frame(width:proxy.size.width,height:proxy.size.height)
            }
                .ignoresSafeArea(.keyboard)
                .preferredColorScheme(.dark)
                .onChange(of: scenePhase) { _, phase in
                    if phase == .inactive { workspace.inactive() }
                    else if phase == .background { workspace.background() }
                    else if phase == .active { workspace.foreground() }
                }
        }
    }
}
struct WorkspaceView: View {
    @ObservedObject var workspace: WorkspaceModel
    @ObservedObject private var pictureInPicture:WindowPictureInPicture
    init(workspace:WorkspaceModel) {self.workspace=workspace;self.pictureInPicture=workspace.pictureInPicture}
    @State private var filesSheet = false
    @State private var settingsSheet = false
    @State private var environmentsSheet = false
    @State private var sessionActions = false
    @State private var frames: [String:CGRect] = [:]
    @State private var draggedTab: WorkspaceItemID?
    @State private var dragLocation = CGPoint.zero
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("TRAMES").font(.system(.headline, design: .monospaced)).foregroundStyle(.mint)
                ScrollView(.horizontal,showsIndicators:false) {
                    HStack(spacing:4) {
                        ForEach(workspace.layout.tabs,id:\.self) { item in
                            if let session=workspace.session(for:item) {
                                HStack(spacing:0) {
                                    Button { workspace.select(item) } label: {
                                        WorkspaceTabLabel(session:session,item:item).lineLimit(1)
                                            .padding(.horizontal,8).frame(minHeight:44).contentShape(Rectangle())
                                    }.buttonStyle(.plain)
                                        .accessibilityIdentifier("tab-\(item.token)")
                                        .simultaneousGesture(DragGesture(minimumDistance:10,coordinateSpace:.named("workspace"))
                                            .onChanged { value in draggedTab=item;dragLocation=value.location }
                                            .onEnded { value in finishTabDrag(item,at:value.location) })
                                        .contextMenu {
                                            Button(L10n.text("移到浮动窗口"),systemImage:"rectangle.on.rectangle") { workspace.detach(item) }
                                            Button(L10n.text("关闭"),role:.destructive) { workspace.closeItem(item) }
                                        }
                                    Button { workspace.closeItem(item) } label: {
                                        Image(systemName:"xmark").font(.system(size:12))
                                            .frame(width:44,height:44).contentShape(Rectangle())
                                    }.buttonStyle(.plain).accessibilityLabel(L10n.text("关闭标签页"))
                                        .accessibilityIdentifier("close-tab-\(item.token)")
                                }
                                .background(workspace.layout.selected == item ? Color.white.opacity(0.12) : .clear)
                                .clipShape(RoundedRectangle(cornerRadius:6))
                                .workspaceFrame(item.token)
                            }
                        }
                    }
                }.workspaceFrame("tabs")
                if let active=workspace.layout.focused ?? workspace.layout.selected,pictureInPicture.supported {
                    Button {workspace.preview(active)} label: {Image(systemName:pictureInPicture.item == active ? "pip.exit" : "pip.enter").frame(width:44,height:44)}
                        .accessibilityLabel(L10n.text("画中画")).accessibilityIdentifier("window-pip")
                }
                Button { sessionActions = true } label: {
                    Label(L10n.text("新建或管理会话"), systemImage: "plus")
                        .labelStyle(.iconOnly).frame(minWidth: 44, minHeight: 44)
                }.accessibilityIdentifier("session-actions").confirmationDialog(L10n.text("新建或管理会话"), isPresented: $sessionActions, titleVisibility: .visible) {
                    Button(L10n.text("新建终端"),systemImage:"terminal") {workspace.openLocal()}
                    Button(L10n.text("Linux 环境"),systemImage:"square.stack.3d.up") {environmentsSheet=true}
                    Button("Files", systemImage: "folder") { filesSheet = true }
                    Button(L10n.text("设置"), systemImage: "gear") { settingsSheet = true }
                }

            }.padding(.horizontal,12).padding(.vertical,6).workspaceFrame("header")
            GeometryReader { proxy in
                ZStack(alignment:.topLeading) {
                    // A terminal/Metal view has exactly one SwiftUI host for its
                    // entire lifetime. Docking changes geometry, never parents.
                    ForEach(workspace.items,id:\.self) { item in
                        WorkspaceWindow(workspace:workspace,item:item,viewport:proxy.size)
                            .zIndex(workspace.layout.placement(item) == .floating
                                ? Double((workspace.layout.floating.firstIndex(of:item) ?? 0)+2)
                                : workspace.layout.selected == item ? 1 : 0)
                    }
                }.frame(width:proxy.size.width,height:proxy.size.height,alignment:.topLeading)
                .overlay {
                    if workspace.layout.selected == nil && workspace.layout.floating.isEmpty {
                        ContentUnavailableView(L10n.text("打开一个终端"),systemImage:"terminal",description:Text(L10n.text("从顶部标签栏恢复窗口，或选择 Linux 环境。")))
                    }
                }
            .onAppear { workspace.updateViewport(proxy.size) }
            .onChange(of:proxy.size) { _,size in workspace.updateViewport(size) }
            }.workspaceFrame("body")
        }
        .coordinateSpace(name:"workspace")
        .onReceive(NotificationCenter.default.publisher(for:Notification.Name("TramesShowEnvironments"))) { _ in environmentsSheet=true }
        .onPreferenceChange(WorkspaceFramesKey.self) { frames=$0 }
        .overlay(alignment:.topLeading) {
            if let item=draggedTab,let session=workspace.session(for:item) {
                WorkspaceTabLabel(session:session,item:item).padding(10).background(.ultraThinMaterial)
                    .clipShape(RoundedRectangle(cornerRadius:6)).offset(x:dragLocation.x-40,y:dragLocation.y-18).allowsHitTesting(false)
            }
        }
        .overlay(alignment:.bottomTrailing) {
            PictureInPictureSource(controller:pictureInPicture)
                .frame(width:pictureInPicture.item != nil ? 160 : 1,height:pictureInPicture.item != nil ? 100 : 1)
                .opacity(pictureInPicture.item != nil ? 1 : 0.01).allowsHitTesting(false).accessibilityHidden(true)
        }
        .background(.black)
        .sheet(isPresented: $filesSheet) { if let session = workspace.current { TransferView(session: session) } }
        .fullScreenCover(isPresented:$environmentsSheet) {EnvironmentManagerView(workspace:workspace)}
        .sheet(isPresented: $settingsSheet) { SettingsView(workspace: workspace) }
        .alert("Trames", isPresented: Binding(get: { workspace.error != nil }, set: { if !$0 { workspace.error = nil } })) {
            Button(L10n.text("好")) { workspace.error = nil }
        } message: { Text(workspace.error ?? "") }
    }
    private func finishTabDrag(_ item: WorkspaceItemID, at point: CGPoint) {
        defer { draggedTab=nil }
        let body=frames["body"] ?? .zero
        if body.contains(point) {
            workspace.detach(item,at:CGPoint(x:max(0,point.x-body.minX-60),y:max(0,point.y-body.minY-20)))
        } else if (frames["header"] ?? .zero).contains(point) {
            let target=workspace.layout.tabs.first { candidate in
                candidate != item && (frames[candidate.token].map { point.x < $0.midX } ?? false)
            }
            workspace.dock(item,before:target)
        }
    }

}
struct SessionView: View {
    @ObservedObject var workspace: WorkspaceModel
    @ObservedObject var session: TerminalSession
    var body: some View {
        VStack(spacing: 0) {
            GeometryReader { proxy in
                TerminalPane(session:session,workspace:workspace,onFocus:{ [weak workspace, weak session] in if let session { workspace?.focus(.terminal(session.id)) } })
                    .frame(width:proxy.size.width,height:proxy.size.height)
            }.frame(minHeight:0,maxHeight:.infinity).clipped()
            HStack {
                Text(session.state).lineLimit(2).accessibilityIdentifier("session-state")
                Spacer()
                if let cpu = session.cpuPercent {
                    Text("CPU \(cpu)%").monospacedDigit().accessibilityIdentifier("cpu-usage")
                        .accessibilityHint(L10n.text("App CPU 用量，一个核心满载为百分之百"))
                }
                if let capacity = session.capacity {
                    Text("\(ByteCountFormatter.string(fromByteCount: Int64(capacity.used_bytes), countStyle: .binary)) / \(ByteCountFormatter.string(fromByteCount: Int64(capacity.budget_bytes), countStyle: .binary))")
                }
            }.font(.caption).foregroundStyle(.secondary).padding(.horizontal,12)
                .padding(.vertical,6).fixedSize(horizontal:false,vertical:true)
                .layoutPriority(1)
        }
    }
}
struct SettingsView: View {
    @ObservedObject var workspace: WorkspaceModel
    @Environment(\.dismiss) private var dismiss
    @AppStorage("useVolumeBudget") private var volume = false
    @State private var managingEnvironments=false
    @State private var resetConfirmation = false
    @State private var collectingAndroidDiagnostics = false
    @State private var androidDiagnosticsURL:URL?
    @State private var diagnosticsError:String?
    var body: some View {
        NavigationStack {
            Form {
                Section(L10n.text("环境配置")) {
                    LabeledContent(L10n.text("当前环境"),value:workspace.selectedEnvironment.name)
                    Button(L10n.text("管理环境配置"),systemImage:"slider.horizontal.3") {managingEnvironments=true}
                    Text(L10n.text("CPU、内存、原生组件、网络与访问权限分别保存在每个环境的 TOML 配置中。")).font(.caption).foregroundStyle(.secondary)
                }
                Section(L10n.text("本地磁盘")) {
                    Toggle(L10n.text("使用设备卷容量作为存储预算"), isOn: $volume)
                    Text(L10n.text("默认预算 16 GiB；启用后按应用所在卷的总容量计算。实际可用空间仍受系统剩余容量限制，下次打开本地终端生效。")).font(.caption)
                    Button(L10n.text("重置 Linux 环境"),role:.destructive) {resetConfirmation=true}.disabled(workspace.resetting)
                }
                if let directories=workspace.directories {Section(L10n.text("设备目录授权")) {NavigationLink(L10n.text("Files 目录授权")) {FolderGrantsView(store:directories)}}}
                Section(L10n.text("后台行为")) { Text(L10n.text("进入后台后暂停本地 Linux；返回前台后继续。Files 采用独立副本导入和导出。")).font(.caption) }
                if FileManager.default.fileExists(atPath:PreparationDiagnostics.logURL.path) {
                    Section(L10n.text("诊断")) {
                        ShareLink(item:PreparationDiagnostics.logURL) {Label(L10n.text("共享环境准备日志"),systemImage:"square.and.arrow.up")}
                        if workspace.selectedEnvironment.bootTarget == "android",let root=RootfsLocations.sharedRoot {
                            Button(L10n.text("采集 Android 诊断日志")) {
                                let profile=workspace.selectedEnvironment
                                let local=workspace.local?.local
                                collectingAndroidDiagnostics=true
                                Task {
                                    defer {collectingAndroidDiagnostics=false}
                                    do {androidDiagnosticsURL=try await Task.detached(priority:.utility) {
                                        var warning:String?
                                        do {try local?.captureAndroidLogcat()}
                                        catch {warning=error.localizedDescription}
                                        return try AndroidDiagnostics.capture(root:root,environment:profile,logcatError:warning)
                                    }.value}
                                    catch {diagnosticsError=error.localizedDescription}
                                }
                            }.disabled(collectingAndroidDiagnostics)
                            if collectingAndroidDiagnostics {ProgressView()}
                            if let androidDiagnosticsURL {ShareLink(item:androidDiagnosticsURL) {Label(L10n.text("共享 Android 诊断日志"),systemImage:"square.and.arrow.up")}}
                            if let diagnosticsError {Text(diagnosticsError).foregroundStyle(.red)}
                        }
                    }
                }
            }.navigationTitle(L10n.text("设置")).toolbar { Button(L10n.text("完成")) { dismiss() } }
                .fullScreenCover(isPresented:$managingEnvironments) {EnvironmentManagerView(workspace:workspace)}
                .alert(L10n.text("重置 Linux 环境？"),isPresented:$resetConfirmation) {
                    Button(L10n.text("删除并重新初始化"),role:.destructive) {dismiss(); workspace.resetLocal()}
                    Button(L10n.text("取消"),role:.cancel) {}
                } message: {Text(L10n.text("将重置当前环境中的软件、文件和配置。其他 Linux 环境和 Files 目录授权会保留。"))}
        }
    }
}
