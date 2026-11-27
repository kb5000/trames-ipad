// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import CryptoKit
import XCTest
import TramesKit
@testable import Trames

@MainActor
final class EnvironmentConfigurationTests:XCTestCase {
    func testSysctlNamesRemainLiteralThroughDraftAndNativeCodec() throws {
        let defaults=try LocalSession.environmentConfig(["action":"defaults","distribution":"alpine","name":"Task limit","uuid":UUID().uuidString.lowercased()])
        var value=try EnvironmentConfiguration(toml:String(decoding:defaults,as:UTF8.self))
        value.set("kernel.sysctl.kernel.threads-max",.number(300))
        XCTAssertEqual(value.get("kernel.sysctl.kernel.threads-max"),.number(300))
        XCTAssertEqual(value.get("kernel.sysctl"),.object(["kernel.threads-max":.number(300)]))
        let actual=try EnvironmentConfiguration(toml:String(decoding:value.toml(),as:UTF8.self))
        XCTAssertEqual(actual,value)
        for invalid in [0.0,1.5,1073741824.0] {
            var draft=value;draft.set("kernel.sysctl.kernel.threads-max",.number(invalid))
            XCTAssertThrowsError(try draft.toml())
        }
        value.set("kernel.sysctl.kernel.unknown",.number(2))
        XCTAssertThrowsError(try value.toml())
    }

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

/// Runs inside the installed app through the environment manager and its FFI.
/// Stage hash-checked ELF inputs explicitly; ordinary unit jobs do not download.
@MainActor
final class LocalTaskCapacityTests:XCTestCase {
    func testProductTaskLimitWithOneCPU() async throws {try await run(cpus:1)}
    func testProductTaskLimitWithFourCPUs() async throws {try await run(cpus:4)}

