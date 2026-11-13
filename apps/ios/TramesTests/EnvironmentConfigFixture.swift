// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import TramesKit
func authorizedBridgeConfiguration(_ directory:URL) throws -> (URL,URL) {
    let uuid=UUID().uuidString.lowercased(),config=directory.appendingPathComponent("env.toml"),grants=directory.appendingPathComponent("grants.toml")
    let text="""
    version = 1
    [environment]
    name = "test"
    uuid = "\(uuid)"
    [resources]
    cpus = 1
    [session]
    cwd = "/root"
    [network]
    mode = "none"
    [components]
    vulkan = false
    [interop.clipboard]
    enabled = true
    """
    _=try LocalSession.environmentConfig(["action":"decode","toml":text])
    try Data(text.utf8).write(to:config)
    try Data("environment_uuid = \"\(uuid)\"\n[grant]\nclipboard = true\n".utf8).write(to:grants)
    return(config,grants)
}
