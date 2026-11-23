// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation

enum USBFileSharing {
    static func directory() throws -> URL {
        try FileManager.default.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: true)
    }

    static func filename(guestPath: String) throws -> String {
        let name = guestPath.components(separatedBy: "/").last ?? ""
        guard !name.isEmpty, name != ".", name != "..", !name.utf8.contains(0) else {
            throw CocoaError(.fileWriteInvalidFileName)
        }
        return name
    }

    /// Publish only the completed staging file. moveItem refuses an existing
    /// destination, including a dangling symlink; guest transfers never write
    /// directly into the directory exposed to the computer.
    static func publish(stagedFile: URL, to directory: URL) throws -> URL {
        let destination = directory.appendingPathComponent(stagedFile.lastPathComponent)
        var coordinationError: NSError?, moveError: Error?
        NSFileCoordinator().coordinate(writingItemAt: destination, options: [], error: &coordinationError) { coordinated in
            do { try FileManager.default.moveItem(at: stagedFile, to: coordinated) }
            catch { moveError = error }
        }
        if let error = moveError ?? coordinationError { throw error }
        return destination
    }
}
