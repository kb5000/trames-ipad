// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest

final class EnvironmentDeletionUITests:XCTestCase {
    func testStoppedAndroidCanBeDeletedOnlyAfterConfirmation() {
        continueAfterFailure=false
        let app=XCUIApplication()
        // This mode owns a separate, disposable configuration fixture store.
        app.launchArguments=["--ui-acceptance","--environment-ui-acceptance","--environment-ui-reset","-AppleLanguages","(zh-Hans)","-AppleLocale","zh_CN"]
        app.launch();defer {app.terminate()}
        let manager=app.buttons["open-manager"]
        XCTAssertTrue(manager.waitForExistence(timeout:20),app.debugDescription)
        manager.tap()
        let android=app.descendants(matching:.any).matching(identifier:"environment-row-android").firstMatch
        XCTAssertTrue(android.waitForExistence(timeout:10),app.debugDescription);android.tap()
        let title=app.staticTexts["environment-detail-name"]
        let selected=XCTNSPredicateExpectation(predicate:NSPredicate(format:"label == %@","Android"),object:title)
        XCTAssertEqual(XCTWaiter.wait(for:[selected],timeout:10),.completed)
        func requestDeletion() {
            app.buttons["更多"].firstMatch.tap()
            let delete=app.descendants(matching:.any).matching(identifier:"delete-environment").firstMatch
            XCTAssertTrue(delete.waitForExistence(timeout:5),app.debugDescription);delete.tap()
            XCTAssertTrue(app.buttons["删除环境"].firstMatch.waitForExistence(timeout:5),app.debugDescription)
        }
        requestDeletion()
        if app.buttons["取消"].firstMatch.exists {app.buttons["取消"].firstMatch.tap()}else{title.tap()}
        XCTAssertTrue(android.exists)
        requestDeletion();app.buttons["删除环境"].firstMatch.tap()
        let removed=XCTNSPredicateExpectation(predicate:NSPredicate(format:"exists == false"),object:android)
        XCTAssertEqual(XCTWaiter.wait(for:[removed],timeout:15),.completed)
        XCTAssertTrue(app.descendants(matching:.any).matching(identifier:"environment-row-alpine").firstMatch.exists)
        XCTAssertTrue(app.descendants(matching:.any).matching(identifier:"environment-row-debian").firstMatch.exists)
    }
}
