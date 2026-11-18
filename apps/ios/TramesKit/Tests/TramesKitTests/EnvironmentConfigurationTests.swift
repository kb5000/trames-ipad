// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import XCTest
@testable import TramesKit

final class EnvironmentConfigurationTests:XCTestCase {
    private func configuration(_ uuid:String=UUID().uuidString.lowercased()) throws -> EnvironmentConfiguration {
        let data=try LocalSession.environmentConfig(["action":"defaults","distribution":"debian","name":"测试 Debian","uuid":uuid])
        return try EnvironmentConfiguration(toml:String(decoding:data,as:UTF8.self))
    }
    private func files(_ root:URL)->EnvironmentConfigurationFiles {EnvironmentConfigurationFiles(configuration:root.appendingPathComponent("profiles/env.toml"),authorization:root.appendingPathComponent("grants.toml"))}
    func testNativeCodecPreservesBooleansArraysAndCrossPlatformFields() throws {
        var value=try configuration()
        value.set("resources.cpus",.number(2));value.set("resources.memory",.string("4GiB"));value.set("components.vulkan",.bool(false))
        value.set("session.shell",.array(["/bin/sh","-c","printf '%s' 'two words'"].map(BridgeValue.string)))
        value.set("mounts",.array([.object(["share":.string("work"),"host":.null,"target":.string("/work"),"readonly":.bool(true)])]))
        value.set("interop.files.shares",.array([.string("work")]))
        let actual=try EnvironmentConfiguration(toml:String(decoding:value.toml(),as:UTF8.self))
        XCTAssertEqual(actual,value);XCTAssertEqual(actual.get("components.vulkan"),.bool(false));XCTAssertEqual(actual.get("resources.cpus"),.number(2))
        value.set("resources.memory",.string("incorrect"));XCTAssertThrowsError(try value.toml())
        value.set("components.vulkan",.number(0));XCTAssertThrowsError(try value.toml())
    }
    func testDraftSaveIsIsolatedAndAnImportedRequestCannotGrantHostAccess() throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("config-model-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let file=files(root),defaults=try configuration().toml();let old=try file.load(defaults:defaults)
        XCTAssertTrue(old.permissions.values.allSatisfy{!$0})
        var draft=old.document;draft.set("resources.memory",.string("2GiB"));draft.set("interop.clipboard.enabled",.bool(true))
        let saved=try file.save(draft,permissions:old.permissions,replacing:old)
        XCTAssertFalse(saved.permissions["clipboard"] ?? true);XCTAssertFalse(saved.document.bool("interop.clipboard.enabled"))
        let sibling=files(root.appendingPathComponent("other"));let otherDefaults=try configuration().toml();_=try sibling.load(defaults:otherDefaults)
        let original=try Data(contentsOf:file.configuration)
        var permission=saved.permissions;permission["clipboard"]=true
        let allowed=try file.save(saved.document,permissions:permission,replacing:saved)
        XCTAssertTrue(allowed.document.bool("interop.clipboard.enabled"));XCTAssertTrue(allowed.authorization.bool("grant.clipboard"))
        XCTAssertNotEqual(try Data(contentsOf:file.configuration),original)
        XCTAssertFalse(FileManager.default.fileExists(atPath:sibling.configuration.path))
        XCTAssertEqual(try file.load(defaults:defaults),allowed)
    }
    func testStaleInvalidOrReidentifiedDraftNeverOverwritesExistingFiles() throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("config-conflict-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let file=files(root),defaults=try configuration().toml(),first=try file.load(defaults:defaults)
        let saved=try file.save(first.document,permissions:["clipboard":true],replacing:first)
        let config=try Data(contentsOf:file.configuration),grants=try Data(contentsOf:file.authorization)
        var invalid=saved.document;invalid.set("resources.memory",.string("broken"))
        XCTAssertThrowsError(try file.save(invalid,permissions:saved.permissions,replacing:saved))
        invalid=saved.document;invalid.set("environment.uuid",.string(UUID().uuidString.lowercased()))
        XCTAssertThrowsError(try file.save(invalid,permissions:saved.permissions,replacing:saved))
        XCTAssertEqual(try Data(contentsOf:file.configuration),config);XCTAssertEqual(try Data(contentsOf:file.authorization),grants)
        let external=config+Data("\n# external editor\n".utf8);try external.write(to:file.configuration)
        XCTAssertThrowsError(try file.save(saved.document,permissions:saved.permissions,replacing:saved))
        XCTAssertEqual(try Data(contentsOf:file.configuration),external);XCTAssertEqual(try Data(contentsOf:file.authorization),grants)
    }
    func testMismatchedAuthorizationIsNotReplacedWithAnImplicitGrant() throws {
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("config-identity-\(UUID().uuidString)")
        defer {try? FileManager.default.removeItem(at:root)}
        let file=files(root);try FileManager.default.createDirectory(at:root,withIntermediateDirectories:true)
        let other=try LocalSession.environmentConfig(["action":"grant","uuid":UUID().uuidString.lowercased(),"clipboard":true]);try other.write(to:file.authorization)
        XCTAssertThrowsError(try file.load(defaults:configuration().toml()))
        XCTAssertEqual(try Data(contentsOf:file.authorization),other)
    }
}

