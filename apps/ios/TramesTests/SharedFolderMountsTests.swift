// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest
@testable import Trames
@testable import TramesKit

final class SharedFolderMountsTests: XCTestCase {
    func testMountStoreDefaultsToggleDeduplicateAndResolve() async throws {
        let root = FileManager.default.temporaryDirectory.appendingPathComponent("trames-mounts-\(UUID().uuidString)", isDirectory: true)
        let sharedDir = root.appendingPathComponent("Documents", isDirectory: true)
        let folderA = root.appendingPathComponent("Projects", isDirectory: true)
        let folderB = root.appendingPathComponent("Shared", isDirectory: true)
        try FileManager.default.createDirectory(at: sharedDir, withIntermediateDirectories: true)
        try FileManager.default.createDirectory(at: folderA, withIntermediateDirectories: true)
        try FileManager.default.createDirectory(at: folderB, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: root) }

        let store = try MountStore(directory: root.appendingPathComponent("config", isDirectory: true))
        let initial = await store.configuration(for: "debian")
        XCTAssertTrue(initial.mountSharedDirectory)
        XCTAssertTrue(initial.mounts.isEmpty)

        // Adding a folder named "Shared" deduplicates against the reserved built-in mount.
        var config = try await store.add(selected: folderA, readonly: false, for: "debian")
        config = try await store.add(selected: folderB, readonly: false, for: "debian")
        XCTAssertEqual(config.mounts.map(\.name), ["Projects", "Shared-2"])
        XCTAssertEqual(config.mounts.map(\.readonly), [false, false])

        // Toggle readonly and rename.
        config = try await store.setReadonly(true, id: config.mounts[0].id, for: "debian")
        XCTAssertTrue(config.mounts[0].readonly)
        config = try await store.rename("Work", id: config.mounts[0].id, for: "debian")
        XCTAssertEqual(config.mounts[0].guestPath, "/mnt/iPad/Work")

        // Resolve with built-in Shared enabled.
        let resolved = await store.resolveForSession(environmentID: "debian", sharedDirectory: sharedDir)
        XCTAssertEqual(resolved.mounts.map(\.target), ["/mnt/iPad/Shared", "/mnt/iPad/Work", "/mnt/iPad/Shared-2"])
        XCTAssertEqual(resolved.mounts.map(\.readonly), [false, true, false])
        resolved.lease.release()

        // Disable built-in Shared directory.
        _ = try await store.setMountSharedDirectory(false, for: "debian")
        let withoutShared = await store.resolveForSession(environmentID: "debian", sharedDirectory: sharedDir)
        XCTAssertEqual(withoutShared.mounts.map(\.target), ["/mnt/iPad/Work", "/mnt/iPad/Shared-2"])
        withoutShared.lease.release()
    }

    func testSharedInboxBridgeDrainsWithoutOverwritingAndSkipsStaging() throws {
        let root = FileManager.default.temporaryDirectory.appendingPathComponent("trames-inbox-\(UUID().uuidString)", isDirectory: true)
        let source = root.appendingPathComponent("ShareInbox", isDirectory: true)
        let dest = root.appendingPathComponent("Inbox", isDirectory: true)
        try FileManager.default.createDirectory(at: source, withIntermediateDirectories: true)
        try FileManager.default.createDirectory(at: dest, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: root) }

        try Data("existing".utf8).write(to: dest.appendingPathComponent("note.txt"))
        try Data("incoming".utf8).write(to: source.appendingPathComponent("note.txt"))
        try Data("partial".utf8).write(to: source.appendingPathComponent(".staging-123"))

        let imported = SharedInboxBridge.drainSharedInbox(from: source, to: dest)
        XCTAssertEqual(imported, ["note (2).txt"])
        XCTAssertEqual(try String(contentsOf: dest.appendingPathComponent("note.txt")), "existing")
        XCTAssertEqual(try String(contentsOf: dest.appendingPathComponent("note (2).txt")), "incoming")
        XCTAssertTrue(FileManager.default.fileExists(atPath: source.appendingPathComponent(".staging-123").path))
    }
}
