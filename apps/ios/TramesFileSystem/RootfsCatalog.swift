// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
#if canImport(Darwin)
import Darwin
#endif

enum RootfsCatalogError: Error { case invalidStore, noSuchItem, unavailable }
struct RootfsEnvironment: Codable, Equatable, Identifiable, Sendable {
    let id: String
    let name: String
    let store: String
    var distribution: String? = nil
    var initMode: String? = nil
    var packages: String? = nil
    var needsSetup: Bool? = nil
    var sourceSHA256: String? = nil
    var target: String? = nil
    // Boot mode is an in-memory UI cache of the TOML profile. This JSON file
    // is an installation/File Provider index, never configuration authority.
    enum CodingKeys:String,CodingKey {case id,name,store,distribution,packages,needsSetup,sourceSHA256,target}
    var bootTarget: String { target ?? (distro == "android" ? "android" : "linux") }
    var distro: String { distribution ?? "alpine" }
    var boot: String { initMode ?? "shell" }
    func storeURL(in root:URL) throws -> URL {
        guard !id.isEmpty,id.utf8.count<=64,id.allSatisfy({$0.isASCII && ($0.isLetter || $0.isNumber || $0=="-")}),
              !name.isEmpty,name.utf8.count<=255,!name.contains("/"),!name.utf8.contains(0),
              !store.hasPrefix("/"),!store.split(separator:"/",omittingEmptySubsequences:false).contains(where:{$0.isEmpty || $0=="." || $0==".."}) else { throw RootfsCatalogError.invalidStore }
        let canonicalRoot=root.resolvingSymlinksInPath()
        let url=canonicalRoot.appendingPathComponent(store,isDirectory:true).resolvingSymlinksInPath()
        guard url.path.hasPrefix(canonicalRoot.path+"/") else { throw RootfsCatalogError.invalidStore }
        return url
    }
}
struct RootfsCatalog: Codable {
    let version: Int
    let environments: [RootfsEnvironment]
    static let alpine=RootfsCatalog(version:1,environments:[RootfsEnvironment(id:"alpine",name:"Alpine",store:"environment")])
    func installedEnvironments(in root:URL) throws -> [RootfsEnvironment] {
        try environments.filter {FileManager.default.fileExists(atPath:try $0.storeURL(in:root).appendingPathComponent("manifest").path)}
    }
    static func load(_ root:URL) throws -> RootfsCatalog {
        let data=try Data(contentsOf:root.appendingPathComponent("roots.json"))
        guard data.count<=65536 else { throw RootfsCatalogError.invalidStore }
        let result=try JSONDecoder().decode(Self.self,from:data)
        guard result.version==1,result.environments.count<=64,Set(result.environments.map(\.id)).count==result.environments.count,Set(result.environments.map(\.store)).count==result.environments.count else { throw RootfsCatalogError.invalidStore }
        for environment in result.environments { _=try environment.storeURL(in:root) }
        return result
    }
    func save(_ root:URL) throws {
        try JSONEncoder().encode(self).write(to:root.appendingPathComponent("roots.json"),options:.atomic)
    }
}
enum RootfsLocations {
    static var group: String? { Bundle.main.object(forInfoDictionaryKey:"TramesAppGroup") as? String }
    static var sharedRoot: URL? {
        guard let group else { return nil }
        return FileManager.default.containerURL(forSecurityApplicationGroupIdentifier:group)?.appendingPathComponent("Trames",isDirectory:true)
    }
    static func prepare(privateRoot:URL,sharedRoot:URL?) throws -> URL {
        guard let sharedRoot else { return privateRoot }
        let fm=FileManager.default
        let old=privateRoot.appendingPathComponent("environment",isDirectory:true)
        let shared=sharedRoot.appendingPathComponent("environment",isDirectory:true)
        do {
            try fm.createDirectory(at:sharedRoot,withIntermediateDirectories:true)
            // Never choose a second root silently if both stores contain data.
            if fm.fileExists(atPath:old.path) && fm.fileExists(atPath:shared.path) { return privateRoot }
            // Finish fallible catalog work before moving existing user data.
            if !fm.fileExists(atPath:sharedRoot.appendingPathComponent("roots.json").path) { try RootfsCatalog.alpine.save(sharedRoot) }
            if fm.fileExists(atPath:old.path) { try fm.moveItem(at:old,to:shared) }
        } catch {
            if fm.fileExists(atPath:old.path) || !fm.fileExists(atPath:shared.path) { return privateRoot }
            throw error
        }
        return sharedRoot
    }
}
struct RootfsEntry {
    let path: Data
    let inode: UInt64
    let kind: UInt8
    let mode: UInt32
    let modified: Date
    var filename: String { String(decoding:path.split(separator:47).last ?? Data.SubSequence(),as:UTF8.self) }
    var parent: Data { guard let index=path.lastIndex(of:47),index>0 else { return Data([47]) };return Data(path[..<index]) }
    func identifier(environment:String) -> String { "node/\(environment)/p/\(String(RootfsSnapshot.hash(path),radix:16))" }
}
/// Reads the atomic v3/v4 checkpoint and complete incremental WAL frames without taking its writer lock.
/// No VFS instance is opened, no recovery runs and no namespace is modified.
struct RootfsSnapshot {
    let store: URL
    let revision: UInt64
    let entries: [Data:RootfsEntry]
    static func hash(_ data:Data) -> UInt64 { data.reduce(0xcbf29ce484222325) { ($0 ^ UInt64($1)) &* 0x100000001b3 } }
    init(store:URL) throws {
        let data=try Self.readImage(store)
        guard data.count>=48,data.count<=128*1024*1024 else { throw RootfsCatalogError.invalidStore }
        var tail=Reader(data:Data(data.suffix(8)))
        let checksum=try tail.uint(8)
        let body=Data(data.dropLast(8));guard Self.hash(body)==checksum else { throw RootfsCatalogError.invalidStore }
        var r=Reader(data:body)
        guard try r.take(8)==Data("TRAMESFS".utf8) else { throw RootfsCatalogError.invalidStore }
        let version=try r.uint(4);guard version==3 || version==4 else { throw RootfsCatalogError.invalidStore }
        _=try r.uint(8);let limit=try r.uint(4);let next=try r.uint(8)
        let count=try r.uint(4),paths=try r.uint(4)
        guard limit<=1_000_000,count>0,count<=limit,paths<=limit else { throw RootfsCatalogError.invalidStore }
        struct Node { let inode:UInt64;let kind:UInt8;let mode:UInt32;let modified:Date }
        var nodes:[UInt64:Node]=[:]
        for _ in 0..<count {
            let inode=try r.uint(8),mode=UInt32(try r.uint(4));_=try r.take(8)
            let kind=try r.take(4);guard let tag=kind.first,tag<=8,kind.dropFirst().allSatisfy({$0==0}),inode>0,inode<next,nodes[inode]==nil else { throw RootfsCatalogError.invalidStore }
            _=try r.take(16);let seconds=Int64(bitPattern:try r.uint(8));let nanos=try r.uint(4);_=try r.take(20)
            guard nanos<1_000_000_000 else { throw RootfsCatalogError.invalidStore }
            let size=try r.uint(4);guard size<4096 else { throw RootfsCatalogError.invalidStore };_=try r.take(Int(size))
            nodes[inode]=Node(inode:inode,kind:tag,mode:mode,modified:Date(timeIntervalSince1970:Double(seconds)+Double(nanos)/1_000_000_000))
        }
        if version==4 {
            let count=try r.uint(4);guard count<=nodes.count else { throw RootfsCatalogError.invalidStore }
            for _ in 0..<count {
                let inode=try r.uint(8),count=try r.uint(4);guard nodes[inode] != nil,count>0,count<=128 else { throw RootfsCatalogError.invalidStore }
                for _ in 0..<count { let name=try r.uint(4),value=try r.uint(4);guard name<=255,value<=65536 else { throw RootfsCatalogError.invalidStore };_=try r.take(Int(name+value)) }
            }
        }
        var entries:[Data:RootfsEntry]=[:]
        for _ in 0..<paths {
            let length=try r.uint(4),inode=try r.uint(8)
            guard length>0,length<4096,let node=nodes[inode] else { throw RootfsCatalogError.invalidStore }
            let path=try r.take(Int(length));let parts=path.dropFirst().split(separator:47,omittingEmptySubsequences:false)
            guard path.first==47,!path.contains(0),entries[path]==nil,path==Data([47]) || parts.allSatisfy({!$0.isEmpty && $0.count<=255 && $0 != Data(".".utf8) && $0 != Data("..".utf8)}) else { throw RootfsCatalogError.invalidStore }
            entries[path]=RootfsEntry(path:path,inode:inode,kind:node.kind,mode:node.mode,modified:node.modified)
        }
        guard r.offset==body.count,entries[Data([47])]?.kind==0 else { throw RootfsCatalogError.invalidStore }
        for entry in entries.values where entry.path != Data([47]) { guard entries[entry.parent]?.kind==0 else { throw RootfsCatalogError.invalidStore } }
        self.store=store;self.revision=checksum;self.entries=entries
    }
    static func signature(store:URL) throws -> String {
        try ["manifest","manifest.wal"].map { name in
            let path=store.appendingPathComponent(name).path
            if name == "manifest.wal", !FileManager.default.fileExists(atPath:path) {return "-"}
            let a=try FileManager.default.attributesOfItem(atPath:path)
            return "\(a[.systemFileNumber] ?? "")/\(a[.modificationDate] ?? "")/\(a[.size] ?? "")"
        }.joined(separator:"/")
    }
    private static func readImage(_ store:URL) throws -> Data {
        // A writer may replace the checkpoint between our two opens. Retry the
        // pair on a chain mismatch; no writer lock or recovery is acquired.
        for attempt in 0..<3 {
            do { return try readImageOnce(store) }
            catch { if attempt == 2 {throw error} }
        }
        throw RootfsCatalogError.invalidStore
    }
    private static func readImageOnce(_ store:URL) throws -> Data {
        var image=try Data(contentsOf:store.appendingPathComponent("manifest"))
        let url=store.appendingPathComponent("manifest.wal")
        guard FileManager.default.fileExists(atPath:url.path) else {return image}
        let size=try FileManager.default.attributesOfItem(atPath:url.path)[.size] as? NSNumber
        guard let size,size.uint64Value<=260*1024*1024 else {throw RootfsCatalogError.invalidStore}
        let wal=try Data(contentsOf:url)
        var r=Reader(data:wal),frames:[Data]=[]
        while wal.count-r.offset>=16 {
            guard try r.take(8)==Data("TRMWAL01".utf8) else {throw RootfsCatalogError.invalidStore}
            let length=try r.uint(8)
            guard length>=24,length<=256*1024*1024 else {throw RootfsCatalogError.invalidStore}
            if length+8>wal.count-r.offset {break}
            let body=try r.take(Int(length)),digest=try r.uint(8)
            guard Self.hash(body)==digest else {throw RootfsCatalogError.invalidStore}
            frames.append(body)
        }
        let current=Self.hash(image)
        var first=0
        for (i,body) in frames.enumerated() {
            var header=Reader(data:body);_=try header.uint(8)
            if try header.uint(8)==current {first=i+1}
        }
        if first==frames.count {return image}
        var spans=[JournalSpan(buffer:JournalBuffer(image),start:0,count:image.count)]
        var imageLength=image.count,expected=current
        for body in frames.dropFirst(first) {
            var d=Reader(data:body)
            guard try d.uint(8)==expected else {throw RootfsCatalogError.invalidStore}
            let target=try d.uint(8),length=try d.uint(8)
            guard length>=48,length<=128*1024*1024 else {throw RootfsCatalogError.invalidStore}
            var ends:[Int]=[],end=0
            for span in spans {end+=span.count;ends.append(end)}
            var next:[JournalSpan]=[],produced=0
            while d.offset<body.count {
                let tag=try d.uint(1)
                if tag==0 {
                    let start=try d.uint(8),count=try d.uint(8)
                    guard start<=imageLength,count>0,count<=imageLength-Int(start),count<=Int(length)-produced else {throw RootfsCatalogError.invalidStore}
                    produced+=Int(count)
                    var at=Int(start),remaining=Int(count),low=0,high=ends.count
                    while low<high {let mid=(low+high)/2;if ends[mid]<=at {low=mid+1}else{high=mid}}
                    var index=low
                    while remaining>0 {
                        let span=spans[index],base=index==0 ? 0:ends[index-1]
                        let offset=at-base,n=min(remaining,span.count-offset)
                        Self.appendSpan(&next,JournalSpan(buffer:span.buffer,start:span.start+offset,count:n))
                        at+=n;remaining-=n;index+=1
                    }
                } else if tag==1 {
                    let count=try d.uint(8)
                    guard count>0,count<=Int(length)-produced else {throw RootfsCatalogError.invalidStore}
                    let bytes=try d.take(Int(count))
                    Self.appendSpan(&next,JournalSpan(buffer:JournalBuffer(bytes),start:0,count:Int(count)))
                    produced+=Int(count)
                } else {throw RootfsCatalogError.invalidStore}
            }
            guard produced==Int(length) else {throw RootfsCatalogError.invalidStore}
            spans=next;imageLength=Int(length);expected=target
        }
        image=Self.flatten(spans)
        guard Self.hash(image)==expected else {throw RootfsCatalogError.invalidStore}
        return image
    }
    private final class JournalBuffer {
        let data:Data
        init(_ data:Data) {self.data=data}
    }
    private struct JournalSpan {
        let buffer:JournalBuffer
        let start:Int
        var count:Int
    }
    private static func flatten(_ spans:[JournalSpan]) -> Data {
        var data=Data();data.reserveCapacity(spans.reduce(0){$0+$1.count})
        for span in spans {data.append(span.buffer.data[span.start..<span.start+span.count])}
        return data
    }
    private static func appendSpan(_ spans:inout [JournalSpan],_ span:JournalSpan) {
        if let last=spans.last,last.buffer===span.buffer,last.start+last.count==span.start {
            spans[spans.count-1].count+=span.count
        } else {spans.append(span)}
        if spans.count>=32768 {
            let bytes=Self.flatten(spans)
            spans=[JournalSpan(buffer:JournalBuffer(bytes),start:0,count:bytes.count)]
        }
    }
    func object(_ entry:RootfsEntry) throws -> URL {
        guard entry.kind==1 else { throw RootfsCatalogError.noSuchItem }
        return store.appendingPathComponent("objects",isDirectory:true).appendingPathComponent(String(format:"%016llx.dat",entry.inode))
    }
    @discardableResult
    func materialize(_ entry:RootfsEntry,to destination:URL) throws -> RootfsFileVersion {
        let result=try RootfsEditStore.copy(object(entry),to:destination)
        return RootfsFileVersion(inode:entry.inode,sha256:result.sha256)
    }
    private struct Reader {
        let data:Data;var offset=0
        mutating func take(_ count:Int) throws -> Data {
            guard count>=0,offset<=data.count,count<=data.count-offset else { throw RootfsCatalogError.invalidStore }
            defer { offset+=count };return Data(data[offset..<offset+count])
        }
        mutating func uint(_ count:Int) throws -> UInt64 {
            try take(count).enumerated().reduce(0) { $0 | UInt64($1.element) << ($1.offset*8) }
        }
    }
}
