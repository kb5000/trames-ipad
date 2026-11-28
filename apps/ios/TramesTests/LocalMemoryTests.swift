// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest
@testable import Trames

final class LocalMemoryTests: XCTestCase {
    func testExplicitFreshSessionIdleAndStoppedMemoryAccounting() async throws {
        guard ProcessInfo.processInfo.environment["TRAMES_MEMORY_ACCEPTANCE"] == "1" else {
            throw XCTSkip("Explicit memory measurement is run once with the unified acceptance batch")
        }
        let result = try await LocalMemoryProbe.run()
        for (name, url) in [("memory-summary", result.summary), ("memory-samples", result.samples),
                            ("memory-terminal", result.terminal)] {
            let attachment = XCTAttachment(data: try Data(contentsOf: url), uniformTypeIdentifier: "public.plain-text")
            attachment.name = name; attachment.lifetime = .keepAlways; add(attachment)
        }
    }

    @MainActor
    func testMemoryPressureGuardProtectsAppFromCriticalOOM() {
        let workspace = WorkspaceModel(prepareEnvironment: false)
        let session = TerminalSession(title: "Alpine", isLocal: true)
        workspace.add(session)
        XCTAssertNotNil(workspace.localOwner)

        // Non-critical warning with healthy headroom keeps the session running.
        XCTAssertFalse(workspace.handleMemoryPressure(critical: false, availableBytes: 512 << 20))
        XCTAssertNotNil(workspace.localOwner)
        XCTAssertNil(workspace.error)

        // Warning with critically low available memory stops the session cleanly.
        XCTAssertTrue(workspace.handleMemoryPressure(critical: false, availableBytes: 64 << 20))
        XCTAssertNil(workspace.localOwner)
        XCTAssertNotNil(workspace.error)

        // Critical pressure always stops an active local session cleanly.
        workspace.error = nil
        let second = TerminalSession(title: "Debian", isLocal: true)
        workspace.add(second)
        XCTAssertNotNil(workspace.localOwner)
        XCTAssertTrue(workspace.handleMemoryPressure(critical: true, availableBytes: 0))
        XCTAssertNil(workspace.localOwner)
        XCTAssertNotNil(workspace.error)
    }
}
