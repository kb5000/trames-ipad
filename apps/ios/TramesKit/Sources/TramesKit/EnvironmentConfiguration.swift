// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation

/// A lossless JSON view of the shared Rust TOML model. Swift owns presentation;
/// the original codec owns schema validation, defaults and TOML serialization.
public struct EnvironmentConfiguration: Equatable, Sendable {
    public private(set) var value: BridgeValue
    public init(toml: String) throws {
        value = try JSONDecoder().decode(BridgeValue.self, from: LocalSession.environmentConfig(["action":"decode", "toml":toml]))
    }
    public init(value: BridgeValue) { self.value = value }
    public func toml(authorization: Bool = false) throws -> Data {
        let json = try JSONSerialization.jsonObject(with: JSONEncoder().encode(value))
        return try LocalSession.environmentConfig(["action":authorization ? "authorization-encode" : "encode", "value":json])
    }
    private static func keys(_ path: String) -> [String] {
        let prefix = "kernel.sysctl."
        if path.hasPrefix(prefix) {
            return ["kernel", "sysctl", String(path.dropFirst(prefix.count))]
        }
        return path.split(separator:".").map(String.init)
    }
    public func get(_ path: String) -> BridgeValue {
        Self.keys(path).reduce(value) { node, key in
            if case .object(let object) = node { return object[String(key)] ?? .null }; return .null
        }
    }
    public func string(_ path: String) -> String { get(path).text ?? "" }
    public func bool(_ path: String) -> Bool { if case .bool(let result) = get(path) { return result }; return false }
    public func strings(_ path: String) -> [String] { if case .array(let result) = get(path) { return result.compactMap(\.text) }; return [] }
    public mutating func set(_ path: String, _ new: BridgeValue) {
        func replacing(_ node: BridgeValue, _ keys: ArraySlice<String>) -> BridgeValue {
            guard let key = keys.first else { return new }
            var object: [String:BridgeValue]; if case .object(let old) = node { object = old } else { object = [:] }
            object[key] = replacing(object[key] ?? .null, keys.dropFirst()); return .object(object)
        }
        value = replacing(value, Self.keys(path)[...])
    }
    public static let iPadPermissions = ["clipboard", "files", "host_ui"]
}

public struct EnvironmentConfigurationSnapshot: Equatable, Sendable {
    public let document: EnvironmentConfiguration
    public let authorization: EnvironmentConfiguration
    public let configurationData: Data?
    public let authorizationData: Data?
    public var permissions: [String:Bool] {
        Dictionary(uniqueKeysWithValues: EnvironmentConfiguration.iPadPermissions.map {
            ($0, document.bool("interop.\($0).enabled") && authorization.bool("grant.\($0)"))
        })
    }
}

/// Cold manager I/O only. This never opens a guest filesystem or execution slot.
/// Imported/guest TOML is configuration, never an independent host grant.
public struct EnvironmentConfigurationFiles: Sendable {
    public let configuration: URL
    public let authorization: URL
    public init(configuration: URL, authorization: URL) { self.configuration=configuration; self.authorization=authorization }
    private func read(_ url: URL) throws -> Data? {
        guard FileManager.default.fileExists(atPath:url.path) else { return nil }
        let attributes = try url.resourceValues(forKeys:[.isRegularFileKey,.fileSizeKey])
        guard attributes.isRegularFile == true, let size=attributes.fileSize, size<=65536 else { throw TramesError.message("Invalid environment configuration file") }
        let data = try Data(contentsOf:url)
        guard data.count<=65536 else { throw TramesError.message("Environment configuration exceeds 64 KiB") }; return data
    }
    public func load(defaults: Data) throws -> EnvironmentConfigurationSnapshot {
        let config=try read(configuration), grants=try read(authorization)
        guard let text=String(data:config ?? defaults,encoding:.utf8) else { throw TramesError.message("Environment configuration must be UTF-8") }
        let document=try EnvironmentConfiguration(toml:text), auth:EnvironmentConfiguration
        if let grants {
            guard let text=String(data:grants,encoding:.utf8) else { throw TramesError.message("Invalid host authorization") }
            auth=try EnvironmentConfiguration(value:JSONDecoder().decode(BridgeValue.self,from:LocalSession.environmentConfig(["action":"authorization-decode","toml":text])))
        } else {
            // Missing authorization is denied. Only an explicit host save can
            // enable capabilities; importing a TOML cannot grant them.
            auth=EnvironmentConfiguration(value:.object(["environment_uuid":document.get("environment.uuid"),"grant":.object([:])]))
        }
        guard auth.string("environment_uuid")==document.string("environment.uuid") else { throw TramesError.message("Host grants belong to another environment") }
        return EnvironmentConfigurationSnapshot(document:document,authorization:auth,configurationData:config,authorizationData:grants)
    }
    public func save(_ document: EnvironmentConfiguration, permissions: [String:Bool], replacing old: EnvironmentConfigurationSnapshot) throws -> EnvironmentConfigurationSnapshot {
        guard try read(configuration)==old.configurationData, try read(authorization)==old.authorizationData else { throw TramesError.message("Configuration changed elsewhere. Reload before saving.") }
        for key in ["environment.uuid","environment.type","environment.name","image.distribution"] {
            guard document.get(key)==old.document.get(key) else { throw TramesError.message("An editor cannot replace the environment identity or installed image") }
        }
        var config=document, final=old.authorization, reduced=old.authorization
        for key in EnvironmentConfiguration.iPadPermissions {
            let allowed=permissions[key] ?? false
            config.set("interop.\(key).enabled",.bool(allowed)); final.set("grant.\(key)",.bool(allowed))
            reduced.set("grant.\(key)",.bool(old.authorization.bool("grant.\(key)") && allowed))
        }
        // Validate all three complete documents before changing any file.
        let configData=try config.toml(), reducedData=try reduced.toml(authorization:true), finalData=try final.toml(authorization:true)
        try FileManager.default.createDirectory(at:configuration.deletingLastPathComponent(),withIntermediateDirectories:true)
        try FileManager.default.createDirectory(at:authorization.deletingLastPathComponent(),withIntermediateDirectories:true)
        do {
            // Lower authority first; raise it only after matching configuration.
            // Each rename is atomic. This is not a power-loss transaction.
            try reducedData.write(to:authorization,options:.atomic)
            try configData.write(to:configuration,options:.atomic)
            try finalData.write(to:authorization,options:.atomic)
        } catch {
            if let data=old.configurationData { try? data.write(to:configuration,options:.atomic) } else { try? FileManager.default.removeItem(at:configuration) }
            if let data=old.authorizationData { try? data.write(to:authorization,options:.atomic) } else { try? FileManager.default.removeItem(at:authorization) }
            throw error
        }
        return try load(defaults:configData)
    }
}
