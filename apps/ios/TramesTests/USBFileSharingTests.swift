// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import XCTest
@testable import Trames

final class USBFileSharingTests: XCTestCase {
    private func workspace(_ body: (URL, URL) throws -> Void) throws {
        let root = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: root) }
        let staging = root.appendingPathComponent("staging"), documents = root.appendingPathComponent("Documents")
        try FileManager.default.createDirectory(at: staging, withIntermediateDirectories: true)
        try FileManager.default.createDirectory(at: documents, withIntermediateDirectories: true)
        try body(staging, documents)
    }

    func testExportRetainsFilenameAndCompleteBytes() throws {
        try workspace { staging, documents in
            let name = try USBFileSharing.filename(guestPath: "/root/中文 report.tar.gz")
            let file = staging.appendingPathComponent(name)
            let bytes = Data((0..<262144).map { UInt8(truncatingIfNeeded: $0) })
            try bytes.write(to: file)
            let exported = try USBFileSharing.publish(stagedFile: file, to: documents)
            XCTAssertEqual(exported.lastPathComponent, "中文 report.tar.gz")
            XCTAssertEqual(try Data(contentsOf: exported), bytes)
            XCTAssertFalse(FileManager.default.fileExists(atPath: file.path))
        }
        for path in ["", "/", "/root/", "/root/.", "/root/..", "/root/a\0b"] {
            XCTAssertThrowsError(try USBFileSharing.filename(guestPath: path), path)
        }
    }

    func testExistingExportIsNotOverwritten() throws {
        try workspace { staging, documents in
            let file = staging.appendingPathComponent("report.txt"), existing = documents.appendingPathComponent("report.txt")
            try Data("new".utf8).write(to: file)
            try Data("original".utf8).write(to: existing)
            XCTAssertThrowsError(try USBFileSharing.publish(stagedFile: file, to: documents))
            XCTAssertEqual(try String(contentsOf: existing, encoding: .utf8), "original")
            XCTAssertEqual(try String(contentsOf: file, encoding: .utf8), "new")
        }
    }

    func testDanglingSymlinkIsNotReplacedOrFollowed() throws {
        try workspace { staging, documents in
            let file = staging.appendingPathComponent("report.txt")
            let existing = documents.appendingPathComponent("report.txt")
            let outside = staging.appendingPathComponent("missing.txt")
            try Data("new".utf8).write(to: file)
            try FileManager.default.createSymbolicLink(at: existing, withDestinationURL: outside)
            XCTAssertThrowsError(try USBFileSharing.publish(stagedFile: file, to: documents))
            XCTAssertEqual(try FileManager.default.destinationOfSymbolicLink(atPath: existing.path), outside.path)
            XCTAssertFalse(FileManager.default.fileExists(atPath: outside.path))
        }
    }
}
