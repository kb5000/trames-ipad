// swift-tools-version:6.0
import PackageDescription
import Foundation
let libraryDirectory = ProcessInfo.processInfo.environment["TRAMES_RUST_LIB_DIR"]
let nativeLinkerSettings: [LinkerSetting] = [.linkedLibrary("trames_ffi")] + (libraryDirectory.map { [.unsafeFlags(["-L", $0])] } ?? [])
let package = Package(
    name: "TramesKit", platforms: [.iOS(.v17), .macOS(.v15)],
    products: [.library(name: "TramesKit", targets: ["TramesKit"])],
    targets: [
        .systemLibrary(name: "CTrames"),
        .target(name: "TramesKit", dependencies: ["CTrames"], linkerSettings: nativeLinkerSettings),
        .testTarget(name: "TramesKitTests", dependencies: ["TramesKit"])],
    swiftLanguageModes: [.v5])
