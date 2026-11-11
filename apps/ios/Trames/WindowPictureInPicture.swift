// SPDX-License-Identifier: GPL-3.0-or-later
import AVKit
import CoreMedia
import CoreVideo
import SwiftUI
import TramesKit

/// One user-selected live window. Idle applications allocate no capture buffers
/// or timer. Background frames are software-rendered, never Metal submissions.
@MainActor
final class WindowPictureInPicture: NSObject, ObservableObject, AVPictureInPictureControllerDelegate, AVPictureInPictureSampleBufferPlaybackDelegate {
    @Published private(set) var item: WorkspaceItemID?
    @Published private(set) var active = false
    @Published private(set) var starting = false
    private var paused = false
    var keepsRunning: Bool { active && !paused }
    var supported: Bool { AVPictureInPictureController.isPictureInPictureSupported() }
    var source: (() -> TerminalSession?)?
    var onActivityChanged: (() -> Void)?
    var onRestore: ((WorkspaceItemID) -> Void)?
    var onError: ((String) -> Void)?
    private weak var display: AVSampleBufferDisplayLayer?
    private var controller: AVPictureInPictureController?
    private var possible: NSKeyValueObservation?
    private var timer: Timer?
    private var startTimeout: Task<Void,Never>?
    private var pool: CVPixelBufferPool?
    private var dimensions = CGSize.zero
    private var format: CMVideoFormatDescription?
    private var lastGraphic: CGImage?
    private var lastGraphicWindow: WorkspaceItemID?
    #if DEBUG
    private var frameCount=0
    private func trace(_ message:String) {
        let line="\(Date()) \(message)\n"
        let url=FileManager.default.urls(for:.documentDirectory,in:.userDomainMask)[0].appendingPathComponent("pip-debug.txt")
        if let handle=try? FileHandle(forWritingTo:url) {defer {try? handle.close()};try? handle.seekToEnd();try? handle.write(contentsOf:Data(line.utf8))}
        else {try? Data(line.utf8).write(to:url)}
    }
    #endif
    private final class WeakCanvas {weak var value:WaylandCanvas?;init(_ value:WaylandCanvas){self.value=value}}
    private var canvases:[WorkspaceItemID:WeakCanvas]=[:]
    func register(_ canvas:WaylandCanvas,for item:WorkspaceItemID) {
        guard canvases[item]?.value !== canvas else {return}
        canvases=canvases.filter {$0.value.value != nil};canvases[item]=WeakCanvas(canvas)
    }

