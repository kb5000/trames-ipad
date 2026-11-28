// SPDX-License-Identifier: GPL-3.0-or-later
#if DEBUG
import CryptoKit
import Foundation
import UIKit
import TramesKit

/// Explicit hosted acceptance scenario. It measures the actual linked runtime
/// in the App process, using a fresh private store and fixed guest declaration.
/// Sampling yields the existing async executor; no timer/service thread is added.
@MainActor
enum LocalMemoryProbe {
    struct Result {
        let summary: URL
        let samples: URL
        let terminal: URL
    }
    private struct Sample: Encodable {
        let seconds: Double
        let phase: String
        let process: LocalProcessMemory
    }
    static func run() async throws -> Result {
        let fm = FileManager.default
        guard let archive = Bundle.main.url(forResource: "alpine-minirootfs", withExtension: "tar.gz",
                                           subdirectory: "GuestVulkan/acceptance") else {
            throw TramesError.message("Missing pinned offline Alpine memory acceptance archive")
        }
        let source = try FileHandle(forReadingFrom: archive)
        var digest = SHA256()
        while let block = try source.read(upToCount: 1 << 20), !block.isEmpty { digest.update(data: block) }
        try source.close()
        let archiveSHA = digest.finalize().map { String(format: "%02x", $0) }.joined()
        let directory = fm.urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Diagnostics/memory-\(UUID().uuidString)", isDirectory: true)
        try fm.createDirectory(at: directory, withIntermediateDirectories: true)
        let store = directory.appendingPathComponent("store", isDirectory: true)
        let config = directory.appendingPathComponent("environment.toml")
        let declaration = """
        version = 1
        [environment]
        name = "memory-acceptance"
        type = "linux"
        [resources]
        cpus = 1
        memory = "512MiB"
        [network]
        mode = "none"
        [components]
        thunking = false
        vulkan = false
        vaapi = false
        audio = false
        audio_input = false
        """
        try Data(declaration.utf8).write(to: config)
        let samplesURL = directory.appendingPathComponent("samples.jsonl")
        let terminalURL = directory.appendingPathComponent("terminal.log")
        let summaryURL = directory.appendingPathComponent("memory.json")
        _ = fm.createFile(atPath: samplesURL.path, contents: nil)
        _ = fm.createFile(atPath: terminalURL.path, contents: nil)
        let samples = try FileHandle(forWritingTo: samplesURL)
        let terminal = try FileHandle(forWritingTo: terminalURL)
        defer { try? samples.close(); try? terminal.close() }
        let began = ProcessInfo.processInfo.systemUptime
        #if targetEnvironment(simulator)
        let physical = false
        #else
        let physical = true
        #endif
        var report: [String: Any] = [
            "state": "running", "scenario": "fresh archive import, one noninteractive idle shell, asynchronous stop",
            "scope": "hosted acceptance App process; manager, runtime and guest share process accounting",
            "baseline_kind": "loaded App before a fresh session, not a clean-install/cold-App-launch baseline",
            "physical_device": physical, "model": UIDevice.current.model,
            "system_version": UIDevice.current.systemVersion,
            "bundle_identifier": Bundle.main.bundleIdentifier ?? "", "bundle_version": Bundle.main.infoDictionary?["CFBundleVersion"] ?? "",
            "archive_sha256": archiveSHA, "environment_toml": declaration,
            "sample_interval_seconds": 0.1, "peak_kind": "sampled, not kernel high-water mark",
            "guest_rss_scope": "individual original shell /proc status in terminal.log; not added to host footprint"
        ]
        func save() throws {
            try JSONSerialization.data(withJSONObject: report, options: [.prettyPrinted, .sortedKeys])
                .write(to: summaryURL, options: .atomic)
        }
        func snapshot(_ phase: String) throws -> LocalProcessMemory {
            let memory = try LocalSession.processMemory()
            var line = try JSONEncoder().encode(Sample(seconds: ProcessInfo.processInfo.systemUptime - began,
                                                      phase: phase, process: memory))
            line.append(10); try samples.write(contentsOf: line)
            return memory
        }
        func samplePeriod(_ phase: String, seconds: Double) async throws {
            let until = ProcessInfo.processInfo.systemUptime + seconds
            while ProcessInfo.processInfo.systemUptime < until {
                let value = try snapshot(phase)
                if phase == "stopped", value.runtime_workers != 0 {
                    throw TramesError.message("Unexpected session worker during stopped sampling")
                }
                try await Task.sleep(for: .milliseconds(100))
            }
        }
        var session: LocalSession?
        try save()
        do {
            guard try snapshot("before-session").runtime_workers == 0 else {
                throw TramesError.message("Memory acceptance requires no existing local environment")
            }
            let script = "printf '\\nMEMORY-GUEST-STATUS-BEGIN\\n'; cat /proc/$$/status; " +
                "printf '\\nMEMORY-GUEST-CPU-BEGIN\\n'; cat /proc/cpuinfo; " +
                "printf '\\nMEMORY-IDLE-READY\\n'; read memory_exit"
            let local = try LocalSession(store: store, rootfs: archive, total: 2 << 30, available: 2 << 30,
                                         network: "none", memoryBudgetMiB: 512, processMemoryBudgetMiB: 512,
                                         command: ["/bin/sh", "-c", script], environmentConfig: config)
            session = local
            var startup = Data()
            while !String(decoding: startup, as: UTF8.self).contains("\r\nMEMORY-IDLE-READY\r\n") {
                guard ProcessInfo.processInfo.systemUptime - began < 300 else {
                    throw TramesError.message("Memory acceptance startup deadline")
                }
                _ = try snapshot("startup")
                let status = try local.status()
                guard status.phase == "starting" || status.phase == "running" else {
                    throw TramesError.message(status.error ?? "Memory acceptance guest stopped")
                }
                let bytes = try local.receive(); startup.append(bytes); try terminal.write(contentsOf: bytes)
                guard startup.count <= 2 << 20 else { throw TramesError.message("Unbounded memory acceptance startup output") }
                try await Task.sleep(for: .milliseconds(100))
            }
            let text = String(decoding: startup, as: UTF8.self)
            let cpuRows = text.components(separatedBy: "\n").filter { $0.hasPrefix("processor") }
            guard cpuRows.count == 1 else { throw TramesError.message("Memory acceptance CPU declaration was not applied") }
            startup.removeAll(keepingCapacity: false)
            try await samplePeriod("idle", seconds: 30)
            local.close(); session = nil
            let closeDeadline = ProcessInfo.processInfo.systemUptime + 30
            while try snapshot("closing").runtime_workers != 0 {
                guard ProcessInfo.processInfo.systemUptime < closeDeadline else {
                    throw TramesError.message("Memory acceptance worker retirement deadline")
                }
                try await Task.sleep(for: .milliseconds(100))
            }
            try await samplePeriod("stopped", seconds: 15)
            report["state"] = "complete"
        } catch {
            session?.close()
            report["state"] = "failed"; report["error"] = String(reflecting: error)
            try? samples.synchronize(); try? terminal.synchronize(); try? save()
            throw error
        }
        try samples.synchronize(); try terminal.synchronize(); try save()
        // Cleanup is outside the stopped measurements and only targets the
        // private store created by this probe. Diagnostics remain shareable.
        try await LocalSession.reset(store: store)
        return Result(summary: summaryURL, samples: samplesURL, terminal: terminalURL)
    }
}
#endif
