// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import UniformTypeIdentifiers
import TramesKit
struct ExportFile: Identifiable { let id = UUID(); let url: URL }
struct TransferView: View {
    @ObservedObject var session: TerminalSession
    @Environment(\.dismiss) private var dismiss
    @State private var path = "/root/imported-file"
    @State private var selected: URL?
    @State private var chooser = false
    @State private var busy = false
    @State private var replace = true
    @State private var message = ""
    @State private var exportFile: ExportFile?
    @State private var usbDirectory: ExportFile?
    @State private var task: Task<Void,Never>?
    var body: some View {
        NavigationStack {
            Form {
                Section(L10n.text("本地 Linux 文件")) {
                    Text(UserDefaults.standard.bool(forKey:"rootfsFileProviderAvailable") ? L10n.text("可在文件 App 中浏览 Trames › %@。编辑现有普通文件后，回到 Trames 时写回；待写回副本可在「待写回的编辑」中另存。", String(describing: session.environmentName)) : L10n.text("当前签名未提供文件共享容器，可继续使用这里的导入和导出。"))
                        .font(.caption).foregroundStyle(.secondary)
                    TextField(L10n.text("文件路径"), text: $path).textInputAutocapitalization(.never).autocorrectionDisabled().disabled(busy)
                    if let selected { Text(selected.lastPathComponent) }
                    Button(L10n.text("选择 Files 文件")) { chooser = true }.disabled(busy)
                    Toggle(L10n.text("替换 Linux 中的同名文件"),isOn:$replace).disabled(busy)
                    Button(L10n.text("导入到 Linux")) { transfer(upload: true) }.disabled(busy || selected == nil)
                    Button(L10n.text("导出到 Files")) { transfer(upload: false) }.disabled(busy)
                }
                Section(L10n.text("USB 文件共享")) {
                    Text(L10n.text("电脑连接 iPad 后，在 Apple Devices 或 Finder 的「文件」中选择 Trames。添加文件后，在这里选择它，再点「导入到 Linux」。导出的文件也会出现在电脑的同一列表中。"))
                        .font(.caption).foregroundStyle(.secondary)
                    Button(L10n.text("选择 USB 共享文件")) {
                        do { usbDirectory = ExportFile(url: try USBFileSharing.directory()) }
                        catch { message = error.localizedDescription }
                    }.disabled(busy)
                    Button(L10n.text("导出到 USB 共享目录")) { transfer(upload: false, usb: true) }.disabled(busy)
                }
                Section { Text(message.isEmpty ? L10n.text("导入使用文件副本；开启替换后，同名文件在传输完整成功后替换，取消或失败时保留原文件。") : message).font(.caption) }
                if busy { Button(L10n.text("取消传输"), role: .destructive) { task?.cancel(); message = L10n.text("取消中…") } }
            }.navigationTitle("Files")
                .toolbar { Button(L10n.text("完成")) { task?.cancel(); dismiss() } }
                .fileImporter(isPresented: $chooser, allowedContentTypes: [.data, .item]) { result in
                    switch result {
                    case .success(let url): select(url)
                    case .failure(let error): message = error.localizedDescription
                    }
                }
                .sheet(item: $exportFile) { file in
                    FileExportPicker(url: file.url) { exportFile = nil; try? FileManager.default.removeItem(at: file.url.deletingLastPathComponent()) }
                }
                .sheet(item: $usbDirectory) { directory in
                    USBFileImportPicker(directory: directory.url) { url in
                        if let url { select(url) }
                        usbDirectory = nil
                    }
                }
                .onDisappear { task?.cancel() }
        }
    }
    private func select(_ url: URL) {
        selected = url
        if path == "/root/imported-file" { path = "/root/" + url.lastPathComponent }
    }
    private func transfer(upload: Bool, usb: Bool = false) {
        guard !path.isEmpty else { message = L10n.text("请输入文件路径"); return }
        busy = true; message = L10n.text("传输中…")
        let guestPath = path, source = selected, replacing = replace
        task = Task {
            let stageDirectory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString, isDirectory: true)
            var keep = false
            defer { busy = false; if !keep { try? FileManager.default.removeItem(at: stageDirectory) } }
            do {
                try FileManager.default.createDirectory(at: stageDirectory, withIntermediateDirectories: true)
                let stage = stageDirectory.appendingPathComponent(upload ? "payload" : (try USBFileSharing.filename(guestPath: guestPath)))
                if upload, let source { try await ScopedFiles.stage(selected: source, destination: stage) }
                try Task.checkCancellation()
                if let local = session.local {
                    try await local.transferAsync(guest: guestPath, host: stage, export: !upload, replace:upload && replacing)
                } else { throw TramesError.message(L10n.text("会话已关闭")) }
                try Task.checkCancellation()
                if upload { message = L10n.text("传输完成") }
                else if usb {
                    let destination = try USBFileSharing.publish(stagedFile: stage, to: USBFileSharing.directory())
                    message = L10n.text("已导出到 USB 共享目录：%@", destination.lastPathComponent)
                }
                else { keep = true; exportFile = ExportFile(url: stage); message = L10n.text("请选择导出位置") }
            } catch { message = error.localizedDescription }
        }
    }
}
struct USBFileImportPicker: UIViewControllerRepresentable {
    let directory: URL
    let finished: (URL?) -> Void
    func makeCoordinator() -> Coordinator { Coordinator(finished: finished) }
    func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: [.item], asCopy: false)
        picker.directoryURL = directory
        picker.shouldShowFileExtensions = true
        picker.delegate = context.coordinator
        return picker
    }
    func updateUIViewController(_ controller: UIDocumentPickerViewController, context: Context) {}
    final class Coordinator: NSObject, UIDocumentPickerDelegate {
        let finished: (URL?) -> Void
        init(finished: @escaping (URL?) -> Void) { self.finished = finished }
        func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) { finished(nil) }
        func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) { finished(urls.first) }
    }
}
struct FileExportPicker: UIViewControllerRepresentable {
    let url: URL
    let finished: () -> Void
    func makeCoordinator() -> Coordinator { Coordinator(finished: finished) }
    func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
        let picker = UIDocumentPickerViewController(forExporting: [url], asCopy: true)
        picker.delegate = context.coordinator; return picker
    }
    func updateUIViewController(_ view: UIDocumentPickerViewController, context: Context) {}
    static func dismantleUIViewController(_ controller: UIDocumentPickerViewController, coordinator: Coordinator) { coordinator.cleanup() }
    final class Coordinator: NSObject, UIDocumentPickerDelegate {
        let finished: () -> Void
        private var done = false
        func cleanup() { guard !done else { return }; done = true; finished() }
        init(finished: @escaping () -> Void) { self.finished = finished }
        func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) { cleanup() }
        func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) { cleanup() }
    }
}
