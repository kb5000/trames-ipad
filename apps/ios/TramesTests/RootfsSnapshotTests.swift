// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import XCTest
@testable import Trames
final class RootfsSnapshotTests:XCTestCase {
    private func fixture() throws -> (URL,RootfsSnapshot) {
        let directory=FileManager.default.temporaryDirectory.appendingPathComponent("provider-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at:directory.appendingPathComponent("objects"),withIntermediateDirectories:true)
        let url=try XCTUnwrap(Bundle(for:Self.self).url(forResource:"rootfs-manifest",withExtension:"hex",subdirectory:"Fixtures"))
        let characters=Array(try String(contentsOf:url,encoding:.utf8).trimmingCharacters(in:.whitespacesAndNewlines))
        let bytes=try stride(from:0,to:characters.count,by:2).map {try XCTUnwrap(UInt8(String(characters[$0..<$0+2]),radix:16))}
        try Data(bytes).write(to:directory.appendingPathComponent("manifest"))
        let snapshot=try RootfsSnapshot(store:directory)
        let entry=try XCTUnwrap(snapshot.entries[Data("/root/readme.txt".utf8)])
        try Data("rootfs fixture".utf8).write(to:snapshot.object(entry))
        return (directory,snapshot)
    }
    func testNewNestedEnvironmentResolvesTheExistingRootBeforeMissingChildren() throws {
        let parent=FileManager.default.temporaryDirectory.appendingPathComponent("environment-path-\(UUID().uuidString)",isDirectory:true)
        let actual=parent.appendingPathComponent("actual",isDirectory:true),alias=parent.appendingPathComponent("alias",isDirectory:true)
        try FileManager.default.createDirectory(at:actual,withIntermediateDirectories:true)
        defer {try? FileManager.default.removeItem(at:parent)}
        try FileManager.default.createSymbolicLink(at:alias,withDestinationURL:actual)
        let profile=RootfsEnvironment(id:"new-id",name:"Debian",store:"environments/new-id",distribution:"debian")
        let resolved=try profile.storeURL(in:alias)
        XCTAssertEqual(resolved.path,actual.resolvingSymlinksInPath().appendingPathComponent("environments/new-id").path)
        let catalog=RootfsCatalog(version:1,environments:[profile]);try catalog.save(alias)
        XCTAssertEqual(try RootfsCatalog.load(alias).environments,[profile])
    }
    func testLegacyCatalogAndNewEnvironmentMetadataRoundTrip() throws {
        let legacy=try JSONDecoder().decode(RootfsCatalog.self,from:Data("{\"version\":1,\"environments\":[{\"id\":\"alpine\",\"name\":\"Alpine\",\"store\":\"environment\"}]}".utf8))
        XCTAssertEqual(legacy.environments[0].distro,"alpine")
        XCTAssertEqual(legacy.environments[0].boot,"shell")
        let debian=RootfsEnvironment(id:"debian",name:"Debian",store:"environments/debian",distribution:"debian",initMode:"systemd",packages:"standard",needsSetup:true)
        let roundTrip=try JSONDecoder().decode(RootfsCatalog.self,from:JSONEncoder().encode(RootfsCatalog(version:1,environments:legacy.environments+[debian])))
        XCTAssertEqual(roundTrip.environments[1],debian)
        XCTAssertEqual(roundTrip.environments[0],legacy.environments[0])
    }
    func testPendingImportDoesNotHideInstalledEnvironmentsFromFiles() throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("pending-rootfs-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at:root,withIntermediateDirectories:true)
        defer {try? FileManager.default.removeItem(at:root)}
        let (installed,_)=try fixture()
        try FileManager.default.moveItem(at:installed,to:root.appendingPathComponent("environment"))
        let pending=RootfsEnvironment(id:"debian",name:"Debian",store:"environments/debian",distribution:"debian")
        let catalog=RootfsCatalog(version:1,environments:RootfsCatalog.alpine.environments+[pending])
        XCTAssertEqual(try catalog.installedEnvironments(in:root),RootfsCatalog.alpine.environments)
        let next=try pending.storeURL(in:root)
        try FileManager.default.createDirectory(at:next,withIntermediateDirectories:true)
        XCTAssertEqual(try catalog.installedEnvironments(in:root),RootfsCatalog.alpine.environments)
        try FileManager.default.copyItem(at:root.appendingPathComponent("environment/manifest"),to:next.appendingPathComponent("manifest"))
        XCTAssertEqual(try catalog.installedEnvironments(in:root),catalog.environments)
    }
    func testProductionManifestReadOnlyBrowsingAndExportPreserveHardlinks() throws {
        let (directory,snapshot)=try fixture();defer {try? FileManager.default.removeItem(at:directory)}
        let original=try Data(contentsOf:directory.appendingPathComponent("manifest"))
        let entry=try XCTUnwrap(snapshot.entries[Data("/root/readme.txt".utf8)]),alias=try XCTUnwrap(snapshot.entries[Data("/alias".utf8)])
        XCTAssertEqual(entry.inode,alias.inode);XCTAssertNotEqual(entry.identifier(environment:"alpine"),alias.identifier(environment:"alpine"))
        XCTAssertEqual(Set(snapshot.entries.values.filter{$0.parent==Data([47]) && $0.path != Data([47])}.map(\.filename)),["alias","root","link","dev"])
        let output=directory.appendingPathComponent("cache-copy.txt")
        try snapshot.materialize(entry,to:output)
        XCTAssertEqual(try Data(contentsOf:output),Data("rootfs fixture".utf8))
        XCTAssertEqual(try Data(contentsOf:directory.appendingPathComponent("manifest")),original)
        let link=try XCTUnwrap(snapshot.entries[Data("/link".utf8)])
        XCTAssertThrowsError(try snapshot.materialize(link,to:output))
        var broken=original;broken[15] ^= 1;try broken.write(to:directory.appendingPathComponent("manifest"))
        XCTAssertThrowsError(try RootfsSnapshot(store:directory))
        XCTAssertEqual(try Data(contentsOf:output),Data("rootfs fixture".utf8))
    }
    func testCatalogSupportsMultipleRootsAndRejectsEscapingStores() throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("roots-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at:root,withIntermediateDirectories:true);defer {try? FileManager.default.removeItem(at:root)}
        let catalog=RootfsCatalog(version:1,environments:[.init(id:"alpine",name:"Alpine",store:"environment"),.init(id:"debian",name:"Debian",store:"environments/debian"),.init(id:"android",name:"Android",store:"environments/android",distribution:"android",target:"android")])
        try catalog.save(root);XCTAssertEqual(try RootfsCatalog.load(root).environments,catalog.environments)
        XCTAssertEqual(catalog.environments[0].bootTarget,"linux")
        XCTAssertEqual(catalog.environments[2].bootTarget,"android")
        XCTAssertThrowsError(try RootfsEnvironment(id:"bad",name:"bad",store:"../outside").storeURL(in:root))
        try FileManager.default.createSymbolicLink(at:root.appendingPathComponent("escape"),withDestinationURL:root.deletingLastPathComponent())
        XCTAssertThrowsError(try RootfsEnvironment(id:"escape",name:"escape",store:"escape").storeURL(in:root))
    }
    func testEnablingSharedStoragePreservesExistingRootfsAndNoGroupUsesPrivateStore() throws {
        let base=FileManager.default.temporaryDirectory.appendingPathComponent("group-\(UUID().uuidString)")
        let local=base.appendingPathComponent("private"),shared=base.appendingPathComponent("shared")
        try FileManager.default.createDirectory(at:local.appendingPathComponent("environment"),withIntermediateDirectories:true)
        defer {try? FileManager.default.removeItem(at:base)}
        let marker=Data("keep existing Linux data".utf8)
        try marker.write(to:local.appendingPathComponent("environment/marker"))
        let blocked=base.appendingPathComponent("blocked")
        try Data().write(to:blocked)
        XCTAssertEqual(try RootfsLocations.prepare(privateRoot:local,sharedRoot:blocked),local)
        XCTAssertEqual(try Data(contentsOf:local.appendingPathComponent("environment/marker")),marker)
        XCTAssertEqual(try RootfsLocations.prepare(privateRoot:local,sharedRoot:nil),local)
        XCTAssertEqual(try RootfsLocations.prepare(privateRoot:local,sharedRoot:shared),shared)
        XCTAssertEqual(try Data(contentsOf:shared.appendingPathComponent("environment/marker")),marker)
        XCTAssertEqual(try RootfsCatalog.load(shared).environments.first?.id,"alpine")
    }
}
