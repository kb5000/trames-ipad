// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import TramesKit
@main
struct TramesApp: App {
    @StateObject private var workspace = WorkspaceModel()
    @AppStorage(InterfaceStyle.defaultsKey) private var interfaceStyle = "system"
    @Environment(\.scenePhase) private var scenePhase
    var body: some Scene {
        WindowGroup {
            GeometryReader { proxy in
                WorkspaceKeyboardContainer(workspace: workspace)
                    .frame(width:proxy.size.width,height:proxy.size.height)
            }
                .ignoresSafeArea(.keyboard)
                .preferredColorScheme((InterfaceStyle(rawValue:interfaceStyle) ?? .system).colorScheme)
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
    #if TRAMES_ENABLE_PIP
    @ObservedObject private var pictureInPicture:WindowPictureInPicture
    init(workspace:WorkspaceModel) {self.workspace=workspace;self.pictureInPicture=workspace.pictureInPicture}
    #else
    init(workspace:WorkspaceModel) {self.workspace=workspace}
    #endif
    @State private var managerSheet = false
    @State private var managerDestination: ManagerDestination?
    @State private var frames: [String:CGRect] = [:]
    @State private var draggedTab: WorkspaceItemID?
    @State private var dragLocation = CGPoint.zero
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("TRAMES").font(.system(.headline, design: .monospaced)).foregroundStyle(Color.accentColor)
                ScrollView(.horizontal,showsIndicators:false) {
                    HStack(spacing:4) {
                        ForEach(workspace.layout.tabs,id:\.self) { item in
                            if workspace.session(for: item) != nil || workspace.webTab(for: item) != nil {
                                HStack(spacing:0) {
                                    Button { workspace.select(item) } label: {
                                        WorkspaceTabLabel(workspace: workspace, item: item).lineLimit(1)
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
                                .background(workspace.layout.selected == item ? Color.accentColor.opacity(0.14) : .clear)
                                .clipShape(RoundedRectangle(cornerRadius:6))
                                .workspaceFrame(item.token)
                            }
                        }
                    }
                }.workspaceFrame("tabs")
                #if TRAMES_ENABLE_PIP
                if let active=workspace.layout.focused ?? workspace.layout.selected,pictureInPicture.supported {
                    Button {workspace.preview(active)} label: {Image(systemName:pictureInPicture.item == active ? "pip.exit" : "pip.enter").frame(width:44,height:44)}
                        .accessibilityLabel(L10n.text("画中画")).accessibilityIdentifier("window-pip")
                }
                #endif
                Button { openManager(destination: nil) } label: {
                    Label(L10n.text("环境与设置"), systemImage: "slider.horizontal.3")
                        .labelStyle(.iconOnly).frame(minWidth: 44, minHeight: 44)
                }.accessibilityIdentifier("open-manager")
                Menu {
                    Button {
                        if workspace.profiles.isEmpty { openManager(destination: nil) }
                        else { workspace.openLocal() }
                    } label: {
                        Label(L10n.text("新建终端"), systemImage: "terminal")
                    }
                    .accessibilityIdentifier("new-terminal-action")
                    Button {
                        workspace.openWebBrowser()
                    } label: {
                        Label(L10n.text("内置浏览器"), systemImage: "globe")
                    }
                    .accessibilityIdentifier("new-browser-action")
                } label: {
                    Label(L10n.text("新建"), systemImage: "plus")
                        .labelStyle(.iconOnly).frame(minWidth: 44, minHeight: 44)
                }.accessibilityIdentifier("session-actions")

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
                        if workspace.profiles.isEmpty {
                            ContentUnavailableView {Label(L10n.text("尚未创建环境"),systemImage:"square.stack.3d.up")} actions: {Button(L10n.text("新建环境")) {openManager(destination: nil)}}
                        } else {ContentUnavailableView(L10n.text("打开一个终端"),systemImage:"terminal",description:Text(L10n.text("从顶部标签栏恢复窗口，或打开环境与设置。")))}
                    }
                }
            .onAppear { workspace.updateViewport(proxy.size) }
            .onChange(of:proxy.size) { _,size in workspace.updateViewport(size) }
            }.workspaceFrame("body")
        }
        .coordinateSpace(name:"workspace")
        .onReceive(NotificationCenter.default.publisher(for:Notification.Name("TramesShowEnvironments"))) { _ in openManager(destination: nil) }
        .onPreferenceChange(WorkspaceFramesKey.self) { frames=$0 }
        .overlay(alignment:.topLeading) {
            if let item=draggedTab, workspace.session(for: item) != nil || workspace.webTab(for: item) != nil {
                WorkspaceTabLabel(workspace: workspace, item: item).padding(10).background(.ultraThinMaterial)
                    .clipShape(RoundedRectangle(cornerRadius:6)).offset(x:dragLocation.x-40,y:dragLocation.y-18).allowsHitTesting(false)
            }
        }
        .overlay(alignment: .top) {
            if let notice = workspace.inboxNotice {
                HStack(spacing: 10) {
                    Image(systemName: "checkmark.circle.fill").foregroundStyle(.green)
                    Text(notice.message).font(.callout).lineLimit(2)
                    Button(L10n.text("复制路径")) {
                        UIPasteboard.general.string = notice.guestPath
                        workspace.dismissInboxNotice()
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                    Button {
                        workspace.dismissInboxNotice()
                    } label: {
                        Image(systemName: "xmark").font(.caption)
                    }
                    .buttonStyle(.plain)
                    .accessibilityLabel(L10n.text("关闭"))
                }
                .padding(.horizontal, 14)
                .padding(.vertical, 10)
                .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 12))
                .shadow(radius: 6, y: 2)
                .padding(.top, 56)
                .padding(.horizontal, 16)
                .transition(.move(edge: .top).combined(with: .opacity))
            }
        }
        #if TRAMES_ENABLE_PIP
        .overlay(alignment:.bottomTrailing) {
            PictureInPictureSource(controller:pictureInPicture)
                .frame(width:pictureInPicture.item != nil ? 160 : 1,height:pictureInPicture.item != nil ? 100 : 1)
                .opacity(pictureInPicture.item != nil ? 1 : 0.01).allowsHitTesting(false).accessibilityHidden(true)
        }
        #endif
        .background(Color(uiColor:.systemBackground))
        .fullScreenCover(isPresented:$managerSheet) {EnvironmentManagerView(workspace:workspace, initialDestination: managerDestination)}
        .alert("Trames", isPresented: Binding(get: { workspace.error != nil }, set: { if !$0 { workspace.error = nil } })) {
            Button(L10n.text("好")) { workspace.error = nil }
        } message: { Text(workspace.error ?? "") }
    }
    private func openManager(destination: ManagerDestination?) {
        managerDestination = destination
        managerSheet = true
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
            HStack(spacing: 10) {
                Text(session.state).lineLimit(1).accessibilityIdentifier("session-state")
                Spacer(minLength: 8)
                if let cpu = session.cpuPercent {
                    Text("CPU \(cpu)%").monospacedDigit().accessibilityIdentifier("cpu-usage")
                        .accessibilityHint(L10n.text("App CPU 用量，一个核心满载为百分之百"))
                }
                if let memory = session.memoryBytes {
                    Text(L10n.text("内存 %@", ByteCountFormatter.string(fromByteCount: Int64(memory), countStyle: .binary)))
                        .monospacedDigit().accessibilityIdentifier("memory-usage")
                }
                if let capacity = session.capacity {
                    Text(L10n.text("磁盘 %@ / %@",
                                   ByteCountFormatter.string(fromByteCount: Int64(capacity.used_bytes), countStyle: .binary),
                                   ByteCountFormatter.string(fromByteCount: Int64(capacity.budget_bytes), countStyle: .binary)))
                        .monospacedDigit().accessibilityIdentifier("disk-usage")
                }
            }.font(.caption).foregroundStyle(.secondary).padding(.horizontal,12)
                .padding(.vertical,6).fixedSize(horizontal:false,vertical:true)
                .layoutPriority(1)
        }
    }
}
