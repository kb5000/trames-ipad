// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest
import UIKit
final class EnvironmentManagerTests:XCTestCase {
    override func setUpWithError() throws {continueAfterFailure=false;XCUIDevice.shared.orientation = .landscapeLeft}
    private func launch(_ language:String="zh-Hans",reset:Bool=true)->XCUIApplication {
        let app=XCUIApplication();app.launchArguments=["--ui-acceptance","--environment-ui-acceptance","-AppleLanguages","(\(language))","-AppleLocale",language=="en" ? "en_US":"zh_CN"]+(reset ? ["--environment-ui-reset"]:[]);app.launch();return app
    }
    private func manager(_ app:XCUIApplication,_ name:String="Linux 环境") {
        let menu=app.buttons["session-actions"];XCTAssertTrue(menu.waitForExistence(timeout:20),app.debugDescription);menu.tap();app.buttons[name].firstMatch.tap()
        XCTAssertTrue(app.buttons["create-linux-environment"].waitForExistence(timeout:15),app.debugDescription)
    }
    private func choose(_ app:XCUIApplication,_ distro:String) {
        let row=app.buttons["environment-row-"+distro].firstMatch
        let link=app.descendants(matching:.any)["environment-row-"+distro].firstMatch
        if row.exists {row.tap()}else{XCTAssertTrue(link.waitForExistence(timeout:5),app.debugDescription);link.tap()}
    }
    func testResourcePermissionAndDraftPersistenceAcrossEnvironmentsAndLaunch() {
        var app=launch();manager(app);choose(app,"debian")
        app.buttons["environment-tab-configuration"].tap()
        let cpu=app.buttons["environment-cpus"];XCTAssertTrue(cpu.waitForExistence(timeout:10),app.debugDescription);cpu.tap();app.buttons["1"].firstMatch.tap()
        let memory=app.buttons["environment-memory"];memory.tap();app.buttons["4GiB"].firstMatch.tap()
        app.buttons["environment-tab-permissions"].tap()
        let clipboard=app.switches["environment-permission-clipboard"];XCTAssertTrue(clipboard.waitForExistence(timeout:5),app.debugDescription)
        XCTAssertEqual(clipboard.value as? String,"1")
        clipboard.coordinate(withNormalizedOffset:CGVector(dx:0.95,dy:0.5)).tap()
        XCTAssertEqual(clipboard.value as? String,"0")
        app.buttons["save-environment-config"].tap()
        XCTAssertTrue(app.staticTexts["配置已保存，下次启动生效"].waitForExistence(timeout:8),app.debugDescription)
        choose(app,"alpine");app.buttons["environment-tab-configuration"].tap()
        XCTAssertTrue(app.buttons["environment-memory"].label.contains("自动"),app.debugDescription)
        choose(app,"debian");app.buttons["environment-tab-configuration"].tap()
        XCTAssertTrue(app.buttons["environment-memory"].label.contains("4GiB"),app.debugDescription)
        let cwd=app.textFields["environment-cwd"];cwd.tap();cwd.typeText("/tmp/draft")
        choose(app,"alpine")
        XCTAssertTrue(app.buttons["放弃更改"].waitForExistence(timeout:5));app.buttons["取消"].tap()
        XCTAssertEqual(app.staticTexts["environment-detail-name"].label,"Debian")
        app.buttons["还原草稿"].tap()
        app.terminate();app=launch(reset:false);manager(app);choose(app,"debian");app.buttons["environment-tab-configuration"].tap()
        XCTAssertTrue(app.buttons["environment-memory"].label.contains("4GiB"),app.debugDescription)
        app.buttons["environment-tab-permissions"].tap();XCTAssertEqual(app.switches["environment-permission-clipboard"].value as? String,"0")
        let attachment=XCTAttachment(screenshot:XCUIScreen.main.screenshot());attachment.name="Environment permissions - Chinese";attachment.lifetime = .keepAlways;add(attachment)
        app.terminate()
    }
    func testEnglishConfigurationAndroidAndCreationSheetInPortrait() {
        let app=launch("en");manager(app,"Linux Environments");choose(app,"android");app.buttons["environment-tab-configuration"].tap()
        XCTAssertTrue(app.staticTexts.matching(NSPredicate(format:"label ENDSWITH %@","Android init")).firstMatch.waitForExistence(timeout:5),app.debugDescription)
        XCTAssertFalse(app.textFields["environment-cwd"].exists)
        app.buttons["environment-tab-permissions"].tap();XCTAssertTrue(app.staticTexts.matching(NSPredicate(format:"label CONTAINS %@","Mac Only")).firstMatch.exists)
        XCUIDevice.shared.orientation = .portrait
        let new=app.buttons["create-linux-environment"]
        if !new.isHittable {app.navigationBars.buttons.firstMatch.tap()}
        new.tap();XCTAssertTrue(app.textFields["environment-name"].waitForExistence(timeout:8),app.debugDescription)
        app.buttons.matching(NSPredicate(format:"label BEGINSWITH %@","Distribution")).firstMatch.tap();app.buttons["Alpine"].firstMatch.tap()
        let name=app.textFields["environment-name"];name.tap();name.typeText(String(repeating:XCUIKeyboardKey.delete.rawValue,count:6)+"UI Alpine")
        XCTAssertTrue(app.buttons["confirm-create-environment"].isHittable,app.debugDescription)
        let attachment=XCTAttachment(screenshot:XCUIScreen.main.screenshot());attachment.name="New environment - English portrait keyboard";attachment.lifetime = .keepAlways;add(attachment)
        app.buttons["confirm-create-environment"].tap()
        let heading=app.staticTexts["environment-detail-name"]
        let created=XCTNSPredicateExpectation(predicate:NSPredicate(format:"label == %@","UI Alpine"),object:heading)
        XCTAssertEqual(XCTWaiter.wait(for:[created],timeout:10),.completed,app.debugDescription)
        app.terminate()
    }
}
