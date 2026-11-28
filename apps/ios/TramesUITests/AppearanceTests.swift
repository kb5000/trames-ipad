// SPDX-License-Identifier: GPL-3.0-or-later
import XCTest
import UIKit

final class AppearanceTests:XCTestCase {
    override func setUpWithError() throws {continueAfterFailure=false;XCUIDevice.shared.orientation = .landscapeLeft}
    private func launch()->XCUIApplication {
        let app=XCUIApplication()
        app.launchArguments=["--ui-acceptance","--environment-ui-acceptance","-AppleLanguages","(en)","-AppleLocale","en_US"]
        app.launch();XCTAssertTrue(app.buttons["session-actions"].waitForExistence(timeout:15));return app
    }
    private func settings(_ app:XCUIApplication) {
        app.buttons["open-manager"].tap()
        let row=app.descendants(matching:.any)["manager-row-settings"].firstMatch
        XCTAssertTrue(row.waitForExistence(timeout:8),app.debugDescription);row.tap()
        XCTAssertTrue(app.buttons["interface-style"].waitForExistence(timeout:5),app.debugDescription)
    }
    private func select(_ style:String,in app:XCUIApplication) {
        app.buttons["interface-style"].tap();app.buttons[style].firstMatch.tap()
        XCTAssertTrue(app.buttons["interface-style"].label.contains(style))
    }
    @discardableResult private func capture(_ name:String)->Double {
        let screenshot=XCUIScreen.main.screenshot()
        let attachment=XCTAttachment(screenshot:screenshot);attachment.name=name;attachment.lifetime = .keepAlways;add(attachment)
        // Sample the middle of the rendered form, not the dimmed window behind
        // it. A selected menu value alone cannot prove the open sheet updated.
        let image=screenshot.image.cgImage!
        let center=image.cropping(to:CGRect(x:Double(image.width)*0.36,y:Double(image.height)*0.3,width:Double(image.width)*0.28,height:Double(image.height)*0.4))!
        var pixel=[UInt8](repeating:0,count:4)
        pixel.withUnsafeMutableBytes {bytes in
            let context=CGContext(data:bytes.baseAddress,width:1,height:1,bitsPerComponent:8,bytesPerRow:4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
            context.interpolationQuality = .high;context.draw(center,in:CGRect(x:0,y:0,width:1,height:1))
        }
        return Double(Int(pixel[0])+Int(pixel[1])+Int(pixel[2]))/(3*255)
    }
    func testAppearanceSwitchesLiveAndPersistsAcrossLaunch() {
        var app=launch();settings(app);select("Light",in:app);XCTAssertGreaterThan(capture("iPad settings light"),0.65)
        select("Dark",in:app);XCTAssertLessThan(capture("iPad settings dark"),0.35)
        XCTAssertTrue(app.buttons["create-linux-environment"].waitForExistence(timeout:8));capture("iPad manager dark")
        app.terminate();app=launch();settings(app)
        XCTAssertTrue(app.buttons["interface-style"].label.contains("Dark"))
        select("Light",in:app)
        XCTAssertTrue(app.buttons["create-linux-environment"].waitForExistence(timeout:8));capture("iPad manager light")
        select("Follow System",in:app)
        app.terminate()
    }
}
