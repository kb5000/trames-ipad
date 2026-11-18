// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import XCTest
import TramesKit
@testable import Trames

@MainActor
final class EnvironmentConfigurationTests:XCTestCase {
    func testProfilesRemainIndependentAndSystemdPreparationIsPersisted() async throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("profile-edit-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let store=try EnvironmentStore(storageRoot:root)
        let a=try await store.create(name:"Debian A",distribution:"debian",initMode:"shell",packages:"minimal",archive:nil,sha256:nil)
        let b=try await store.create(name:"Debian B",distribution:"debian",initMode:"shell",packages:"minimal",archive:nil,sha256:nil)
        let first=try await store.configurationSnapshot(a),other=try await store.configurationSnapshot(b)
        var value=first.document;value.set("resources.cpus",.number(1));value.set("resources.memory",.string("4GiB"));value.set("boot.mode",.string("systemd"));value.set("components.audio",.bool(true))
        let saved=try await store.saveConfiguration(a,document:value,permissions:["clipboard":false,"files":true,"host_ui":true],replacing:first)
        XCTAssertEqual(saved.document.string("resources.memory"),"4GiB");XCTAssertFalse(saved.permissions["clipboard"] ?? true)
        let unchanged=try await store.configurationSnapshot(b);XCTAssertEqual(unchanged,other)
        let reopened=try EnvironmentStore(storageRoot:root),profiles=try await reopened.profiles()
        XCTAssertEqual(profiles.first{$0.id==a.id}?.boot,"systemd");XCTAssertEqual(profiles.first{$0.id==a.id}?.needsSetup,true)
        XCTAssertEqual(profiles.first{$0.id==b.id}?.boot,"shell")
        let exported=try await store.exportedConfiguration(a,document:saved.document)
        XCTAssertEqual(try EnvironmentConfiguration(toml:String(contentsOf:exported,encoding:.utf8)),saved.document)
    }
    func testImportedConfigurationGetsFreshIdentityAndTheEditorCannotGrantFromToml() async throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("profile-import-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let store=try EnvironmentStore(storageRoot:root),uuid=UUID().uuidString.lowercased()
        var input=try EnvironmentConfiguration(toml:String(decoding:LocalSession.environmentConfig(["action":"defaults","distribution":"debian","name":"Imported","uuid":uuid]),as:UTF8.self))
        input.set("components.vulkan",.bool(false));input.set("interop.clipboard.enabled",.bool(false))
        let profile=try await store.create(name:"Independent",distribution:"debian",initMode:"shell",packages:"minimal",archive:nil,sha256:nil,configuration:input)
        let snapshot=try await store.configurationSnapshot(profile)
        XCTAssertNotEqual(snapshot.document.string("environment.uuid"),uuid);XCTAssertEqual(snapshot.document.string("environment.name"),"Independent");XCTAssertFalse(snapshot.document.bool("components.vulkan"))
        let model=WorkspaceModel(prepareEnvironment:false),editor=EnvironmentEditor()
        // Exercise the exact editor merge without a guest or network download.
        editor.document=snapshot.document;editor.snapshot=snapshot;editor.permissions=snapshot.permissions
        var elevated=snapshot.document;elevated.set("interop.clipboard.enabled",.bool(true));try editor.applyTOML(String(decoding:elevated.toml(),as:UTF8.self))
        XCTAssertFalse(editor.permissions["clipboard"] ?? true)
        elevated.set("environment.uuid",.string(UUID().uuidString.lowercased()))
        XCTAssertThrowsError(try editor.applyTOML(String(decoding:elevated.toml(),as:UTF8.self)))
        XCTAssertTrue(model.sessions.isEmpty)
    }
}
