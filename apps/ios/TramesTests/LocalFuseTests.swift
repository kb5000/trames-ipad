// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import CryptoKit
import XCTest
import TramesKit
@testable import Trames

/// Optional hardware acceptance inputs are staged into the test bundle; ordinary
/// unit-test jobs do not download or embed third-party Linux executables.
@MainActor
final class LocalFuseTests: XCTestCase {
    func testFuseWithOneCPU() async throws { try await run(cpus: 1) }
    func testFuseWithTwoCPUs() async throws { try await run(cpus: 2) }
    func testFuseWithFourCPUs() async throws { try await run(cpus: 4) }

    private func run(cpus: Int) async throws {
        let fm = FileManager.default
        guard let inputs = Bundle(for: Self.self).resourceURL?.appendingPathComponent("Fixtures/FuseAcceptance"),
              fm.fileExists(atPath: inputs.appendingPathComponent("manifest.json").path) else {
            throw XCTSkip("Stage the verified FuseAcceptance test inputs for device acceptance")
        }
        let archive = try XCTUnwrap(Bundle.main.url(forResource: "alpine-minirootfs", withExtension: "tar.gz", subdirectory: "GuestVulkan/acceptance"))
        struct Inputs: Decodable { let files: [String: String]; let rootfs_sha256: String }
        let manifest = try JSONDecoder().decode(Inputs.self, from: Data(contentsOf: inputs.appendingPathComponent("manifest.json")))
        let names = ["passthrough", "passthrough_ll", "client", "workload.sh", "acceptance.sh"]
        guard Set(manifest.files.keys) == Set(names) else { throw TramesError.message("Invalid FUSE input manifest") }
        func digest(_ url: URL) throws -> String { SHA256.hash(data: try Data(contentsOf: url)).map { String(format: "%02x", $0) }.joined() }
        for name in names { guard try digest(inputs.appendingPathComponent(name)) == manifest.files[name] else { throw TramesError.message("FUSE input hash mismatch: " + name) } }
        guard try digest(archive) == manifest.rootfs_sha256 else { throw TramesError.message("FUSE rootfs hash mismatch") }
        let root = fm.temporaryDirectory.appendingPathComponent("fuse-acceptance-\(UUID().uuidString)")
        try fm.createDirectory(at: root, withIntermediateDirectories: true)
        defer { try? fm.removeItem(at: root) }
        let config = root.appendingPathComponent("environment.toml")
        let toml = """
        version = 1
        [environment]
        name = "fuse-acceptance"
        uuid = "\(UUID().uuidString.lowercased())"
        [resources]
        cpus = \(cpus)
        memory = "1024MiB"
        [session]
        cwd = "/root"
        [network]
        mode = "none"
        [components]
        vulkan = false
        """
        _ = try LocalSession.environmentConfig(["action": "decode", "toml": toml])
        try Data(toml.utf8).write(to: config)
        let cases = ["busybox", "mapping", "copyout", "metadata", "special", "aio", "copy", "transfer", "cached", "ring", "unmount", "daemon-stop", "daemon-exit"]
        for profile in ["lowlevel", "writeback", "highlevel"] {
            let store = root.appendingPathComponent(profile)
            let session = try LocalSession(store: store, rootfs: archive, total: 2 << 30, available: 2 << 30,
                                           network: "off", muslLibc: try EnvironmentStore.muslLibrary(),
                                           environmentConfig: config)
            do {
                let startup = Date()
                while try session.status().phase == "starting" {
                    guard Date().timeIntervalSince(startup) < 60 else { throw TramesError.message("FUSE startup deadline") }
                    try await Task.sleep(for: .milliseconds(20))
                }
                XCTAssertEqual(try session.status().phase, "running")
                _ = try session.receive()
                for name in names {
                    try session.transfer(guest: "/tmp/fuse-" + name, host: inputs.appendingPathComponent(name), export: false)
                }
                let command = "mkdir -p /opt/fuse-tool; for n in passthrough passthrough_ll client workload.sh acceptance.sh; do cp /tmp/fuse-$n /opt/fuse-tool/$n; done; chmod 755 /opt/fuse-tool/*; /bin/sh /opt/fuse-tool/acceptance.sh \(profile); printf '\\nFUSE_HOSTED_DONE:%d\\n' $?\n"
                XCTAssertEqual(try session.send(Data(command.utf8)), command.utf8.count)
                let began = Date()
                var output = Data()
                while String(decoding: output, as: UTF8.self).range(of: "\\nFUSE_HOSTED_DONE:[0-9]+\\r?\\n", options: .regularExpression) == nil {
                    output.append(try session.receive())
                    let status = try session.status()
                    guard status.phase == "running", Date().timeIntervalSince(began) < 150, output.count <= 2 << 20 else {
                        throw TramesError.message("FUSE \(profile)/\(cpus) stopped or exceeded its budget: \(status.error ?? status.phase); " + String(decoding: output.suffix(4096), as: UTF8.self))
                    }
                    try await Task.sleep(for: .milliseconds(20))
                }
                let text = String(decoding: output, as: UTF8.self).replacingOccurrences(of: "\r", with: "")
                let attachment = XCTAttachment(string: text)
                attachment.name = "FUSE-\(profile)-\(cpus)"; attachment.lifetime = .keepAlways; add(attachment)
                let passed = text.split(separator: "\n").compactMap { line -> String? in
                    let parts = line.split(separator: " ")
                    guard parts.count == 3, parts[0] == "CASE", parts[2] == "PASS" else { return nil }
                    return String(parts[1])
                }
                XCTAssertEqual(passed.sorted(), cases.sorted(), text)
                XCTAssertTrue(text.contains("WORKLOAD \(profile) FAILED=0\n"), text)
                XCTAssertTrue(text.contains("\nFUSE_ACCEPTANCE_RESULT:0\n"), text)
                XCTAssertTrue(text.contains("\nFUSE_HOSTED_DONE:0\n"), text)
                if profile == "writeback" { XCTAssertTrue(text.contains("activating writeback"), text) }
                #if targetEnvironment(simulator)
                let physical = false
                #else
                let physical = true
                #endif
                let record: [String: Any] = ["profile": profile, "cpus": cpus, "physical_device": physical,
                                              "checks": cases.count, "writeback": profile == "writeback"]
                print("FUSE_DEVICE_RECORD:" + String(decoding: try JSONSerialization.data(withJSONObject: record, options: .sortedKeys), as: UTF8.self))
                session.close()
                try await LocalSession.reset(store: store)
            } catch {
                let attachment = XCTAttachment(string: "FUSE \(profile)/\(cpus): \(error)")
                attachment.name = "FUSE-error-\(profile)-\(cpus)"; attachment.lifetime = .keepAlways; add(attachment)
                session.close(); try? await LocalSession.reset(store: store); throw error
            }
        }
    }
}
