// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation

#if canImport(Darwin)
public struct EnvironmentMount: Codable, Identifiable, Sendable, Equatable {
    public let id: UUID
    public var name: String
    public var readonly: Bool
    public var isCloudBacked: Bool
    fileprivate var bookmark: Data

    public var guestPath: String {
        "\(MountStore.mountRoot)/\(name)"
    }
}

public struct EnvironmentMountConfiguration: Codable, Sendable, Equatable {
    public var mountSharedDirectory: Bool
    public var mounts: [EnvironmentMount]

    public init(mountSharedDirectory: Bool = true, mounts: [EnvironmentMount] = []) {
        self.mountSharedDirectory = mountSharedDirectory
        self.mounts = mounts
    }
}

/// Holds active security-scoped resource access for a running LocalSession.
public final class SessionMountLease: @unchecked Sendable {
    private let lock = NSLock()
    private var scopedByTarget: [String: URL]

    public init(scopedByTarget: [String: URL] = [:]) {
        self.scopedByTarget = scopedByTarget
    }

    public func retain(_ url: URL, for target: String) {
        lock.lock()
        let previous = scopedByTarget.updateValue(url, forKey: target)
        lock.unlock()
        if let previous, previous != url {
            previous.stopAccessingSecurityScopedResource()
        }
    }

    public func release(target: String) {
        lock.lock()
        let removed = scopedByTarget.removeValue(forKey: target)
        lock.unlock()
        removed?.stopAccessingSecurityScopedResource()
    }

    public func release() {
        lock.lock()
        let active = Array(scopedByTarget.values)
        scopedByTarget.removeAll()
        lock.unlock()
        for url in active {
            url.stopAccessingSecurityScopedResource()
        }
    }

    deinit {
        release()
    }
}

public struct ResolvedSessionMounts: Sendable {
    public let mounts: [LocalHostMount]
    public let lease: SessionMountLease
    public let failedMountIDs: [UUID]
}