    func attach(_ layer: AVSampleBufferDisplayLayer) {
        guard display !== layer else { return }
        display=layer
        var timebase:CMTimebase?
        if CMTimebaseCreateWithSourceClock(allocator:nil,sourceClock:CMClockGetHostTimeClock(),timebaseOut:&timebase)==noErr,let timebase {
            CMTimebaseSetTime(timebase,time:CMClockGetTime(CMClockGetHostTimeClock()))
            CMTimebaseSetRate(timebase,rate:1);layer.controlTimebase=timebase
        }
        guard supported else { return }
        let controller=AVPictureInPictureController(contentSource:.init(sampleBufferDisplayLayer:layer,playbackDelegate:self))
        controller.delegate=self;controller.requiresLinearPlayback=true
        controller.canStartPictureInPictureAutomaticallyFromInline=false
        self.controller=controller
        possible=controller.observe(\.isPictureInPicturePossible,options:[.new]) { [weak self] _,_ in
            Task { @MainActor in self?.startWhenPossible() }
        }
    }
    func toggle(_ selected:WorkspaceItemID, source:@escaping ()->TerminalSession?) {
        if item == selected && (active || starting) { stop();return }
        if active || starting {
            if let item {canvases[item]?.value?.pictureCapture=nil}
            self.source=source;item=selected;paused=false
            lastGraphic=nil;lastGraphicWindow=nil;pool=nil;format=nil;dimensions = .zero
            canvases[selected]?.value?.pictureCapture={ [weak self] image in self?.acceptGraphic(image,for:selected) }
            tick();controller?.invalidatePlaybackState();onActivityChanged?();return
        }
        guard supported else { onError?(L10n.text("此设备不支持画中画"));return }
        self.source=source;item=selected;paused=false;starting=true
        #if DEBUG
        frameCount=0;trace("start layer=\(String(describing:display)) controller=\(String(describing:controller))")
        #endif
        canvases=canvases.filter {$0.value.value != nil}
        canvases[selected]?.value?.pictureCapture={ [weak self] image in self?.acceptGraphic(image,for:selected) }
        do {
            let audio=AVAudioSession.sharedInstance()
            if audio.category != .playAndRecord { try audio.setCategory(.playback,mode:.moviePlayback,options:.mixWithOthers) }
            try audio.setActive(true)
        } catch { failed(error);return }
        tick()
        timer=Timer(timeInterval:0.2,repeats:true) { [weak self] _ in Task { @MainActor in self?.tick() } }
        RunLoop.main.add(timer!,forMode:.common)
        startWhenPossible()
        startTimeout=Task { [weak self] in
            try? await Task.sleep(for:.seconds(8))
            guard !Task.isCancelled,let self,self.starting else {return}
            self.failed(NSError(domain:"Trames.PiP",code:1,userInfo:[NSLocalizedDescriptionKey:L10n.text("画中画未能启动，请保持窗口可见后重试")]))
        }
    }
    private func startWhenPossible() {
        guard starting,let controller,controller.isPictureInPicturePossible else {return}
        controller.startPictureInPicture()
    }
    func stop() {
        startTimeout?.cancel();startTimeout=nil
        starting=false
        if active { controller?.stopPictureInPicture() }
        else { finish() }
    }
    private func finish() {
        timer?.invalidate();timer=nil;active=false;starting=false;paused=false
        if let item {canvases[item]?.value?.pictureCapture=nil}
        item=nil;source=nil;pool=nil;format=nil;dimensions = .zero
        lastGraphic=nil;lastGraphicWindow=nil;display?.flushAndRemoveImage()
        onActivityChanged?()
    }
    private func failed(_ error:Error) { finish();onError?(error.localizedDescription) }
    func acceptGraphic(_ image:CGImage,for selected:WorkspaceItemID) {
        guard item == selected else {return}
        lastGraphic=image;lastGraphicWindow=selected
    }
    private func tick() {
        guard !paused,let item,let display else {return}
        guard let session=source?() else {stop();return}
        if display.status == .failed { display.flush() }
        #if DEBUG
        if frameCount<2 {trace("tick ready=\(display.isReadyForMoreMediaData) status=\(display.status.rawValue) possible=\(controller?.isPictureInPicturePossible ?? false) error=\(String(describing:display.error))")}
        #endif
        guard display.isReadyForMoreMediaData else {return}
        let size:CGSize
        switch item {
        case .terminal: size=session.terminal.bounds.size
        case .wayland(_,let id):
            guard let window=session.waylandWindows.first(where:{$0.id==id}) else {stop();return}
            size=CGSize(width:Int(window.width),height:Int(window.height))
        }
        guard size.width>0,size.height>0 else {return}
        let factor=min(1,960/size.width,720/size.height)
        let target=CGSize(width:max(2,Int(size.width*factor)/2*2),height:max(2,Int(size.height*factor)/2*2))
        if dimensions != target {
            dimensions=target;format=nil
            CVPixelBufferPoolCreate(nil,nil,[kCVPixelBufferIOSurfacePropertiesKey:[:],kCVPixelBufferPixelFormatTypeKey:kCVPixelFormatType_32BGRA,kCVPixelBufferWidthKey:Int(target.width),kCVPixelBufferHeightKey:Int(target.height),kCVPixelBufferCGImageCompatibilityKey:true,kCVPixelBufferCGBitmapContextCompatibilityKey:true] as CFDictionary,&pool)
        }
        guard let pool else {return}
        var pixel:CVPixelBuffer?
        guard CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(nil,pool,[kCVPixelBufferPoolAllocationThresholdKey:3] as CFDictionary,&pixel)==kCVReturnSuccess,let pixel else {return}
        CVPixelBufferLockBaseAddress(pixel,[])
        var locked=true
        defer {if locked {CVPixelBufferUnlockBaseAddress(pixel,[])}}
        guard let context=CGContext(data:CVPixelBufferGetBaseAddress(pixel),width:Int(target.width),height:Int(target.height),bitsPerComponent:8,bytesPerRow:CVPixelBufferGetBytesPerRow(pixel),space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGBitmapInfo.byteOrder32Little.rawValue|CGImageAlphaInfo.premultipliedFirst.rawValue) else {return}
        context.setFillColor(UIColor.black.cgColor);context.fill(CGRect(origin:.zero,size:target))
        context.scaleBy(x:target.width/size.width,y:target.height/size.height)
        context.translateBy(x:0,y:size.height);context.scaleBy(x:1,y:-1)
        switch item {
        case .terminal:
            // SwiftTerm is a scroll view: replay its original CoreText draw in
            // the visible bounds, rather than snapshotting a scrollback layer.
            let bounds=session.terminal.bounds
            context.translateBy(x:-bounds.minX,y:-bounds.minY)
            UIGraphicsPushContext(context);session.terminal.draw(bounds);UIGraphicsPopContext()
        case .wayland(_,let id):
            if let frame=try? session.local?.waylandFrame(window:id), !frame.images.contains(where:{$0.kind != 1}) {
                draw(frame:frame,in:context,height:size.height)
                frame.release(presented:true)
            } else if lastGraphicWindow == item,let image=lastGraphic {
                UIGraphicsPushContext(context);UIImage(cgImage:image).draw(in:CGRect(origin:.zero,size:size));UIGraphicsPopContext()
            }
        }
        #if DEBUG
        if frameCount==0,let image=context.makeImage(),let data=UIImage(cgImage:image).pngData() {
            try? data.write(to:FileManager.default.urls(for:.documentDirectory,in:.userDomainMask)[0].appendingPathComponent("pip-first-frame.png"))
        }
        #endif
        context.flush()
        CVPixelBufferUnlockBaseAddress(pixel,[]);locked=false
        if format == nil {CMVideoFormatDescriptionCreateForImageBuffer(allocator:nil,imageBuffer:pixel,formatDescriptionOut:&format)}
        guard let format else {return}
        var timing=CMSampleTimingInfo(duration:CMTime(value:1,timescale:5),presentationTimeStamp:CMClockGetTime(CMClockGetHostTimeClock()),decodeTimeStamp:.invalid)
        var sample:CMSampleBuffer?
        guard CMSampleBufferCreateReadyWithImageBuffer(allocator:nil,imageBuffer:pixel,formatDescription:format,sampleTiming:&timing,sampleBufferOut:&sample)==noErr,let sample else {return}
        if let attachments=CMSampleBufferGetSampleAttachmentsArray(sample,createIfNecessary:true) {
            let entry=unsafeBitCast(CFArrayGetValueAtIndex(attachments,0),to:CFMutableDictionary.self)
            CFDictionarySetValue(entry,Unmanaged.passUnretained(kCMSampleAttachmentKey_DisplayImmediately).toOpaque(),Unmanaged.passUnretained(kCFBooleanTrue).toOpaque())
        }
        display.enqueue(sample)
        #if DEBUG
        frameCount+=1
        if frameCount<=2 {trace("enqueued frame=\(frameCount) size=\(target) status=\(display.status.rawValue) possible=\(controller?.isPictureInPicturePossible ?? false)")}
        #endif
        controller?.invalidatePlaybackState()
    }
    private func draw(frame:LocalWaylandFrame,in context:CGContext,height:CGFloat) {
        // Same BGRA backing capability as normal presentation; the retained frame
        // outlives the synchronous CoreGraphics draw. No guest pointer escapes.
        let orientations:[UIImage.Orientation]=[.up,.right,.down,.left,.upMirrored,.leftMirrored,.downMirrored,.rightMirrored]
        UIGraphicsPushContext(context);defer {UIGraphicsPopContext()}
        for (image,shape) in zip(frame.images,frame.geometry) {
            guard image.kind == 1,image.offset < image.length,
                  let base=UnsafeRawPointer(bitPattern:UInt(image.address)),
                  let provider=CGDataProvider(dataInfo:nil,data:base.advanced(by:Int(image.offset)),size:Int(image.length-image.offset),releaseData:{_,_,_ in}),
                  let bitmap=CGImage(width:Int(image.width),height:Int(image.height),bitsPerComponent:8,bitsPerPixel:32,bytesPerRow:Int(image.stride),space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:[.byteOrder32Little,CGBitmapInfo(rawValue:image.alpha == 0 ? CGImageAlphaInfo.noneSkipFirst.rawValue : CGImageAlphaInfo.premultipliedFirst.rawValue)],provider:provider,decode:nil,shouldInterpolate:false,intent:.defaultIntent) else {continue}
            let w=CGFloat(shape.width),h=CGFloat(shape.height)
            let sx=CGFloat(shape.source.0),sy=CGFloat(shape.source.1),sw=CGFloat(shape.source.2),sh=CGFloat(shape.source.3)
            context.saveGState()
            context.clip(to:CGRect(x:CGFloat(image.x),y:CGFloat(image.y),width:w,height:h))
            UIImage(cgImage:bitmap,scale:1,orientation:orientations[Int(image.transform & 7)]).draw(in:CGRect(x:CGFloat(image.x)-sx*w/sw,y:CGFloat(image.y)-sy*h/sh,width:w/sw,height:h/sh))
            context.restoreGState()
        }
    }

