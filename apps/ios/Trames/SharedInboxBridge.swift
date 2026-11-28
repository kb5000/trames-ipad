// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import TramesKit

/// Manages the built-in shared folder (`Documents/` -> `/mnt/iPad/Shared`) and
/// drains files dropped by `TramesShareExtension` from the App Group container
/// into `Documents/Inbox/` (`/mnt/iPad/Shared/Inbox/`) without polling.
final class SharedInboxBridge: @unchecked Sendable {
    static let appGroupInboxName = "ShareInbox"

    private let lock = NSLock()
    private var directoryFD: Int32 = -1
    private var source: DispatchSourceFileSystemObject?
    private let onImported: @Sendable ([String]) -> Void

    init(onImported: @escaping @Sendable ([String]) -> Void = { _ in }) {
        self.onImported = onImported
    }

    deinit {
        stopWatching()
    }

    static func sharedDirectory() throws -> URL {
        let url = try FileManager.default.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: true)
        try FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
        let imports = url.appendingPathComponent("Imports", isDirectory: true)
        try FileManager.default.createDirectory(at: imports, withIntermediateDirectories: true)
        return url
    }

    static func inboxDirectory() throws -> URL {
        let imports = try sharedDirectory().appendingPathComponent("Imports", isDirectory: true)
        try FileManager.default.createDirectory(at: imports, withIntermediateDirectories: true)
        return imports
    }

    static func appGroupInboxDirectory() -> URL? {
        guard let root = RootfsLocations.sharedRoot else { return nil }
        let inbox = root.appendingPathComponent(appGroupInboxName, isDirectory: true)
        try? FileManager.default.createDirectory(at: inbox, withIntermediateDirectories: true)
        return inbox
    }

    private static func legacyAppGroupInboxDirectory() -> URL? {
        guard let group = RootfsLocations.group, !group.isEmpty,
              let container = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: group)
        else { return nil }
        let legacy = container.appendingPathComponent(appGroupInboxName, isDirectory: true)
        return FileManager.default.fileExists(atPath: legacy.path) ? legacy : nil
    }

    func startWatching() {
        lock.lock()
        defer { lock.unlock() }
        guard source == nil, let stagedDir = Self.appGroupInboxDirectory() else { return }
        let fd = open(stagedDir.path, O_EVTONLY)
        guard fd >= 0 else { return }
        let watcher = DispatchSource.makeFileSystemObjectSource(
            fileDescriptor: fd,
            eventMask: [.write, .rename, .extend],
            queue: DispatchQueue.global(qos: .utility)
        )
        watcher.setEventHandler { [weak self] in
            self?.drainAndNotify()
        }
        watcher.setCancelHandler {
            close(fd)
        }
        directoryFD = fd
        source = watcher
        watcher.resume()
    }

    func stopWatching() {
        lock.lock()
        let active = source
        source = nil
        directoryFD = -1
        lock.unlock()
        active?.cancel()
    }

    @discardableResult
    func drainAndNotify() -> [String] {
        let destination = try? Self.inboxDirectory()
        var imported = Self.drainSharedInbox(to: destination)
        if let legacy = Self.legacyAppGroupInboxDirectory() {
            imported.append(contentsOf: Self.drainSharedInbox(from: legacy, to: destination))
        }
        // Migrate any files previously placed in the iOS-reserved Documents/Inbox folder.
        if let docs = try? FileManager.default.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: false) {
            let oldInbox = docs.appendingPathComponent("Inbox", isDirectory: true)
            if FileManager.default.fileExists(atPath: oldInbox.path) {
                imported.append(contentsOf: Self.drainSharedInbox(from: oldInbox, to: destination))
            }
        }
        if !imported.isEmpty {
            onImported(imported)
        }
        return imported
    }

    @discardableResult
    static func drainSharedInbox(from sourceDir: URL? = appGroupInboxDirectory(), to destinationDir: URL? = try? inboxDirectory()) -> [String] {
        guard let sourceDir, let destinationDir else { return [] }
        let fm = FileManager.default
        try? fm.createDirectory(at: destinationDir, withIntermediateDirectories: true)
        guard let entries = try? fm.contentsOfDirectory(at: sourceDir, includingPropertiesForKeys: nil, options: [.skipsHiddenFiles]) else {
            return []
        }
        var imported: [String] = []
        for item in entries.sorted(by: { $0.lastPathComponent < $1.lastPathComponent }) {
            // Skip in-progress staging files written by the Share Extension.
            if item.lastPathComponent.hasPrefix(".staging-") { continue }
            let target = uniqueDestination(in: destinationDir, preferredName: item.lastPathComponent)
            do {
                try fm.moveItem(at: item, to: target)
                imported.append(target.lastPathComponent)
            } catch {
                // Retry copy+remove across volumes if move fails.
                do {
                    try fm.copyItem(at: item, to: target)
                    try? fm.removeItem(at: item)
                    imported.append(target.lastPathComponent)
                } catch {
                    continue
                }
            }
        }
        return imported
    }

    static func uniqueDestination(in directory: URL, preferredName: String) -> URL {
        let fm = FileManager.default
        let sanitized = preferredName.isEmpty ? "shared-item" : preferredName
        let candidate = directory.appendingPathComponent(sanitized)
        if !fm.fileExists(atPath: candidate.path) {
            return candidate
        }
        let nsName = sanitized as NSString
        let ext = nsName.pathExtension
        let stem = ext.isEmpty ? sanitized : nsName.deletingPathExtension
        var index = 2
        while true {
            let nextName = ext.isEmpty ? "\(stem) (\(index))" : "\(stem) (\(index)).\(ext)"
            let nextURL = directory.appendingPathComponent(nextName)
            if !fm.fileExists(atPath: nextURL.path) {
                return nextURL
            }
            index += 1
        }
    }
}
