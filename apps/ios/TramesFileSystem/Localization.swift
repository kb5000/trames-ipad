// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation

/// Native UI only. Never changes guest LANG, LC_* or environment names.
enum L10n {
    static func text(_ key: String, _ arguments: CVarArg...) -> String {
        let value = Bundle.main.localizedString(forKey: key, value: nil, table: nil)
        return arguments.isEmpty ? value : String(format: value, locale: Locale.current, arguments: arguments)
    }
}
