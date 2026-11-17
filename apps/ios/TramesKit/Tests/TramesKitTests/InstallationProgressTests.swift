// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest
import Foundation
@testable import TramesKit
final class InstallationProgressTests:XCTestCase {
    private final class Probe:@unchecked Sendable {
        let lock=NSLock()
        private var stages:[String]=[]
        func append(_ event:InstallationProgress) {lock.lock();defer {lock.unlock()};stages.append(event.stage)}
        func values()->[String] {lock.lock();defer {lock.unlock()};return stages}
    }
    func testColdInstallDeliversStoreAndArchiveCheckpointsBeforeMissingArchiveFailure() throws {
        let directory=FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer {try? FileManager.default.removeItem(at:directory)}
        let probe=Probe()
        XCTAssertThrowsError(try LocalSession.install(store:directory,rootfs:directory.appendingPathComponent("missing.tar.gz"),total:1<<30,available:1<<30,progress:{probe.append($0)}))
        XCTAssertEqual(probe.values(),["store","store_ready","archive"])
    }
    func testCancelledInstallerStopsAtTheNativeCallbackBeforeCreatingStorage() async throws {
        let directory=FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer {try? FileManager.default.removeItem(at:directory)}
        let task=Task.detached {
            withUnsafeCurrentTask {$0?.cancel()}
            try LocalSession.install(store:directory,rootfs:directory.appendingPathComponent("missing.tar.gz"),total:1<<30,available:1<<30,progress:{_ in})
        }
        do {try await task.value;XCTFail("Expected cancellation")}
        catch {XCTAssertTrue(error is CancellationError)}
        XCTAssertFalse(FileManager.default.fileExists(atPath:directory.path))
    }
}