    private func run(cpus:Int) async throws {
        let fm=FileManager.default
        guard let inputs=Bundle(for:Self.self).resourceURL?.appendingPathComponent("Fixtures/TaskCapacity"),
              fm.fileExists(atPath:inputs.appendingPathComponent("manifest.json").path) else {
            throw XCTSkip("Stage verified TaskCapacity acceptance inputs")
        }
        struct Inputs:Decodable {let files:[String:String];let rootfs_sha256:String}
        let manifest=try JSONDecoder().decode(Inputs.self,from:Data(contentsOf:inputs.appendingPathComponent("manifest.json")))
        func digest(_ url:URL) throws -> String {SHA256.hash(data:try Data(contentsOf:url)).map{String(format:"%02x",$0)}.joined()}
        guard Set(manifest.files.keys)==Set(["task-limit","task-capacity"]) else {throw TramesError.message("Invalid task-capacity manifest")}
        for (name,sha) in manifest.files {
            guard try digest(inputs.appendingPathComponent(name))==sha else {throw TramesError.message("Task-capacity hash mismatch: "+name)}
        }
        let archive=try XCTUnwrap(Bundle.main.url(forResource:"alpine-minirootfs",withExtension:"tar.gz",subdirectory:"GuestVulkan/acceptance"))
        guard try digest(archive)==manifest.rootfs_sha256 else {throw TramesError.message("Task-capacity rootfs hash mismatch")}
        let root=fm.temporaryDirectory.appendingPathComponent("task-limit-product-\(UUID().uuidString)")
        defer {try? fm.removeItem(at:root)}
        let initial=try EnvironmentStore(storageRoot:root)
        let profiles=try await initial.profiles()
        let profile=try XCTUnwrap(profiles.first)
        let disk=try profile.storeURL(in:root)
        try LocalSession.install(store:disk,rootfs:archive,total:2<<30,available:2<<30,muslLibc:EnvironmentStore.muslLibrary(),sha256:manifest.rootfs_sha256)
        let snapshot=try await initial.configurationSnapshot(profile)
        var document=snapshot.document
        document.set("resources.cpus",.number(Double(cpus)))
        document.set("resources.memory",.string("512MiB"))
        document.set("network.mode",.string("none"))
        document.set("components.vulkan",.bool(false))
        _=try await initial.saveConfiguration(profile,document:document,permissions:snapshot.permissions,replacing:snapshot)

        for stage in ["default","configured","restart","capacity"] {
            let store=try EnvironmentStore(storageRoot:root)
            let before=try await store.configurationSnapshot(profile)
            if stage == "configured" || stage == "capacity" {
                var value=before.document
                value.set("kernel.sysctl.kernel.threads-max",.number(stage == "capacity" ? 600:64))
                let saved=try await store.saveConfiguration(profile,document:value,permissions:before.permissions,replacing:before)
                XCTAssertEqual(saved.document.get("kernel.sysctl.kernel.threads-max"),stage == "capacity" ? .number(600):.number(64))
                if stage == "configured" {
                    var invalid=saved.document;invalid.set("kernel.sysctl.kernel.threads-max",.number(0))
                    do {_=try await store.saveConfiguration(profile,document:invalid,permissions:saved.permissions,replacing:saved);XCTFail("Invalid task limit was saved")}
                    catch { /* Complete document validation precedes publication. */ }
                    let current=try await store.configurationSnapshot(profile)
                    XCTAssertEqual(current,saved)
                }
            }
            let local=try await store.start(profile:profile,useVolumeBudget:false)
            do {
                try local.pause(false)
                if stage == "default" {
                    for name in manifest.files.keys {try local.transfer(guest:"/tmp/"+name,host:inputs.appendingPathComponent(name),export:false)}
                }
                let command:String
                switch stage {
                case "default":command="chmod 755 /tmp/task-limit /tmp/task-capacity; /tmp/task-limit read 65536"
                case "configured":command="/tmp/task-limit live 64"
                case "restart":command="/tmp/task-limit read 64"
                default:command="/tmp/task-limit read 600 && /tmp/task-capacity 512"
                }
                let checked="test \"$(grep -c '^processor' /proc/cpuinfo)\" -eq \(cpus) && { "+command+"; }"
                let output=try await execute(local,command:checked,stage:stage,cpus:cpus)
                if stage == "configured" {XCTAssertTrue(output.contains("TASK_LIMIT_LIVE_OK:"),output);XCTAssertTrue(output.contains("final=65"),output)}
                if stage == "capacity" {XCTAssertEqual(output.components(separatedBy:"task-capacity-round:").count-1,4,output);XCTAssertTrue(output.contains("task-capacity-ok:"),output)}
                #if targetEnvironment(simulator)
                let physical=false
                #else
                let physical=true
                #endif
                let record:[String:Any] = ["stage":stage,"cpus":cpus,"physical_device":physical,"configured_limit":stage == "default" ? 65536:(stage == "capacity" ? 600:64)]
                print("TASK_CAPACITY_PRODUCT_RECORD:"+String(decoding:try JSONSerialization.data(withJSONObject:record,options:.sortedKeys),as:UTF8.self))
                local.close()
            } catch {
                local.close()
                try? await LocalSession.reset(store:disk,storeOwnerLock:root.appendingPathComponent("environment-owner.lock"))
                throw error
            }
        }
        try await LocalSession.reset(store:disk,storeOwnerLock:root.appendingPathComponent("environment-owner.lock"))
    }

    private func execute(_ local:LocalSession,command:String,stage:String,cpus:Int) async throws -> String {
        _=try local.receive()
        let line=command+"; printf '\\nTASK_LIMIT_HOST_DONE:%d\\n' $?\n"
        XCTAssertEqual(try local.send(Data(line.utf8)),line.utf8.count)
        let deadline=Date().addingTimeInterval(150)
        var bytes=Data()
        while String(decoding:bytes,as:UTF8.self).replacingOccurrences(of:"\r",with:"").range(of:"\\nTASK_LIMIT_HOST_DONE:[0-9]+\\n",options:.regularExpression)==nil {
            bytes.append(try local.receive())
            let status=try local.status()
            guard status.phase=="running",Date()<deadline,bytes.count<=2<<20 else {
                throw TramesError.message("Task capacity \(stage)/\(cpus): \(status.error ?? status.phase); "+String(decoding:bytes.suffix(8192),as:UTF8.self))
            }
            try await Task.sleep(for:.milliseconds(20))
        }
        let text=String(decoding:bytes,as:UTF8.self).replacingOccurrences(of:"\r",with:"")
        let attachment=XCTAttachment(string:text);attachment.name="task-capacity-\(stage)-\(cpus)";attachment.lifetime = .keepAlways;add(attachment)
        XCTAssertTrue(text.contains("\nTASK_LIMIT_HOST_DONE:0\n"),text)
        return text
    }
}
