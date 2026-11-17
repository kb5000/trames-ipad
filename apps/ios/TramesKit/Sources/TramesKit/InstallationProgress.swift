// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import CTrames

public struct InstallationProgress:Codable,Sendable {
    public let stage:String
    public let item:String
    public let path:String
    public let completed:UInt64
    public let total:UInt64
    public let files:UInt64
    public init(stage:String,item:String="",path:String="",completed:UInt64=0,total:UInt64=0,files:UInt64=0) {
        self.stage=stage;self.item=item;self.path=path;self.completed=completed;self.total=total;self.files=files
    }
}
// Lives on the blocking install caller's stack. The C ABI borrows this box only
// until it returns; callbacks neither retain it nor dispatch back into Rust.
final class InstallationCallback {
    let progress:@Sendable(InstallationProgress)->Void
    var failure:Error?
    init(_ progress:@escaping @Sendable(InstallationProgress)->Void) {self.progress=progress}
}
let installationCallback:TramesInstallProgressCallback = { bytes,count,context in
    guard let context else {return 0}
    let observer=Unmanaged<InstallationCallback>.fromOpaque(context).takeUnretainedValue()
    if Task.isCancelled {return 0}
    do {
        guard let bytes,count<=65536 else {throw TramesError.message("Invalid installation progress")}
        observer.progress(try JSONDecoder().decode(InstallationProgress.self,from:Data(bytes:bytes,count:Int(count))))
        return 1
    } catch {observer.failure=error;return 0}
}