extension EnvironmentConfigurationTests {
    func testSavedConfigurationControlsRealGuestCpuMemoryAndWorkingDirectory() async throws {
        guard let archive=ProcessInfo.processInfo.environment["TRAMES_CONFIG_TEST_ROOTFS"] else {throw XCTSkip("Set TRAMES_CONFIG_TEST_ROOTFS to the existing Alpine fixture")}
        let root=FileManager.default.temporaryDirectory.appendingPathComponent("configured-guest-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at:root,withIntermediateDirectories:true)
        defer {try? FileManager.default.removeItem(at:root)}
        for memory in ["auto","256MiB"] {
            var value=try configuration();value.set("environment.type",.string("linux"));value.set("image.distribution",.string("alpine"));value.set("resources.cpus",.number(1));value.set("resources.memory",.string(memory));value.set("components.vulkan",.bool(false));value.set("display.windows",.bool(false));value.set("network.mode",.string("none"));value.set("session.cwd",.string("/tmp"));value.set("session.shell",.array(["/bin/sh","-c","printf 'CONFIG-CWD:'; pwd; printf 'CONFIG-CPUS:'; grep -c '^processor' /proc/cpuinfo; grep '^MemTotal:' /proc/meminfo"].map(BridgeValue.string)))
            let config=root.appendingPathComponent("env.toml");try value.toml().write(to:config)
            let local=try LocalSession(store:root.appendingPathComponent(memory),rootfs:URL(fileURLWithPath:archive),total:8<<30,available:4<<30,environmentConfig:config)
            defer {local.close()}
            let deadline=Date().addingTimeInterval(30);var bytes=Data(),phase="starting"
            repeat {
                bytes.append(try local.receive());let state=try local.status();phase=state.phase
                if phase=="failed" {XCTFail(state.error ?? "guest failed");break}
                if phase=="exited" {bytes.append(try local.receive());XCTAssertEqual(state.exit_code,0);break}
                try await Task.sleep(for:.milliseconds(10))
            }while Date()<deadline
            XCTAssertEqual(phase,"exited",String(decoding:bytes,as:UTF8.self))
            let output=String(decoding:bytes,as:UTF8.self)
            XCTAssertTrue(output.contains("CONFIG-CWD:/tmp"),output);XCTAssertTrue(output.contains("CONFIG-CPUS:1"),output)
            let line=try XCTUnwrap(output.components(separatedBy:.newlines).first{$0.hasPrefix("MemTotal:")},output)
            let kib=try XCTUnwrap(UInt64(line.split(whereSeparator:\.isWhitespace)[1]))
            if memory=="auto" {XCTAssertGreaterThan(kib,1<<20)}else{XCTAssertEqual(kib,256<<10)}
            local.close()
        }
    }
}
