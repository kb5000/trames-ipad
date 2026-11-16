// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import TramesKit
@MainActor
final class BridgeController: ObservableObject {
    private weak var local: LocalSession?
    var directories: ScopedDirectories?
    private var task: Task<Void,Never>?
    private var running: UInt64?
    func attach(_ session: LocalSession) { local = session }
    func poll() throws {
        guard let local else { return }
        if let running, !local.bridgeActive(running) { task?.cancel() }
        guard running == nil, let request = try local.pollBridge() else { return }
        if UserDefaults.standard.object(forKey: "bridgeEnabled") as? Bool == false {
            try local.rejectBridge(request.ticket, error: BridgeFailure(code: "denied", message: "Native Bridge is disabled in settings"))
        } else {
            // Environment admission, OS permissions and retained Files folder
            // authorizations decide access; no per-request Trames consent.
            executeHost(request,local:local)
        }
    }

    private func executeHost(_ host:BridgeHostRequest,local:LocalSession) {
        running = host.ticket
        task = Task {
            defer { running = nil; task = nil }
            do {
                try Task.checkCancellation()
                guard local.bridgeActive(host.ticket) else { return }
                let result = try await execute(host.request, session: local)
                if local.bridgeActive(host.ticket) { try local.replyBridge(host.ticket, result: result) }
            } catch {
                let failure = error as? BridgeFailure ?? BridgeFailure(code: error is CancellationError ? "cancelled" : "provider_error", message: error is CancellationError ? "Request cancelled" : "Native operation failed")
                if local.bridgeActive(host.ticket) { try? local.rejectBridge(host.ticket, error: failure) }
            }
        }
    }
    func close() { task?.cancel(); local = nil }
    private func execute(_ request: BridgeRequest, session: LocalSession) async throws -> BridgeValue {
        switch request.method {
        case "clipboard.read":
            try request.only([])
            let text = UIPasteboard.general.string ?? ""
            guard text.utf8.count <= 16384 else { throw BridgeFailure(code: "too_large", message: "Clipboard text exceeds 16 KiB") }
            return .object(["text": .string(text)])
        case "clipboard.write":
            try request.only(["text"])
            UIPasteboard.general.string = try request.string("text", maximum: 16384)
            return .object(["written": .bool(true)])
        case "url.open":
            try request.only(["url"])
            let url = try BridgeValidation.webURL(request.string("url"))
            let opened = await UIApplication.shared.open(url, options: [:])
            return .object(["opened": .bool(opened)])
        case "ui.manager":
            try request.only([])
            NotificationCenter.default.post(name:Notification.Name("TramesShowEnvironments"),object:nil)
            return .object(["opened": .bool(true)])
        case "device.info":
            try request.only([])
            return .object(["platform": .string(UIDevice.current.systemName), "version": .string(UIDevice.current.systemVersion), "model": .string(UIDevice.current.model)])
        case "files.list":
            try request.only([])
            guard let directories else { throw BridgeFailure(code: "unavailable", message: "Files authorizations are unavailable") }
            return .object(["shares": .array(await directories.list().map { .object(["alias": .string($0.id.uuidString), "name": .string($0.name), "provider_id": .string($0.id.uuidString), "readonly": .bool(!$0.writable)]) })])
        case "files.import", "files.export":
            try request.only(["grant", "path", "guest"])
            guard let directories, let id = UUID(uuidString: try request.string("grant")) else { throw BridgeFailure(code: "invalid_params", message: "Expected a folder authorization ID") }
            try await directories.transfer(id: id, relative: request.string("path"), guest: request.string("guest"), export: request.method == "files.export", session: session)
            return .object(["copied": .bool(true)])
        default: throw BridgeFailure(code: "method_not_found", message: "Unknown native capability")
        }
    }
}