/// Persists per-environment mounted folder bookmarks and resolves them for session startup.
public actor MountStore {
    public static let mountRoot = "/mnt/iPad"
    public static let sharedMountName = "Shared"
    public static let sharedGuestPath = "\(mountRoot)/\(sharedMountName)"
    public static let inboxGuestPath = "\(sharedGuestPath)/Imports"
    public static let maximumMounts = 32

    private let directory: URL

    public init(directory: URL) throws {
        self.directory = directory
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    }

    private func fileURL(for environmentID: String) -> URL {
        let safe = environmentID.unicodeScalars.map { scalar -> Character in
            let v = scalar.value
            if (v >= 48 && v <= 57) || (v >= 65 && v <= 90) || (v >= 97 && v <= 122) || scalar == "-" || scalar == "_" {
                return Character(scalar)
            }
            return "_"
        }
        return directory.appendingPathComponent("mounts-\(String(safe)).json")
    }

    public func configuration(for environmentID: String) -> EnvironmentMountConfiguration {
        let url = fileURL(for: environmentID)
        guard FileManager.default.fileExists(atPath: url.path),
              let data = try? Data(contentsOf: url),
              data.count <= 512 * 1024,
              let decoded = try? JSONDecoder().decode(EnvironmentMountConfiguration.self, from: data)
        else {
            return EnvironmentMountConfiguration()
        }
        return decoded
    }

    private func persist(_ config: EnvironmentMountConfiguration, for environmentID: String) throws {
        let url = fileURL(for: environmentID)
        let data = try JSONEncoder().encode(config)
        guard data.count <= 512 * 1024 else {
            throw TramesError.message("Mount configuration exceeds its storage budget")
        }
        try data.write(to: url, options: .atomic)
        try? FileManager.default.setAttributes([.posixPermissions: 0o600], ofItemAtPath: url.path)
    }

    public func setMountSharedDirectory(_ enabled: Bool, for environmentID: String) throws -> EnvironmentMountConfiguration {
        var config = configuration(for: environmentID)
        config.mountSharedDirectory = enabled
        try persist(config, for: environmentID)
        return config
    }

    public func add(selected: URL, readonly: Bool = false, for environmentID: String) throws -> EnvironmentMountConfiguration {
        var config = configuration(for: environmentID)
        guard config.mounts.count < Self.maximumMounts else {
            throw TramesError.message("At most \(Self.maximumMounts) folders can be mounted per environment")
        }
        let scoped = selected.startAccessingSecurityScopedResource()
        defer { if scoped { selected.stopAccessingSecurityScopedResource() } }
        let values = try selected.resourceValues(forKeys: [.isDirectoryKey, .isUbiquitousItemKey])
        guard values.isDirectory == true else {
            throw TramesError.message("Choose a folder")
        }
        #if os(macOS)
        let bookmark = try selected.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil)
        #else
        let bookmark = try selected.bookmarkData(options: .minimalBookmark, includingResourceValuesForKeys: nil, relativeTo: nil)
        #endif
        let isCloud = (values.isUbiquitousItem == true) || selected.path.contains("/Mobile Documents/") || selected.path.contains("/File Provider Storage/")
        let name = Self.uniqueMountName(base: selected.lastPathComponent, existing: config.mounts.map(\.name))
        let mount = EnvironmentMount(id: UUID(), name: name, readonly: readonly, isCloudBacked: isCloud, bookmark: bookmark)
        config.mounts.append(mount)
        try persist(config, for: environmentID)
        return config
    }

    public func setReadonly(_ readonly: Bool, id: UUID, for environmentID: String) throws -> EnvironmentMountConfiguration {
        var config = configuration(for: environmentID)
        guard let index = config.mounts.firstIndex(where: { $0.id == id }) else { return config }
        config.mounts[index].readonly = readonly
        try persist(config, for: environmentID)
        return config
    }

    public func rename(_ requestedName: String, id: UUID, for environmentID: String) throws -> EnvironmentMountConfiguration {
        var config = configuration(for: environmentID)
        guard let index = config.mounts.firstIndex(where: { $0.id == id }) else { return config }
        let sanitized = Self.sanitizeMountName(requestedName)
        guard !sanitized.isEmpty else {
            throw TramesError.message("Mount name cannot be empty")
        }
        guard sanitized.caseInsensitiveCompare(Self.sharedMountName) != .orderedSame else {
            throw TramesError.message("The name \"\(Self.sharedMountName)\" is reserved")
        }
        let duplicate = config.mounts.enumerated().contains { offset, item in
            offset != index && item.name.caseInsensitiveCompare(sanitized) == .orderedSame
        }
        guard !duplicate else {
            throw TramesError.message("A folder named \"\(sanitized)\" is already mounted")
        }
        config.mounts[index].name = sanitized
        try persist(config, for: environmentID)
        return config
    }

    public func remove(id: UUID, for environmentID: String) throws -> EnvironmentMountConfiguration {
        var config = configuration(for: environmentID)
        config.mounts.removeAll { $0.id == id }
        try persist(config, for: environmentID)
        return config
    }

    public func deleteConfiguration(for environmentID: String) {
        let url = fileURL(for: environmentID)
        try? FileManager.default.removeItem(at: url)
    }

    public func resolveSingleMount(id: UUID, for environmentID: String) -> (mount: LocalHostMount, scopedURL: URL?)? {
        var config = configuration(for: environmentID)
        guard let index = config.mounts.firstIndex(where: { $0.id == id }) else { return nil }
        let item = config.mounts[index]
        var stale = false
        do {
            #if os(macOS)
            let folder = try URL(resolvingBookmarkData: item.bookmark, options: [.withSecurityScope, .withoutUI], relativeTo: nil, bookmarkDataIsStale: &stale)
            #else
            let folder = try URL(resolvingBookmarkData: item.bookmark, options: [.withoutUI], relativeTo: nil, bookmarkDataIsStale: &stale)
            #endif
            let scoped = folder.startAccessingSecurityScopedResource()
            var isDir: ObjCBool = false
            guard FileManager.default.fileExists(atPath: folder.path, isDirectory: &isDir), isDir.boolValue else {
                if scoped { folder.stopAccessingSecurityScopedResource() }
                return nil
            }
            if stale {
                #if os(macOS)
                if let refreshed = try? folder.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil) {
                    config.mounts[index].bookmark = refreshed
                    try? persist(config, for: environmentID)
                }
                #else
                if let refreshed = try? folder.bookmarkData(options: .minimalBookmark, includingResourceValuesForKeys: nil, relativeTo: nil) {
                    config.mounts[index].bookmark = refreshed
                    try? persist(config, for: environmentID)
                }
                #endif
            }
            return (LocalHostMount(target: item.guestPath, host: folder, readonly: item.readonly), scoped ? folder : nil)
        } catch {
            return nil
        }
    }

    public func resolveForSession(environmentID: String, sharedDirectory: URL?) -> ResolvedSessionMounts {
        var config = configuration(for: environmentID)
        var resolved: [LocalHostMount] = []
        var scopedByTarget: [String: URL] = [:]
        var failed: [UUID] = []
        var updatedBookmarks = false

        if config.mountSharedDirectory, let sharedDirectory {
            try? FileManager.default.createDirectory(at: sharedDirectory, withIntermediateDirectories: true)
            resolved.append(LocalHostMount(target: Self.sharedGuestPath, host: sharedDirectory, readonly: false))
        }

        for index in config.mounts.indices {
            let item = config.mounts[index]
            var stale = false
            do {
                #if os(macOS)
                let folder = try URL(resolvingBookmarkData: item.bookmark, options: [.withSecurityScope, .withoutUI], relativeTo: nil, bookmarkDataIsStale: &stale)
                #else
                let folder = try URL(resolvingBookmarkData: item.bookmark, options: [.withoutUI], relativeTo: nil, bookmarkDataIsStale: &stale)
                #endif
                let scoped = folder.startAccessingSecurityScopedResource()
                var isDir: ObjCBool = false
                guard FileManager.default.fileExists(atPath: folder.path, isDirectory: &isDir), isDir.boolValue else {
                    if scoped { folder.stopAccessingSecurityScopedResource() }
                    failed.append(item.id)
                    continue
                }
                if stale {
                    #if os(macOS)
                    if let refreshed = try? folder.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil) {
                        config.mounts[index].bookmark = refreshed
                        updatedBookmarks = true
                    }
                    #else
                    if let refreshed = try? folder.bookmarkData(options: .minimalBookmark, includingResourceValuesForKeys: nil, relativeTo: nil) {
                        config.mounts[index].bookmark = refreshed
                        updatedBookmarks = true
                    }
                    #endif
                }
                if scoped {
                    scopedByTarget[item.guestPath] = folder
                }
                resolved.append(LocalHostMount(target: item.guestPath, host: folder, readonly: item.readonly))
            } catch {
                failed.append(item.id)
            }
        }

        if updatedBookmarks {
            try? persist(config, for: environmentID)
        }
        return ResolvedSessionMounts(mounts: resolved, lease: SessionMountLease(scopedByTarget: scopedByTarget), failedMountIDs: failed)
    }

    public static func sanitizeMountName(_ raw: String) -> String {
        let trimmed = raw.trimmingCharacters(in: .whitespacesAndNewlines)
        let cleaned = trimmed
            .replacingOccurrences(of: "/", with: "-")
            .replacingOccurrences(of: "\0", with: "")
        if cleaned.isEmpty || cleaned == "." || cleaned == ".." {
            return "Folder"
        }
        return String(cleaned.prefix(64))
    }

    public static func uniqueMountName(base: String, existing: [String]) -> String {
        let sanitized = sanitizeMountName(base)
        let taken = Set((existing + [sharedMountName]).map { $0.lowercased() })
        if !taken.contains(sanitized.lowercased()) {
            return sanitized
        }
        var suffix = 2
        while taken.contains("\(sanitized)-\(suffix)".lowercased()) {
            suffix += 1
        }
        return "\(sanitized)-\(suffix)"
    }
}
#endif
