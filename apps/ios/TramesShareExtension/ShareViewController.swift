// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import UIKit
import UniformTypeIdentifiers

@objc(ShareViewController)
final class ShareViewController: UIViewController {
    private let iconView = UIImageView()
    private let titleLabel = UILabel()
    private let detailLabel = UILabel()
    private let spinner = UIActivityIndicatorView(style: .medium)
    private let doneButton = UIButton(type: .system)

    override func viewDidLoad() {
        super.viewDidLoad()
        configureUI()
        Task { await handleSharedItems() }
    }

    private func configureUI() {
        view.backgroundColor = UIColor.black.withAlphaComponent(0.28)

        let card = UIVisualEffectView(effect: UIBlurEffect(style: .systemMaterial))
        card.translatesAutoresizingMaskIntoConstraints = false
        card.layer.cornerRadius = 18
        card.clipsToBounds = true
        view.addSubview(card)

        iconView.translatesAutoresizingMaskIntoConstraints = false
        iconView.contentMode = .scaleAspectFit
        iconView.tintColor = .systemGreen
        iconView.isHidden = true

        spinner.translatesAutoresizingMaskIntoConstraints = false
        spinner.startAnimating()

        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        titleLabel.font = .preferredFont(forTextStyle: .headline)
        titleLabel.textAlignment = .center
        titleLabel.numberOfLines = 0
        titleLabel.text = localized("正在保存到 Trames…", "Saving to Trames…")

        detailLabel.translatesAutoresizingMaskIntoConstraints = false
        detailLabel.font = .monospacedSystemFont(ofSize: 13, weight: .regular)
        detailLabel.textColor = .secondaryLabel
        detailLabel.textAlignment = .center
        detailLabel.numberOfLines = 0
        detailLabel.text = "/mnt/iPad/Shared/Imports"

        doneButton.translatesAutoresizingMaskIntoConstraints = false
        doneButton.setTitle(localized("完成", "Done"), for: .normal)
        doneButton.titleLabel?.font = .preferredFont(forTextStyle: .body)
        doneButton.isHidden = true
        doneButton.addTarget(self, action: #selector(finishExtension), for: .touchUpInside)

        let stack = UIStackView(arrangedSubviews: [spinner, iconView, titleLabel, detailLabel, doneButton])
        stack.translatesAutoresizingMaskIntoConstraints = false
        stack.axis = .vertical
        stack.alignment = .center
        stack.spacing = 10
        card.contentView.addSubview(stack)

        NSLayoutConstraint.activate([
            card.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            card.centerYAnchor.constraint(equalTo: view.centerYAnchor),
            card.widthAnchor.constraint(lessThanOrEqualToConstant: 320),
            card.widthAnchor.constraint(greaterThanOrEqualToConstant: 240),
            iconView.widthAnchor.constraint(equalToConstant: 36),
            iconView.heightAnchor.constraint(equalToConstant: 36),
            stack.topAnchor.constraint(equalTo: card.contentView.topAnchor, constant: 22),
            stack.bottomAnchor.constraint(equalTo: card.contentView.bottomAnchor, constant: -20),
            stack.leadingAnchor.constraint(equalTo: card.contentView.leadingAnchor, constant: 20),
            stack.trailingAnchor.constraint(equalTo: card.contentView.trailingAnchor, constant: -20),
        ])
    }

    @objc private func finishExtension() {
        extensionContext?.completeRequest(returningItems: nil)
    }

    private func handleSharedItems() async {
        guard let inboxDir = Self.appGroupInboxDirectory() else {
            showResult(success: false, title: localized("无法访问 Trames 共享容器", "Cannot access Trames shared container"), detail: "")
            return
        }
        let items = (extensionContext?.inputItems as? [NSExtensionItem]) ?? []
        let providers = items.flatMap { $0.attachments ?? [] }
        guard !providers.isEmpty else {
            showResult(success: false, title: localized("没有可保存的内容", "Nothing to save"), detail: "")
            return
        }

        var savedNames: [String] = []
        for provider in providers {
            if let name = await Self.saveAttachment(provider, to: inboxDir) {
                savedNames.append(name)
            }
        }

        if savedNames.isEmpty {
            showResult(success: false, title: localized("未能保存所选项目", "Could not save selected items"), detail: "")
        } else {
            let detail: String
            if savedNames.count == 1 {
                detail = "/mnt/iPad/Shared/Imports/\(savedNames[0])"
            } else {
                detail = localized("已保存 \(savedNames.count) 个项目到 /mnt/iPad/Shared/Imports", "Saved \(savedNames.count) items to /mnt/iPad/Shared/Imports")
            }
            showResult(success: true, title: localized("已保存到 Trames", "Saved to Trames"), detail: detail)
            try? await Task.sleep(for: .milliseconds(1100))
            await MainActor.run {
                self.finishExtension()
            }
        }
    }

    @MainActor
    private func showResult(success: Bool, title: String, detail: String) {
        spinner.stopAnimating()
        spinner.isHidden = true
        iconView.image = UIImage(systemName: success ? "checkmark.circle.fill" : "exclamationmark.triangle.fill")
        iconView.tintColor = success ? .systemGreen : .systemOrange
        iconView.isHidden = false
        titleLabel.text = title
        detailLabel.text = detail
        doneButton.isHidden = false
    }

    private func localized(_ zh: String, _ en: String) -> String {
        Locale.preferredLanguages.first?.hasPrefix("zh") == true ? zh : en
    }

    private static func appGroupInboxDirectory() -> URL? {
        guard let group = Bundle.main.object(forInfoDictionaryKey: "TramesAppGroup") as? String,
              !group.isEmpty,
              let container = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: group)
        else { return nil }
        let inbox = container
            .appendingPathComponent("Trames", isDirectory: true)
            .appendingPathComponent("ShareInbox", isDirectory: true)
        try? FileManager.default.createDirectory(at: inbox, withIntermediateDirectories: true)
        return inbox
    }

