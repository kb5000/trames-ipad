// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import UniformTypeIdentifiers
struct EnvironmentManagerView: View {
    @ObservedObject var workspace:WorkspaceModel
    @Environment(\.dismiss) private var dismiss
    @State private var creating=false
    @State private var failure:String?
    var body:some View {
        NavigationStack {
            List {
                Section {
                    ForEach(workspace.profiles) {profile in
                        VStack(alignment:.leading,spacing:8) {
                            HStack {
                                Text(profile.name).font(.headline)
                                Spacer()
                                if profile.id==workspace.selectedEnvironmentID {Text(L10n.text("当前")).foregroundStyle(.mint)}
                            }
                            Text("\(profile.distro=="custom" ? L10n.text("自定义 ARM64 Linux") : profile.distro.capitalized) · \(profile.bootTarget == "android" ? "Android" : profile.boot=="systemd" ? "systemd" : "Shell")").font(.caption).foregroundStyle(.secondary)
                            HStack {
                                Button(L10n.text("打开")) {workspace.openEnvironment(profile);dismiss()}.buttonStyle(.borderedProminent)
                                if profile.bootTarget == "linux" {
                                Menu(L10n.text("更多")) {
                                    Button(L10n.text("打开维护终端")) {workspace.openEnvironment(profile,maintenance:true);dismiss()}
                                    if profile.distro != "alpine" {
                                        Button(profile.boot=="systemd" ? L10n.text("改为 Shell 启动") : L10n.text("改为 systemd 启动")) {
                                            Task {do {try await workspace.configureBoot(profile,mode:profile.boot=="systemd" ? "shell" : "systemd")} catch {failure=error.localizedDescription}}
                                        }
                                    }
                                }.buttonStyle(.bordered)
                                }
                            }
                        }.padding(.vertical,4)
                    }
                } footer:{Text(L10n.text("各环境独立保存文件和软件。切换时停止当前环境；进入后台时暂停，返回前台继续。启动方式修改后，下次打开生效。"))}
                Section {Button(L10n.text("新建环境"),systemImage:"plus") {creating=true}.accessibilityIdentifier("create-linux-environment")}
            }.navigationTitle(L10n.text("环境"))
                .toolbar {Button(L10n.text("完成")) {dismiss()}}
                .task {do {try await workspace.refreshEnvironments()} catch {failure=error.localizedDescription}}
                .sheet(isPresented:$creating) {CreateEnvironmentView(workspace:workspace) {dismiss()}}
                .alert("Trames",isPresented:Binding(get:{failure != nil},set:{if !$0 {failure=nil}})) {Button(L10n.text("好")) {failure=nil}} message:{Text(failure ?? "")}
        }
    }
}
private struct CreateEnvironmentView:View {
    @ObservedObject var workspace:WorkspaceModel
    let opened:()->Void
    @Environment(\.dismiss) private var dismiss
    @State private var name="Debian"
    @State private var distribution="debian"
    @State private var target="linux"
    @State private var boot="shell"
    @State private var packages="standard"
    @State private var archive:URL?
    @State private var sha256=""
    @State private var choosing=false
    @State private var busy=false
    @State private var failure:String?
    var body:some View {
        NavigationStack {
            Form {
                Section {
                    TextField(L10n.text("环境名称"),text:$name).accessibilityIdentifier("environment-name")
                    Picker(L10n.text("系统"),selection:$target) {Text("Linux").tag("linux");Text("Android").tag("android")}
                        .onChange(of:target) {_,value in distribution=value == "android" ? "android" : "debian";name=value == "android" ? "Android" : "Debian";boot="shell";archive=nil}
                    if target == "linux" {
                    Picker(L10n.text("发行版"),selection:$distribution) {Text("Alpine").tag("alpine");Text("Debian 13").tag("debian");Text(L10n.text("自定义 rootfs")).tag("custom")}
                        .onChange(of:distribution) {_,value in name=value=="custom" ? "Linux" : value.capitalized;if value=="alpine" {boot="shell"}}
                    }
                    if target == "linux",distribution != "alpine" {
                        Picker(L10n.text("启动方式"),selection:$boot) {Text("Shell").tag("shell");Text("systemd").tag("systemd")}
                    }
                    if distribution=="debian" {
                        Picker(L10n.text("预装软件"),selection:$packages) {Text(L10n.text("日常工具")).tag("standard");Text(L10n.text("开发工具")).tag("development");Text(L10n.text("基础环境")).tag("minimal")}
                        Text(L10n.text("默认包含 Git、SSH 客户端、编辑器、下载和网络工具。选择 systemd 后，会自动安装 systemd 和 D-Bus。")).font(.caption).foregroundStyle(.secondary)
                    }
                    if ["custom","android"].contains(distribution) {
                        Button(archive?.lastPathComponent ?? L10n.text("从 Files 选择 rootfs")) {choosing=true}
                        TextField(L10n.text("SHA-256（可选）"),text:$sha256).textInputAutocapitalization(.never).autocorrectionDisabled()
                        Text(distribution == "android" ? L10n.text("导入可选 Trames Android ARM64 安装包。Android 桌面仍为实验性；iPad 真机兼容性待验证。") : L10n.text("支持 ARM64 Linux 的 tar / tar.gz，需包含 /bin/sh、setsid 及其运行库。systemd 模式还需预装 systemd 和 D-Bus。导入使用独立副本。")).font(.caption).foregroundStyle(.secondary)
                    }
                }
                if let failure {Section {Text(failure).foregroundStyle(.red)}}
                Section {Button(busy ? L10n.text("正在导入…") : L10n.text("创建并打开")) {
                    busy=true
                    Task {do {try await workspace.createEnvironment(name:name,distribution:distribution,initMode:boot,packages:packages,archive:archive,sha256:sha256);dismiss();opened()} catch {failure=error.localizedDescription;busy=false}}
                }.disabled(busy || name.trimmingCharacters(in:.whitespacesAndNewlines).isEmpty || ["custom","android"].contains(distribution) && archive==nil).accessibilityIdentifier("confirm-create-environment")}
            }.disabled(busy).navigationTitle(L10n.text("新建环境")).toolbar {Button(L10n.text("取消")) {dismiss()}.disabled(busy)}
                .fileImporter(isPresented:$choosing,allowedContentTypes:[.data,.archive,.item]) {result in
                    do {archive=try result.get()} catch {failure=error.localizedDescription}
                }
        }.interactiveDismissDisabled(busy)
    }
}
