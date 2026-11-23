// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import Darwin
import XCTest
import TramesKit
@testable import Trames

@MainActor
final class EnvironmentDeletionTests:XCTestCase {
    func testDeletionRemovesOnlyTheCataloguedEnvironmentAndItsEdits() async throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("environment-delete-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let store=try EnvironmentStore(storageRoot:root)
        let archive=root.appendingPathComponent("import.tar");try Data("archive".utf8).write(to:archive)
        let a=try await store.create(name:"Delete me",distribution:"custom",initMode:"shell",packages:"minimal",archive:archive,sha256:nil)
        let b=try await store.create(name:"Keep me",distribution:"custom",initMode:"shell",packages:"minimal",archive:archive,sha256:nil)
        for profile in [a,b] {
            let directory=try profile.storeURL(in:root)
            try FileManager.default.createDirectory(at:directory,withIntermediateDirectories:true)
            try Data("old manifest".utf8).write(to:directory.appendingPathComponent("manifest"))
        }
        let journal=RootfsEditStore(root:root),edited=root.appendingPathComponent("edit.txt")
        try Data("edited".utf8).write(to:edited)
        for profile in [a,b] {
            let id="node/\(profile.id)/p/123"
            try journal.record(RootfsEditBase(identifier:id,environment:profile.id,path:"/root/test.txt",version:RootfsFileVersion(inode:1,sha256:"original")))
            try journal.stage(id,from:edited)
        }
        let otherConfig=try Data(contentsOf:root.appendingPathComponent("profiles/\(b.id).toml"))
        // Only the identifier is accepted from callers; the catalog owns paths.
        let forged=RootfsEnvironment(id:a.id,name:a.name,store:b.store)
        try await store.delete(forged)
        let remaining=try await store.profiles()
        XCTAssertFalse(remaining.contains{$0.id==a.id});XCTAssertTrue(remaining.contains{$0.id==b.id})
        for path in [a.store,"profiles/\(a.id).toml","grants-\(a.id).toml","rootfs-\(a.id).archive"] {
            XCTAssertFalse(FileManager.default.fileExists(atPath:root.appendingPathComponent(path).path),path)
        }
        XCTAssertEqual(try Data(contentsOf:root.appendingPathComponent("profiles/\(b.id).toml")),otherConfig)
        XCTAssertEqual(try String(contentsOf:root.appendingPathComponent(b.store+"/manifest"),encoding:.utf8),"old manifest")
        XCTAssertTrue(FileManager.default.fileExists(atPath:root.appendingPathComponent("rootfs-\(b.id).archive").path))
        XCTAssertEqual(journal.requests().map(\.base.environment),[b.id])
        XCTAssertNil(journal.base("node/\(a.id)/p/123"))
        XCTAssertTrue(FileManager.default.fileExists(atPath:archive.path))
    }

    func testDeletingTheLastEnvironmentStaysEmptyAfterReopening() async throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("environment-empty-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let store=try EnvironmentStore(storageRoot:root),profile=RootfsCatalog.alpine.environments[0]
        try await store.delete(profile)
        let reopened=try EnvironmentStore(storageRoot:root),profiles=try await reopened.profiles()
        XCTAssertTrue(profiles.isEmpty)
        do {_=try await reopened.start(profile:profile,useVolumeBudget:false);XCTFail("Deleted environment restarted")}
        catch RootfsCatalogError.noSuchItem {} catch {XCTFail("Unexpected error: \(error)")}
        do {try await reopened.reset(profile:profile);XCTFail("Deleted environment reset")}
        catch RootfsCatalogError.noSuchItem {} catch {XCTFail("Unexpected error: \(error)")}
        XCTAssertFalse(FileManager.default.fileExists(atPath:root.appendingPathComponent("environment").path))
    }

    func testDeletionWaitsForTheExistingStoreOwnerLock() async throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("environment-lock-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let store=try EnvironmentStore(storageRoot:root),profile=RootfsCatalog.alpine.environments[0]
        let directory=try profile.storeURL(in:root)
        try FileManager.default.createDirectory(at:directory,withIntermediateDirectories:true)
        let sentinel=directory.appendingPathComponent("manifest");try Data("keep until closed".utf8).write(to:sentinel)
        let fd=open(root.appendingPathComponent("environment-owner.lock").path,O_RDWR|O_CREAT|O_NOFOLLOW,0o600)
        XCTAssertGreaterThanOrEqual(fd,0);guard fd>=0 else {return}
        defer {flock(fd,LOCK_UN);close(fd)}
        XCTAssertEqual(flock(fd,LOCK_EX|LOCK_NB),0)
        let deletion=Task {try await store.delete(profile)}
        try await Task.sleep(for:.milliseconds(150))
        XCTAssertTrue(FileManager.default.fileExists(atPath:sentinel.path))
        let beforeRelease=try await store.profiles();XCTAssertEqual(beforeRelease.count,1)
        flock(fd,LOCK_UN)
        try await deletion.value
        let afterRelease=try await store.profiles();XCTAssertTrue(afterRelease.isEmpty)
    }
}