    private static func saveAttachment(_ provider: NSItemProvider, to inboxDir: URL) async -> String? {
        // 1. Prefer concrete file/folder representations (Files, Mail, Downloads, etc.).
        if provider.hasItemConformingToTypeIdentifier(UTType.fileURL.identifier) {
            if let name = await copyFileRepresentation(provider, typeIdentifier: UTType.fileURL.identifier, to: inboxDir) {
                return name
            }
        }
        if provider.hasItemConformingToTypeIdentifier(UTType.item.identifier),
           let typeID = provider.registeredTypeIdentifiers.first(where: { id in
               guard let type = UTType(id) else { return false }
               return type.conforms(to: .data) || type.conforms(to: .directory) || type.conforms(to: .package)
           }) {
            if let name = await copyFileRepresentation(provider, typeIdentifier: typeID, to: inboxDir) {
                return name
            }
        }
        // 2. Fallback for images without a direct file URL.
        if provider.hasItemConformingToTypeIdentifier(UTType.image.identifier) {
            if let name = await copyFileRepresentation(provider, typeIdentifier: UTType.image.identifier, to: inboxDir) {
                return name
            }
            if let image = await loadObject(provider, typeIdentifier: UTType.image.identifier) as? UIImage,
               let png = image.pngData() {
                return writeData(png, preferredName: suggestedFilename(provider, fallback: "shared-image.png"), to: inboxDir)
            }
        }
        // 3. Web URLs.
        if provider.hasItemConformingToTypeIdentifier(UTType.url.identifier) {
            if let url = await loadObject(provider, typeIdentifier: UTType.url.identifier) as? URL {
                let text = url.absoluteString + "\n"
                return writeData(Data(text.utf8), preferredName: suggestedFilename(provider, fallback: "shared-link.txt"), to: inboxDir)
            }
        }
        // 4. Plain text.
        if provider.hasItemConformingToTypeIdentifier(UTType.plainText.identifier) {
            if let text = await loadObject(provider, typeIdentifier: UTType.plainText.identifier) as? String {
                return writeData(Data(text.utf8), preferredName: suggestedFilename(provider, fallback: "shared-text.txt"), to: inboxDir)
            }
        }
        return nil
    }

