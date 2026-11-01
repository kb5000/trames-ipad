// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import XCTest
@testable import Trames

final class LocalizationTests: XCTestCase {
    func testCatalogsHaveMatchingKeysAndFormatArguments() throws {
        func catalog(_ language:String) throws -> [String:String] {
            let path=try XCTUnwrap(Bundle.main.path(forResource:"Localizable",ofType:"strings",inDirectory:language+".lproj"))
            let data=try Data(contentsOf:URL(fileURLWithPath:path))
            return try XCTUnwrap(PropertyListSerialization.propertyList(from:data,format:nil) as? [String:String])
        }
        let english=try catalog("en"), chinese=try catalog("zh-Hans")
        XCTAssertEqual(Set(english.keys),Set(chinese.keys))
        for (key,value) in english {
            XCTAssertFalse(value.isEmpty,key)
            XCTAssertEqual(value.components(separatedBy:"%@").count,(try XCTUnwrap(chinese[key])).components(separatedBy:"%@").count,key)
        }
    }
    func testBothLanguagesKeepDynamicNamesAndLineEndings() throws {
        for (language,expected) in [("en","Downloading 测试 Debian…\r\n"),("zh-Hans","正在下载 测试 Debian…\r\n")] {
            let path=try XCTUnwrap(Bundle.main.path(forResource:language,ofType:"lproj"))
            let bundle=try XCTUnwrap(Bundle(path:path))
            let format=bundle.localizedString(forKey:"正在下载 %@…\r\n",value:nil,table:nil)
            XCTAssertEqual(String(format:format,"测试 Debian"),expected)
            XCTAssertNotEqual(bundle.localizedString(forKey:"待写回的编辑",value:nil,table:nil),"")
        }
        XCTAssertEqual(L10n.text("unknown-diagnostic-key"),"unknown-diagnostic-key")
    }
}
