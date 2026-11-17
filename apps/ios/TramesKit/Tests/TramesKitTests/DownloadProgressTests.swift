// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest
import Foundation
@testable import TramesKit

final class DownloadProgressTests:XCTestCase {
    private final class Probe:@unchecked Sendable {
        let lock=NSLock()
        private var events:[(Int64,Int64)]=[]
        func append(_ done:Int64,_ total:Int64) {lock.lock();defer {lock.unlock()};events.append((done,total))}
        func values()->[(Int64,Int64)] {lock.lock();defer {lock.unlock()};return events}
    }
    func testReportsActualBytesBeforeCompletionAndRetainsDownloadedFile() async throws {
        guard let address=ProcessInfo.processInfo.environment["TRAMES_BOOTSTRAP_TEST_URL"],let base=URL(string:address) else {throw XCTSkip("Run scripts/test-swift-bootstrap.py for the streaming fixture")}
        let probe=Probe()
        let (path,response)=try await ConnectivityRetry.download(from:base.appendingPathComponent("image"),progress:{probe.append($0,$1)})
        defer {try? FileManager.default.removeItem(at:path)}
        let bytes=try Data(contentsOf:path)
        XCTAssertEqual((response as? HTTPURLResponse)?.statusCode,200)
        XCTAssertEqual(bytes,Data(repeating:0x61,count:4*65536))
        let values=probe.values()
        XCTAssertTrue(values.contains {$0.0>0 && $0.0<4*65536},"Must report progress while the body is still arriving")
        XCTAssertTrue(values.contains {$0.0==4*65536 && $0.1==4*65536})
        XCTAssertTrue(zip(values,values.dropFirst()).allSatisfy {$0.0.0 <= $0.1.0})
    }
    func testActiveLargeTransferOutlastsTheFormerResourceDeadline() async throws {
        guard let address=ProcessInfo.processInfo.environment["TRAMES_BOOTSTRAP_TEST_URL"],let base=URL(string:address) else {throw XCTSkip("Run scripts/test-swift-bootstrap.py for the slow fixture")}
        let began=Date()
        let (path,_)=try await ConnectivityRetry.download(from:base.appendingPathComponent("slow"))
        defer {try? FileManager.default.removeItem(at:path)}
        XCTAssertEqual(try Data(contentsOf:path),Data(repeating:0x61,count:4*65536))
        XCTAssertGreaterThanOrEqual(Date().timeIntervalSince(began),64)
    }
    func testCancellationStopsAnInFlightDownload() async throws {
        guard let address=ProcessInfo.processInfo.environment["TRAMES_BOOTSTRAP_TEST_URL"],let base=URL(string:address) else {throw XCTSkip("Run scripts/test-swift-bootstrap.py for the streaming fixture")}
        let started=expectation(description:"Download body started")
        started.assertForOverFulfill=false
        let task=Task {
            try await ConnectivityRetry.download(from:base.appendingPathComponent("image"),progress:{done,_ in if done>0 {started.fulfill()}})
        }
        await fulfillment(of:[started],timeout:5)
        task.cancel()
        do {
            let (path,_)=try await task.value
            try? FileManager.default.removeItem(at:path)
            XCTFail("Expected cancellation")
        } catch {XCTAssertTrue(error is CancellationError)}
    }
}
