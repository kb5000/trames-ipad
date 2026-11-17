// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation

/// Read the same atomic namespace and inode objects as Files. This never opens
/// a second writer, pauses a guest, or adds a scheduler/service thread.
enum AndroidDiagnostics {
    private struct Entry:Encodable {
        let path:String
        let modified:Date
        let truncated:Bool
        let data:Data
    }
    private struct Capture:Encodable {
        let schema=1
        let captured:Date
        let environment:String
        let namespaceRevision:UInt64
        let files:[Entry]
        let errors:[String]
    }
    static func capture(root:URL,environment:RootfsEnvironment,logcatError:String?=nil) throws -> URL {
        let snapshot=try RootfsSnapshot(store:environment.storeURL(in:root))
        let prefixes=["/data/anr/","/data/tombstones/","/data/system/dropbox/"]
        let candidates=snapshot.entries.values.filter {entry in
            guard entry.kind==1,let path=String(data:entry.path,encoding:.utf8) else {return false}
            return path == "/data/local/tmp/trames-logcat.txt" || prefixes.contains {path.hasPrefix($0)}
        }.sorted {
            // Repeated boot warnings must not evict ANR, crash and watchdog
            // evidence from this bounded export.
            let lhs=String(decoding:$0.path,as:UTF8.self).contains("_wtf@")
            let rhs=String(decoding:$1.path,as:UTF8.self).contains("_wtf@")
            return lhs == rhs ? $0.modified>$1.modified : !lhs
        }
        var files:[Entry]=[],errors:[String]=logcatError.map {[$0]} ?? [],remaining=16*1024*1024
        for entry in candidates.prefix(64) {
            let path=String(decoding:entry.path,as:UTF8.self)
            do {
                let source=try snapshot.object(entry)
                let handle=try FileHandle(forReadingFrom:source);defer {try? handle.close()}
                let limit=min(2*1024*1024,remaining)
                guard limit>0 else {break}
                let data=try handle.read(upToCount:limit) ?? Data()
                let truncated = !(try handle.read(upToCount:1) ?? Data()).isEmpty
                files.append(Entry(path:path,modified:entry.modified,truncated:truncated,data:data))
                remaining-=data.count
            } catch {errors.append(path+": "+error.localizedDescription)}
        }
        let directory=PreparationDiagnostics.logURL.deletingLastPathComponent()
        try FileManager.default.createDirectory(at:directory,withIntermediateDirectories:true)
        let output=directory.appendingPathComponent("android-"+environment.id+".json")
        let encoder=JSONEncoder();encoder.dateEncodingStrategy = .iso8601
        try encoder.encode(Capture(captured:Date(),environment:environment.name,namespaceRevision:snapshot.revision,files:files,errors:errors)).write(to:output,options:.atomic)
        return output
    }
}
