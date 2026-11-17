// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import OSLog
import TramesKit

/// One cold preparation's terminal output and bounded, shareable event journal.
/// Producers call directly; no timer, polling task, or resource service thread.
final class PreparationDiagnostics:@unchecked Sendable {
    static var logURL:URL {
        FileManager.default.urls(for:.documentDirectory,in:.userDomainMask)[0]
            .appendingPathComponent("Diagnostics/environment-preparation.jsonl")
    }
    private let lock=NSLock()
    private let began=Date()
    private let output:@Sendable(Data)->Void
    private let environment:String
    private let logger=Logger(subsystem:Bundle.main.bundleIdentifier ?? "org.trames.Trames",category:"Preparation")
    private var file:FileHandle?
    private var written=0
    private var lastStage=""
    private var lastItem=""
    private var hasLine=false
    init(environment:String,output:@escaping @Sendable(Data)->Void) {
        self.environment=environment;self.output=output
        let url=Self.logURL,fm=FileManager.default
        do {
            try fm.createDirectory(at:url.deletingLastPathComponent(),withIntermediateDirectories:true)
            let previous=url.deletingPathExtension().appendingPathExtension("previous.jsonl")
            if fm.fileExists(atPath:url.path) {
                if fm.fileExists(atPath:previous.path) {try fm.removeItem(at:previous)}
                try fm.moveItem(at:url,to:previous)
            }
            _=fm.createFile(atPath:url.path,contents:nil)
            file=try FileHandle(forWritingTo:url)
        } catch {
            output(Data((L10n.text("无法保存准备日志：%@",error.localizedDescription)+"\r\n").utf8))
        }
        report(InstallationProgress(stage:"prepare",item:environment))
    }
    deinit {try? file?.close()}
    func report(_ event:InstallationProgress) {
        lock.lock();defer {lock.unlock()}
        let now=Date(),elapsed=now.timeIntervalSince(began)
        let record:[String:Any] = ["time":now.ISO8601Format(),"elapsed_seconds":elapsed,"environment":environment,
            "stage":event.stage,"item":event.item,"path":event.path,"completed":event.completed,"total":event.total,"files":event.files]
        if var data=try? JSONSerialization.data(withJSONObject:record,options:[.sortedKeys]) {
            data.append(10)
            if written+data.count>2*1024*1024 {
                try? file?.truncate(atOffset:0);try? file?.seek(toOffset:0);written=0
            }
            try? file?.write(contentsOf:data);written+=data.count
        }
        logger.info("stage=\(event.stage,privacy:.public) bytes=\(event.completed) total=\(event.total) entries=\(event.files)")
        let stage=Self.label(event.stage)
        let item=Self.clean(event.item)
        var line=stage+(item.isEmpty ? "" : " · "+item)
        if event.total>0 {
            let percent=Int(min(100,Double(event.completed)*100/Double(event.total)))
            line+=" · \(Self.bytes(event.completed)) / \(Self.bytes(event.total)) (\(percent)%)"
        } else if event.completed>0 {line+=" · "+Self.bytes(event.completed)}
        if event.files>0 {line+=" · "+L10n.text("%llu 项",event.files)}
        if !event.path.isEmpty {line+=" · "+Self.clean(event.path)}
        let time=now.formatted(date:.omitted,time:.standard)
        let changed=event.stage != lastStage || event.item != lastItem
        let prefix=hasLine && changed ? "\r\n" : ""
        output(Data((prefix+"\r\u{1b}[2K["+time+"] "+line).utf8))
        hasLine=true;lastStage=event.stage;lastItem=event.item
    }
    func finish(error:Error?=nil) {
        report(InstallationProgress(stage:error == nil ? "ready" : "failed",item:error?.localizedDescription ?? environment))
        lock.lock();defer {lock.unlock()}
        output(Data("\r\n".utf8));hasLine=false
        try? file?.synchronize()
    }
    private static func clean(_ value:String)->String {
        String(String.UnicodeScalarView(value.unicodeScalars.map {CharacterSet.controlCharacters.contains($0) ? " " : $0})).prefix(160).description
    }
    private static func bytes(_ n:UInt64)->String {ByteCountFormatter.string(fromByteCount:Int64(clamping:n),countStyle:.binary)}
    private static func label(_ stage:String)->String {
        switch stage {
        case "prepare":return L10n.text("准备环境")
        case "download":return L10n.text("下载镜像")
        case "verify","verify_cache":return L10n.text("校验镜像")
        case "verified":return L10n.text("镜像校验完成")
        case "queued":return L10n.text("等待安装任务开始")
        case "store":return L10n.text("打开环境存储")
        case "store_ready":return L10n.text("环境存储已就绪")
        case "archive","adapters":return L10n.text("读取适配组件与归档")
        case "unpack":return L10n.text("解包镜像")
        case "unpacked":return L10n.text("镜像解包完成")
        case "import":return L10n.text("导入文件")
        case "imported":return L10n.text("文件导入完成")
        case "sync":return L10n.text("保存环境存储")
        case "validate":return L10n.text("检查环境启动文件")
        case "complete":return L10n.text("环境导入完成")
        case "publish":return L10n.text("发布环境存储")
        case "packages":return L10n.text("安装环境软件")
        case "boot":return L10n.text("启动环境")
        case "ready":return L10n.text("环境准备完成")
        case "failed":return L10n.text("环境准备失败")
        default:return stage
        }
    }
}
