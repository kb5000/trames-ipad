// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import UniformTypeIdentifiers
import TramesKit
struct FolderGrantsView: View {
    let store: ScopedDirectories
    @State private var grants: [FolderGrant] = []
    @State private var chooser = false
    @State private var writable = false
    @State private var error = ""
    var body: some View {
        Form {
            Section(L10n.text("添加目录授权")) {
                Toggle(L10n.text("允许 Linux 导出新文件到此目录"), isOn: $writable)
                Button(L10n.text("选择 Files 目录")) { chooser = true }
                Text(L10n.text("默认仅允许导入。每次复制仍会询问；目标已存在时拒绝覆盖，每个文件最多 1 GiB。")).font(.caption)
            }
            Section(L10n.text("已授权目录")) {
                ForEach(grants) { grant in
                    VStack(alignment: .leading, spacing: 4) {
                        Text(grant.name)
                        Text(grant.writable ? L10n.text("导入及导出") : L10n.text("仅导入")).font(.caption)
                        Text(grant.id.uuidString).font(.caption2).textSelection(.enabled)
                        Button(L10n.text("撤销授权"), role: .destructive) {
                            Task { do { try await store.revoke(grant.id); grants = await store.list() } catch { self.error = error.localizedDescription } }
                        }
                    }
                }
                if grants.isEmpty { Text(L10n.text("尚未授权目录")) }
            }
            if !error.isEmpty { Section { Text(error).foregroundStyle(.red) } }
        }.navigationTitle(L10n.text("Files 目录授权"))
            .task { grants = await store.list() }
            .fileImporter(isPresented: $chooser, allowedContentTypes: [.folder]) { result in
                let canWrite = writable
                Task {
                    do { try await store.add(selected: result.get(), writable: canWrite); grants = await store.list(); error = "" }
                    catch { self.error = error.localizedDescription }
                }
            }
    }
}
