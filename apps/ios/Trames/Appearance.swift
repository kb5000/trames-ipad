// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI

/// Device UI preference, independent of environment TOML and guest themes.
enum InterfaceStyle: String, CaseIterable, Identifiable {
    static let defaultsKey = "interfaceStyle"
    case system, light, dark
    var id: String { rawValue }
    var title: String {
        switch self { case .system: return "跟随系统"; case .light: return "亮色"; case .dark: return "暗色" }
    }
    var colorScheme: ColorScheme? {
        switch self { case .system: return nil; case .light: return .light; case .dark: return .dark }
    }
}
