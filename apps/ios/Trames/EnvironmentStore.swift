// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import CryptoKit
import TramesKit
actor EnvironmentStore {
    static let archiveURL = URL(string: "https://dl-cdn.alpinelinux.org/alpine/v3.22/releases/aarch64/alpine-minirootfs-3.22.6-aarch64.tar.gz")!
    static let debianURL = URL(string: "https://raw.githubusercontent.com/debuerreotype/docker-debian-artifacts/e36653f9967231373aeba7cff2102fd004ac4f4a/trixie/slim/oci/blobs/rootfs.tar.gz")!
    static func muslLibrary() throws -> URL {
        guard let url = Bundle.main.url(forResource:"libc",withExtension:"so",subdirectory:"GuestMusl") else {throw TramesError.message("Missing bundled Linux library")}
        return url
    }
    nonisolated let root: URL
    nonisolated let authorizationRoot: URL
    private var generation: UInt64 = 0
    private var resetting = false
    private var starting = false
    private var preparation: LocalSession?
    private var preparationPaused=false
    init() throws {
        let privateRoot=try FileManager.default.url(for:.applicationSupportDirectory,in:.userDomainMask,appropriateFor:nil,create:true).appendingPathComponent("Trames",isDirectory:true)
        authorizationRoot=privateRoot
        try FileManager.default.createDirectory(at:privateRoot,withIntermediateDirectories:true)
        let newCatalog = !FileManager.default.fileExists(atPath:privateRoot.appendingPathComponent("roots.json").path) && !FileManager.default.fileExists(atPath:(RootfsLocations.sharedRoot ?? privateRoot).appendingPathComponent("roots.json").path)
        root=try RootfsLocations.prepare(privateRoot:privateRoot,sharedRoot:RootfsLocations.sharedRoot)
        UserDefaults.standard.set(root==RootfsLocations.sharedRoot,forKey:"rootfsFileProviderAvailable")
        try FileManager.default.createDirectory(at:root,withIntermediateDirectories:true)
        try FileManager.default.createDirectory(at:root.appendingPathComponent("environments",isDirectory:true),withIntermediateDirectories:true)
        if !FileManager.default.fileExists(atPath:root.appendingPathComponent("roots.json").path) {try RootfsCatalog.alpine.save(root)}
        if newCatalog {
            let uuid=UUID().uuidString.lowercased(),profiles=root.appendingPathComponent("profiles",isDirectory:true)
            try FileManager.default.createDirectory(at:profiles,withIntermediateDirectories:true)
            try LocalSession.environmentConfig(["action":"defaults","distribution":"alpine","name":"Alpine","uuid":uuid]).write(to:profiles.appendingPathComponent("alpine.toml"),options:.atomic)
            try LocalSession.environmentConfig(["action":"grant","uuid":uuid,"clipboard":true]).write(to:privateRoot.appendingPathComponent("grants-alpine.toml"),options:.atomic)
        }
    }
    func profiles() throws -> [RootfsEnvironment] {
        try RootfsCatalog.load(root).environments.map { entry in
            var profile=entry;let path=configuration(entry)
            if FileManager.default.fileExists(atPath:path.path) {
                let text=try String(contentsOf:path,encoding:.utf8)
                let value=try JSONSerialization.jsonObject(with:LocalSession.environmentConfig(["action":"decode","toml":text])) as? [String:Any]
                profile.initMode=(value?["boot"] as? [String:Any])?["mode"] as? String
            }
            return profile
        }
    }
    private func ownerLock(_ profile:RootfsEnvironment) -> URL {
        authorizationRoot.appendingPathComponent(profile.id=="alpine" ? "environment-owner.lock" : "environment-\(profile.id)-owner.lock")
    }
    private func source(_ profile:RootfsEnvironment) -> URL {authorizationRoot.appendingPathComponent("rootfs-\(profile.id).archive")}
    private func androidImages(archive:URL,output:@escaping @Sendable(Data)->Void) async throws -> URL? {
        let images=try await Task.detached(priority:.userInitiated) {try LocalSession.androidImages(rootfs:archive)}.value
        guard !images.isEmpty else {return nil}
        let cache=authorizationRoot.appendingPathComponent("android-images",isDirectory:true)
        try FileManager.default.createDirectory(at:cache,withIntermediateDirectories:true)
        for image in images {
            try Task.checkCancellation()
            let destination=cache.appendingPathComponent(image.filename)
            if try await Self.verifiedAndroidImage(destination,image:image) {continue}
            output(Data(L10n.text("正在下载 %@…\r\n",image.filename).utf8))
            let (temporary,response)=try await ConnectivityRetry.download(from:image.url)
            defer {try? FileManager.default.removeItem(at:temporary)}
            guard (response as? HTTPURLResponse)?.statusCode==200,try await Self.verifiedAndroidImage(temporary,image:image) else {throw TramesError.message(L10n.text("Android 镜像大小或 SHA-256 校验失败"))}
            try Task.checkCancellation()
            // An invalid cached file is replaced only after the complete new
            // download passes. Every environment keeps separate writable data.
            if FileManager.default.fileExists(atPath:destination.path) {try FileManager.default.removeItem(at:destination)}
            try FileManager.default.moveItem(at:temporary,to:destination)
        }
        return cache
    }
    private nonisolated static func verifiedAndroidImage(_ file:URL,image:AndroidImageDownload) async throws -> Bool {
        try await Task.detached(priority:.utility) {
            guard FileManager.default.fileExists(atPath:file.path),(try file.resourceValues(forKeys:[.fileSizeKey])).fileSize.map(UInt64.init)==image.size else {return false}
            let handle=try FileHandle(forReadingFrom:file);defer {try? handle.close()}
            var hash=SHA256()
            while let bytes=try handle.read(upToCount:65536),!bytes.isEmpty {try Task.checkCancellation();hash.update(data:bytes)}
            return hash.finalize().map {String(format:"%02x",$0)}.joined()==image.sha256
        }.value
    }
    func create(name:String,distribution:String,initMode:String,packages:String,archive:URL?,sha256:String?) async throws -> RootfsEnvironment {
        let name=name.trimmingCharacters(in:.whitespacesAndNewlines)
        guard !name.isEmpty,name.utf8.count<=255,!name.contains("/"),!name.utf8.contains(0),["alpine","debian","custom","android"].contains(distribution),["shell","systemd"].contains(initMode),["minimal","standard","development"].contains(packages),distribution != "alpine" || initMode=="shell" else {throw TramesError.message(L10n.text("环境配置无效"))}
        if distribution == "android", initMode != "shell" {throw TramesError.message("Android uses its own init")}
        let digest=sha256?.trimmingCharacters(in:.whitespacesAndNewlines).lowercased()
        if let digest,!digest.isEmpty, digest.count != 64 || !digest.allSatisfy({$0.isHexDigit && $0.isASCII}) {throw TramesError.message(L10n.text("SHA-256 应为 64 位十六进制字符"))}
        let id=UUID().uuidString.lowercased()
        let profile=RootfsEnvironment(id:id,name:name,store:"environments/\(id)",distribution:distribution,initMode:initMode,packages:packages,needsSetup:distribution=="debian" && (packages != "minimal" || initMode=="systemd"),sourceSHA256:digest?.isEmpty == false ? digest : nil,target:distribution == "android" ? "android" : "linux")
        _=try profile.storeURL(in:root)
        if ["custom","android"].contains(distribution) {
            guard let archive else {throw TramesError.message(L10n.text("请选择 ARM64 Linux rootfs 压缩包"))}
            try await ScopedFiles.stage(selected:archive,destination:source(profile))
        }
        do {
            let previous=try profiles()
            guard previous.count<64 else {throw TramesError.message(L10n.text("环境数量已达上限"))}
            try writeConfiguration(profile)
            try LocalSession.environmentConfig(["action":"grant","uuid":profile.id,"clipboard":true]).write(to:grants(profile),options:.atomic)
            try RootfsCatalog(version:1,environments:previous+[profile]).save(root)
        } catch {try? FileManager.default.removeItem(at:source(profile));throw error}
        return profile
    }
    private func grants(_ profile:RootfsEnvironment)->URL {authorizationRoot.appendingPathComponent("grants-\(profile.id).toml")}
    private func configuration(_ profile:RootfsEnvironment)->URL {root.appendingPathComponent("profiles",isDirectory:true).appendingPathComponent(profile.id+".toml")}
    private func writeConfiguration(_ profile:RootfsEnvironment) throws {
        let path=configuration(profile);try FileManager.default.createDirectory(at:path.deletingLastPathComponent(),withIntermediateDirectories:true)
        try LocalSession.environmentConfig(["action":"defaults","distribution":profile.distro,"name":profile.name,"uuid":profile.id,"init":profile.boot]).write(to:path,options:.atomic)
    }
    private func replace(_ profile:RootfsEnvironment) throws {
        try RootfsCatalog(version:1,environments:profiles().map {$0.id==profile.id ? profile : $0}).save(root)
    }
    func setBoot(_ profile:RootfsEnvironment,mode:String) throws -> RootfsEnvironment {
        guard profile.bootTarget == "linux",["shell","systemd"].contains(mode),profile.distro != "alpine" || mode=="shell" else {throw TramesError.message(L10n.text("此环境不支持 systemd"))}
        var next=profile;next.initMode=mode
        if next.distro=="debian",mode=="systemd" {next.needsSetup=true}
        let path=configuration(profile)
        if FileManager.default.fileExists(atPath:path.path){
            let text=try String(contentsOf:path,encoding:.utf8);var value=try JSONSerialization.jsonObject(with:LocalSession.environmentConfig(["action":"decode","toml":text])) as! [String:Any]
            value["boot"]=["mode":mode];try LocalSession.environmentConfig(["action":"encode","value":value]).write(to:path,options:.atomic)
        }else{try writeConfiguration(next)}
        try replace(next);return next
    }
    func pausePreparation(_ paused:Bool) {preparationPaused=paused;try? preparation?.pause(paused)}
    func start(profile:RootfsEnvironment? = nil,useVolumeBudget:Bool,vulkan:Bool=false,memoryBudgetMiB:Int=1024,processMemoryBudgetMiB:Int=512,maintenance:Bool=false,output:@escaping @Sendable(Data)->Void = {_ in}) async throws -> LocalSession {
        guard !resetting,!starting else {throw TramesError.message(L10n.text("环境仍在准备，请稍后再试"))}
        starting=true;defer {starting=false}
        let requested=profile?.id ?? "alpine"
        var profile=try profiles().first(where:{$0.id==requested}) ?? profile ?? RootfsCatalog.alpine.environments[0]
        let began=generation
        let musl=profile.distro=="alpine" ? try Self.muslLibrary() : nil
        let active=try profile.storeURL(in:root)
        let lock=ownerLock(profile)
        let values=try root.resourceValues(forKeys:[.volumeTotalCapacityKey,.volumeAvailableCapacityForImportantUsageKey])
        guard let total=values.volumeTotalCapacity,let available=values.volumeAvailableCapacityForImportantUsage,total>0,available>=0 else {throw TramesError.message("Cannot determine device storage capacity")}
        if !FileManager.default.fileExists(atPath:active.path) {
            let archive:URL
            if ["custom","android"].contains(profile.distro) {archive=source(profile)}
            else {
                output(Data(L10n.text("正在下载 %@…\r\n", String(describing: profile.name)).utf8))
                let (download,response)=try await ConnectivityRetry.download(from:profile.distro=="debian" ? Self.debianURL : Self.archiveURL)
                guard (response as? HTTPURLResponse)?.statusCode==200,(try download.resourceValues(forKeys:[.fileSizeKey])).fileSize ?? Int.max <= 64*1024*1024 else {try? FileManager.default.removeItem(at:download);throw TramesError.message(L10n.text("rootfs 下载无效"))}
                archive=download
            }
            defer {if !["custom","android"].contains(profile.distro) {try? FileManager.default.removeItem(at:archive)}}
            try Task.checkCancellation();guard generation==began,!resetting else {throw CancellationError()}
            let staging=root.appendingPathComponent("install-\(UUID().uuidString)",isDirectory:true)
            let distribution=profile.distro,sha=profile.sourceSHA256
            let images=distribution=="android" ? try await androidImages(archive:archive,output:output) : nil
            try Task.checkCancellation();guard generation==began,!resetting else {throw CancellationError()}
            output(Data(L10n.text("正在导入 %@…\r\n", String(describing: profile.name)).utf8))
            do {
                try await Task.detached(priority:.userInitiated) {try LocalSession.install(store:staging,rootfs:archive,total:UInt64(total),available:UInt64(available),muslLibc:musl,storeOwnerLock:lock,distribution:distribution,sha256:sha,androidImages:images)}.value
                try Task.checkCancellation();guard generation==began,!resetting else {throw CancellationError()}
                try FileManager.default.createDirectory(at:active.deletingLastPathComponent(),withIntermediateDirectories:true)
                try FileManager.default.moveItem(at:staging,to:active)
            } catch {try? FileManager.default.removeItem(at:staging);throw error}
        }
        if profile.needsSetup==true,!maintenance {
            output(Data(L10n.text("正在准备 Debian 软件，请保持应用前台；输出会实时显示。\r\n").utf8))
            let setup=try LocalSession(store:active,rootfs:nil,total:UInt64(total),available:UInt64(available),pureAccelerator:true,memoryBudgetMiB:memoryBudgetMiB,processMemoryBudgetMiB:processMemoryBudgetMiB,initiallyPaused:preparationPaused,storeOwnerLock:lock,distribution:"debian",debianPackages:profile.packages ?? "standard",prepareSystemd:profile.boot=="systemd")
            preparation=setup
            do {
                while true {
                    try Task.checkCancellation()
                    let bytes=try setup.receive();if !bytes.isEmpty {output(bytes)}
                    let status=try setup.status()
                    if status.phase=="exited" {
                        // Drain the final output before replacing the session.
                        let tail=try setup.receive();if !tail.isEmpty {output(tail)}
                        guard status.exit_code==0 else {throw TramesError.message(L10n.text("Debian 软件安装失败（%@）。可打开维护终端修复，或重新打开以重试。", String(describing: status.exit_code ?? 255)))}
                        break
                    }
                    if ["failed","cancelled"].contains(status.phase) {throw TramesError.message(status.error ?? L10n.text("Debian 软件准备已停止"))}
                    try await Task.sleep(for:.milliseconds(50))
                }
            } catch {setup.close();preparation=nil;throw error}
            setup.close();preparation=nil
            profile.needsSetup=false;try replace(profile)
        }
        try Task.checkCancellation();guard generation==began,!resetting else {throw CancellationError()}
        let icd=Bundle.main.url(forResource:"libtrames_vulkan_icd",withExtension:"so",subdirectory:"GuestVulkan")
        if vulkan,icd==nil {throw TramesError.message("Missing bundled Vulkan compute driver")}
        let retryUntil=Date().addingTimeInterval(10)
        while true {
            try Task.checkCancellation()
            let local=try LocalSession(store:active,rootfs:nil,total:UInt64(total),available:UInt64(available),network:"host",useVolumeBudget:useVolumeBudget,muslLibc:musl,pureAccelerator:true,vulkanICD:icd,memoryBudgetMiB:memoryBudgetMiB,processMemoryBudgetMiB:processMemoryBudgetMiB,multipleTerminals:profile.bootTarget == "linux",initiallyPaused:true,storeOwnerLock:lock,distribution:profile.distro,initMode:maintenance ? "shell" : profile.boot,target:profile.bootTarget,environmentConfig: !maintenance && FileManager.default.fileExists(atPath:configuration(profile).path) ? configuration(profile):nil,environmentGrants: !maintenance && FileManager.default.fileExists(atPath:grants(profile).path) ? grants(profile):nil)
            do {
                var status=try local.status()
                while status.phase=="starting" {try await Task.sleep(for:.milliseconds(20));status=try local.status()}
                if status.phase=="failed" {
                    if status.error?.contains("errno 16")==true,Date()<retryUntil {local.close();try await Task.sleep(for:.milliseconds(50));continue}
                    throw TramesError.message(status.error ?? L10n.text("Linux 环境启动失败"))
                }
                try Task.checkCancellation();return local
            } catch {local.close();throw error}
        }
    }
    func reset(profile:RootfsEnvironment? = nil) async throws {
        guard !resetting else {throw TramesError.message(L10n.text("Linux 环境正在重置"))}
        let profile=try profile ?? profiles().first(where:{$0.id=="alpine"}) ?? RootfsCatalog.alpine.environments[0]
        guard !["custom","android"].contains(profile.distro) || FileManager.default.fileExists(atPath:source(profile).path) else {throw TramesError.message(L10n.text("请重新导入此自定义 rootfs"))}
        resetting=true;generation &+= 1;defer {resetting=false}
        try await LocalSession.reset(store:profile.storeURL(in:root),storeOwnerLock:ownerLock(profile))
        if profile.distro=="debian" {var next=profile;next.needsSetup=true;try replace(next)}
    }
}
