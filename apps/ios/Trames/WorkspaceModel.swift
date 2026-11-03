// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import TramesKit
@MainActor
final class WorkspaceModel: ObservableObject {
    let pictureInPicture=WindowPictureInPicture()
    @Published var sessions: [TerminalSession] = []
    @Published private(set) var localOwner: TerminalSession?
    @Published private(set) var profiles = RootfsCatalog.alpine.environments
    @Published private(set) var selectedEnvironmentID = "alpine"
    var selectedEnvironment:RootfsEnvironment {profiles.first(where:{$0.id==selectedEnvironmentID}) ?? RootfsCatalog.alpine.environments[0]}
    private var terminalSequence = 0
    @Published private(set) var layout = WorkspaceTabs()
    private var observedModes: [WorkspaceItemID:String] = [:]
    private var pendingModes: [WorkspaceItemID:String] = [:]
    private var dialogSizes: [WorkspaceItemID:CGSize] = [:]
    /// Stable identity/order across tab, floating and background placement.
    var items: [WorkspaceItemID] {
        let terminals=sessions.map { WorkspaceItemID.terminal($0.id) }
        let graphics=environmentOwners.flatMap { session in session.waylandWindows.map { WorkspaceItemID.wayland(session.id,$0.id) } }
        return (terminals+graphics).filter { layout.placement($0) != nil }
    }
    func isVisible(_ id: WorkspaceItemID) -> Bool {
        layout.placement(id) == .floating || layout.selected == id
    }
    @Published var error: String?
    @Published var resetting = false
    private var appActive = true
    private(set) var environment: EnvironmentStore?
    private(set) var directories: ScopedDirectories?
    init(prepareEnvironment: Bool = true) {
        pictureInPicture.onActivityChanged={ [weak self] in self?.applyActivityPolicy() }
        pictureInPicture.onRestore={ [weak self] in self?.activate($0) }
        pictureInPicture.onError={ [weak self] in self?.error=$0 }

        #if DEBUG
        if ProcessInfo.processInfo.arguments.contains("--workspace-ui-acceptance") || ProcessInfo.processInfo.arguments.contains("--keyboard-ui-acceptance") {
            let session=TerminalSession(title:"Alpine",isLocal:true)
            session.state="工作区交互验收";add(session)
            session.terminal.feed(text:"\u{1b}]0;\u{7}")
            if ProcessInfo.processInfo.arguments.contains("--keyboard-ui-acceptance") { return }
            let data=Data("[{\"id\":7,\"connection\":1,\"title\":\"测试图形窗口\",\"app_id\":\"workspace-test\",\"width\":400,\"height\":280,\"revision\":1,\"mode\":\"floating\",\"bounds\":[-32,-32,464,344]}]".utf8)
            session.waylandWindows=(try? JSONDecoder().decode([LocalWaylandWindow].self,from:data)) ?? []
            session.onWaylandChanged?(session.waylandWindows)
            return
        }
        #endif
        guard prepareEnvironment else { return }
        FileProviderService.observeSaves { [weak self] in
            guard let self,self.appActive else {return}
            self.localOwner?.pause(false)
        }
        do {
            let environment = try EnvironmentStore(); self.environment = environment
            directories = try ScopedDirectories(file: environment.authorizationRoot.appendingPathComponent("folder-grants.json"))
            Task {
                do {
                    profiles=try await environment.profiles()
                    let saved=UserDefaults.standard.string(forKey:"activeEnvironmentID") ?? "alpine"
                    selectedEnvironmentID=profiles.contains(where:{$0.id==saved}) ? saved : "alpine"
                    openLocal()
                } catch {self.error=error.localizedDescription}
            }
        } catch { self.error = error.localizedDescription }
    }
    var current: TerminalSession? { layout.selected.flatMap { session(for: $0) } ?? local }
    var local: TerminalSession? { localOwner }
    private var environmentOwners: [TerminalSession] {
        var owners=sessions.filter { $0.environmentOwner == nil }
        if let localOwner,!owners.contains(where:{$0.id == localOwner.id}) {owners.append(localOwner)}
        return owners
    }
    func session(for id: WorkspaceItemID) -> TerminalSession? {
        sessions.first { $0.id == id.session } ?? (localOwner?.id == id.session ? localOwner : nil)
    }
    func window(for id: WorkspaceItemID) -> LocalWaylandWindow? {
        guard case .wayland(_, let window) = id else { return nil }
        return session(for:id)?.waylandWindows.first { $0.id == window }
    }
    func add(_ session: TerminalSession) {
        session.onFileProviderError = { [weak self] message in self?.error=message }
        if session.isLocal,session.environmentOwner == nil,localOwner == nil {localOwner=session;terminalSequence=max(1,terminalSequence)}
        session.terminal.keyboardVisibility = { [weak self, weak session] in
            guard let self, let session else { return false }
            return self.isVisible(.terminal(session.id))
        }
        session.onWaylandChanged = { [weak self, weak session] windows in
            guard let self, let session else { return }; self.synchronize(session, windows:windows)
        }
        sessions.append(session)
        change { $0.insert(.terminal(session.id), as:.tabbed) }
    }
    private func change(protocolFocus: Bool = true, _ update: (inout WorkspaceTabs) -> Void) {
        var next=layout; update(&next)
        guard next != layout else { return }
        let old=layout.focused
        if old != next.focused, let old, case .terminal = old, let session=session(for:old) {
            // Resign while the old host is still visible, before publishing the
            // empty/minimized layout and changing keyboard safe-area geometry.
            let visible=next.placement(old) == .floating || next.selected == old
            session.terminal.setKeyboardFocus(false,visible:visible)
        }
        layout=next
        if old != next.focused {
            for session in environmentOwners where session.isLocal {
                if let focused=next.focused, case .wayland(let owner, let window) = focused, owner == session.id {
                    if protocolFocus { session.waylandCommand(["op":"focus","window":window]) }
                } else { session.waylandCommand(["op":"focus","window":NSNull()]) }
            }
        }
    }
    func focus(_ id: WorkspaceItemID, protocolFocus: Bool = true) { change(protocolFocus:protocolFocus) { $0.focus(id) } }
    func activate(_ id: WorkspaceItemID) {
        if layout.placement(id) == .minimized { restore(id) }
        else if layout.placement(id) == .tabbed { select(id) }
        else { focus(id) }
    }
    func select(_ id: WorkspaceItemID) {
        if layout.placement(id) == .minimized { restore(id) }
        else { change { $0.select(id) } }
    }
    func dock(_ id: WorkspaceItemID, before target: WorkspaceItemID? = nil) {
        change { $0.dock(id,before:target) }; nativeMode(id,"maximized")
    }
    func detach(_ id: WorkspaceItemID, at point: CGPoint? = nil) {
        change { $0.detach(id,at:point) }; nativeMode(id,"floating")
    }
    func minimize(_ id: WorkspaceItemID) { change { $0.minimize(id) }; nativeMode(id,"minimized") }
    func restore(_ id: WorkspaceItemID) {
        change { $0.restore(id) }
        nativeMode(id,layout.placement(id) == .tabbed ? "maximized" : "floating")
    }
    func updateViewport(_ size:CGSize) { change { $0.updateViewport(size) } }
    func saveFrame(_ id: WorkspaceItemID, _ frame: DisplayWindowGeometry) { change { $0.frames[id]=frame } }
    func closeItem(_ id: WorkspaceItemID) {
        if pictureInPicture.item == id {pictureInPicture.stop()}
        if case .terminal = id, let session=session(for:id) { close(session) }
        else if case .wayland(_,let window)=id { session(for:id)?.waylandCommand(["op":"close","window":window]) }
    }
    private func nativeMode(_ id: WorkspaceItemID, _ mode: String) {
        if case .wayland(_,let window)=id {
            pendingModes[id]=mode
            session(for:id)?.waylandMode(window,mode)
        }
    }
    private func synchronize(_ session: TerminalSession, windows: [LocalWaylandWindow]) {
        let live=Set(windows.map { WorkspaceItemID.wayland(session.id,$0.id) })
        change { next in
            for id in Array(observedModes.keys) where id.session == session.id && !live.contains(id) {
                next.close(id); observedModes.removeValue(forKey:id); pendingModes.removeValue(forKey:id)
                dialogSizes.removeValue(forKey:id)
            }
            for window in windows {
                let id=WorkspaceItemID.wayland(session.id,window.id)
                let oldMode=observedModes.updateValue(window.mode,forKey:id)
                // Ignore snapshots queued before a host placement request.
                // A matching acknowledgement must not select/focus it again.
                if let pending=pendingModes[id] {
                    if pending == window.mode { pendingModes.removeValue(forKey:id) }
                    continue
                }
                let placement:WorkspaceTabs.Placement = window.mode == "maximized" ? .tabbed : window.mode == "minimized" ? .minimized : .floating
                if next.placement(id) == nil { next.insert(id,as:placement) }
                else if oldMode != window.mode && next.placement(id) != placement {
                    switch placement {
                    case .tabbed: next.dock(id)
                    case .floating: next.detach(id)
                    case .minimized: next.minimize(id)
                    }
                }
                if window.parent != nil && next.placement(id) == .floating {
                    let size=CGSize(width:CGFloat(window.width),height:CGFloat(window.height))
                    if dialogSizes[id] != size {
                        let initial=dialogSizes[id] == nil
                        dialogSizes[id]=size
                        var frame=next.frames[id] ?? DisplayWindowGeometry()
                        frame.size=CGSize(width:size.width,height:size.height+44)
                        if initial {
                            let parent=WorkspaceItemID.wayland(session.id,window.parent!)
                            let parentFrame=next.placement(parent) == .floating ? next.frames[parent] : nil
                            let origin=parentFrame?.origin ?? .zero
                            let parentSize=parentFrame?.size ?? next.viewport
                            frame.origin=CGPoint(x:origin.x+(parentSize.width-frame.size.width)/2,y:origin.y+(parentSize.height-frame.size.height)/2)
                        }
                        frame.constrain(to:next.viewport,minimum:CGSize(width:120,height:88))
                        next.frames[id]=frame
                    }
                }
            }
        }
    }
    func openLocal() {
        guard !resetting else { return }
        if let owner=localOwner {
            terminalSequence+=1
            let session=TerminalSession(title:"\(owner.environmentName) \(terminalSequence)",isLocal:true)
            session.attachTerminal(to:owner)
            session.pause(!appActive)
            add(session)
            return
        }
        guard let environment else { return }
        let session = TerminalSession(title:selectedEnvironment.name,isLocal:true)
        session.pause(!appActive)
        session.bridge.directories = directories
        add(session)
        session.startLocal(environment,useVolumeBudget:UserDefaults.standard.bool(forKey:"useVolumeBudget"),profile:selectedEnvironment)
    }
    private func stopLocal() {
        let localSessions=sessions.filter(\.isLocal)
        for session in localSessions {session.close()}
        localOwner?.close()
        let ids=Set(localSessions.map(\.id)+[localOwner?.id].compactMap{$0})
        sessions.removeAll(where: { $0.isLocal });change {next in for id in ids {next.closeSession(id)}}
        localOwner=nil;terminalSequence=0
        observedModes=observedModes.filter{!ids.contains($0.key.session)}
        pendingModes=pendingModes.filter{!ids.contains($0.key.session)}
        dialogSizes=dialogSizes.filter{!ids.contains($0.key.session)}
    }
    func refreshEnvironments() async throws {
        if let environment {profiles=try await environment.profiles()}
    }
    func openEnvironment(_ profile:RootfsEnvironment,maintenance:Bool=false) {
        guard let environment,!resetting else {return}
        let previous=localOwner
        stopLocal();selectedEnvironmentID=profile.id
        UserDefaults.standard.set(profile.id,forKey:"activeEnvironmentID")
        let session=TerminalSession(title:profile.name,isLocal:true)
        session.pause(!appActive);session.bridge.directories=directories;add(session)
        session.startLocal(environment,useVolumeBudget:UserDefaults.standard.bool(forKey:"useVolumeBudget"),profile:profile,maintenance:maintenance,after:previous)
    }
    func createEnvironment(name:String,distribution:String,initMode:String,packages:String,archive:URL?,sha256:String?) async throws {
        guard let environment else {throw TramesError.message(L10n.text("环境存储不可用"))}
        let profile=try await environment.create(name:name,distribution:distribution,initMode:initMode,packages:packages,archive:archive,sha256:sha256)
        try await refreshEnvironments();openEnvironment(profile)
    }
    func configureBoot(_ profile:RootfsEnvironment,mode:String) async throws {
        guard let environment else {return}
        _=try await environment.setBoot(profile,mode:mode);try await refreshEnvironments()
    }
    func resetLocal() {
        guard let environment,!resetting else {return}
        let profile=selectedEnvironment,previous=localOwner
        resetting=true;stopLocal()
        Task {
            do {await previous?.waitUntilClosed();try await environment.reset(profile:profile);try await refreshEnvironments();resetting=false;openLocal()}
            catch {resetting=false;self.error=error.localizedDescription}
        }
    }
    func close(_ session: TerminalSession) {
        if pictureInPicture.item?.session == session.id {pictureInPicture.stop()}
        session.closeTerminalWindow(); sessions.removeAll { $0.id == session.id }
        change { $0.close(.terminal(session.id)) }
    }
    func preview(_ item:WorkspaceItemID) {
        pictureInPicture.toggle(item) { [weak self] in self?.session(for:item) }
    }
    func inactive() {
        // UIKit announces deactivation before it restricts new Metal work.
        for session in environmentOwners {try? session.local?.suspendGPU(true)}
    }
    private func applyActivityPolicy() {
        let selected=pictureInPicture.item.flatMap {session(for:$0)}
        let owner=selected?.environmentOwner ?? selected
        for session in environmentOwners {
            let preview=pictureInPicture.keepsRunning && owner?.id == session.id
            session.pause(!appActive && !preview,commitEdits:appActive)
        }
    }
    func background() {
        appActive=false;inactive()
        Task {await environment?.pausePreparation(true)}
        applyActivityPolicy()
    }
    func foreground() {
        appActive=true
        for session in environmentOwners {try? session.local?.suspendGPU(false)}
        Task {await environment?.pausePreparation(false)}
        applyActivityPolicy()
    }
}