    func pictureInPictureControllerDidStartPictureInPicture(_ controller:AVPictureInPictureController) {guard item != nil else {controller.stopPictureInPicture();return};active=true;starting=false;startTimeout?.cancel();startTimeout=nil;onActivityChanged?()}
    func pictureInPictureControllerDidStopPictureInPicture(_ controller:AVPictureInPictureController) {finish()}
    func pictureInPictureController(_ controller:AVPictureInPictureController,failedToStartPictureInPictureWithError error:Error) {failed(error)}
    func pictureInPictureController(_ controller:AVPictureInPictureController,restoreUserInterfaceForPictureInPictureStopWithCompletionHandler completionHandler:@escaping (Bool)->Void) {if let item {onRestore?(item)};completionHandler(true)}
    func pictureInPictureController(_ controller:AVPictureInPictureController,setPlaying playing:Bool) {paused = !playing;onActivityChanged?();if playing {tick()};controller.invalidatePlaybackState()}
    func pictureInPictureControllerIsPlaybackPaused(_ controller:AVPictureInPictureController)->Bool {paused}
    func pictureInPictureControllerTimeRangeForPlayback(_ controller:AVPictureInPictureController)->CMTimeRange {CMTimeRange(start:.zero,duration:.positiveInfinity)}
    func pictureInPictureController(_ controller:AVPictureInPictureController,didTransitionToRenderSize newRenderSize:CMVideoDimensions) {}
    func pictureInPictureController(_ controller:AVPictureInPictureController,skipByInterval skipInterval:CMTime,completion completionHandler:@escaping ()->Void) {completionHandler()}
}
private final class PictureSourceView:UIView {
    override class var layerClass:AnyClass {AVSampleBufferDisplayLayer.self}
}
struct PictureInPictureSource:UIViewRepresentable {
    let controller:WindowPictureInPicture
    func makeUIView(context:Context)->UIView {let view=PictureSourceView();controller.attach(view.layer as! AVSampleBufferDisplayLayer);return view}
    func updateUIView(_ view:UIView,context:Context) {}
}