    private static func copyFileRepresentation(_ provider: NSItemProvider, typeIdentifier: String, to inboxDir: URL) async -> String? {
        await withCheckedContinuation { continuation in
            provider.loadFileRepresentation(forTypeIdentifier: typeIdentifier) { url, _ in
                guard let url else {
                    continuation.resume(returning: nil)
                    return
                }
                let fm = FileManager.default
                let preferred = sanitizeFilename(provider.suggestedName ?? url.lastPathComponent, fallbackExtension: url.pathExtension)
                let staging = inboxDir.appendingPathComponent(".staging-\(UUID().uuidString)")
                do {
                    try fm.copyItem(at: url, to: staging)
                    let finalURL = uniqueDestination(in: inboxDir, preferredName: preferred)
                    try fm.moveItem(at: staging, to: finalURL)
                    continuation.resume(returning: finalURL.lastPathComponent)
                } catch {
                    try? fm.removeItem(at: staging)
                    continuation.resume(returning: nil)
                }
            }
        }
    }

    private static func loadObject(_ provider: NSItemProvider, typeIdentifier: String) async -> NSSecureCoding? {
        await withCheckedContinuation { continuation in
            provider.loadItem(forTypeIdentifier: typeIdentifier, options: nil) { item, _ in
                continuation.resume(returning: item)
            }
        }
    }

    private static func writeData(_ data: Data, preferredName: String, to inboxDir: URL) -> String? {
        let fm = FileManager.default
        let staging = inboxDir.appendingPathComponent(".staging-\(UUID().uuidString)")
        do {
            try data.write(to: staging, options: .atomic)
            let finalURL = uniqueDestination(in: inboxDir, preferredName: preferredName)
            try fm.moveItem(at: staging, to: finalURL)
            return finalURL.lastPathComponent
        } catch {
            try? fm.removeItem(at: staging)
            return nil
        }
    }

    private static func suggestedFilename(_ provider: NSItemProvider, fallback: String) -> String {
        guard let raw = provider.suggestedName, !raw.isEmpty else { return fallback }
        let cleaned = sanitizeFilename(raw, fallbackExtension: (fallback as NSString).pathExtension)
        return cleaned.isEmpty ? fallback : cleaned
    }

    private static func sanitizeFilename(_ raw: String, fallbackExtension: String) -> String {
        let trimmed = raw.trimmingCharacters(in: .whitespacesAndNewlines)
            .replacingOccurrences(of: "/", with: "-")
            .replacingOccurrences(of: "\0", with: "")
        var base = (trimmed.isEmpty || trimmed == "." || trimmed == "..") ? "shared-item" : trimmed
        if (base as NSString).pathExtension.isEmpty, !fallbackExtension.isEmpty {
            base += ".\(fallbackExtension)"
        }
        return String(base.prefix(128))
    }

    private static func uniqueDestination(in directory: URL, preferredName: String) -> URL {
        let fm = FileManager.default
        let candidate = directory.appendingPathComponent(preferredName)
        if !fm.fileExists(atPath: candidate.path) { return candidate }
        let ns = preferredName as NSString
        let ext = ns.pathExtension
        let stem = ext.isEmpty ? preferredName : ns.deletingPathExtension
        var index = 2
        while true {
            let name = ext.isEmpty ? "\(stem) (\(index))" : "\(stem) (\(index)).\(ext)"
            let next = directory.appendingPathComponent(name)
            if !fm.fileExists(atPath: next.path) { return next }
            index += 1
        }
    }
}
