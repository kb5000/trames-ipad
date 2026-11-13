// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest
@testable import TramesKit
final class ConfigurationTests: XCTestCase {
    func testLocalMemoryBudgetRejectsInvalidConfigurationBeforeOpeningStore() {
        let store = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        for (total, process) in [(0, 0), (1024, 2048), (-1, 512), ((1 << 28) + 1, 512)] {
            XCTAssertThrowsError(try LocalSession(store: store, rootfs: nil, total: 1 << 30, available: 1 << 30,
                                                 memoryBudgetMiB: total, processMemoryBudgetMiB: process))
            XCTAssertFalse(FileManager.default.fileExists(atPath: store.path))
        }
    }
    func testNativeConfigurationUsesSharedTomlAndRejectsUnknownFields() throws {
        let data=try LocalSession.environmentConfig(["action":"defaults","distribution":"debian","name":"工作环境","uuid":UUID().uuidString.lowercased()])
        let text=try XCTUnwrap(String(data:data,encoding:.utf8))
        let value=try XCTUnwrap(try JSONSerialization.jsonObject(with:LocalSession.environmentConfig(["action":"decode","toml":text])) as? [String:Any])
        XCTAssertEqual((value["environment"] as? [String:Any])?["name"] as? String,"工作环境")
        XCTAssertEqual((value["network"] as? [String:Any])?["mode"] as? String,"host")
        XCTAssertThrowsError(try LocalSession.environmentConfig(["action":"decode","toml":"[resources]\ncpuz=4"]))
    }

}
