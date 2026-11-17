// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
#if canImport(FoundationNetworking)
import FoundationNetworking
#endif
/// Retry only repeatable bootstrap reads, never installation or native effects.
private final class DownloadObserver:NSObject,URLSessionDownloadDelegate,@unchecked Sendable {
    let progress:@Sendable(Int64,Int64)->Void
    // URLSession serializes delegate events. This per-transfer lock only joins
    // task creation/completion with cancellation from the async caller.
    private let lock=NSLock()
    private var continuation:CheckedContinuation<(URL,URLResponse),Error>?
    private var task:URLSessionDownloadTask?
    private var cancelled=false
    private var downloaded:URL?
    private var fileError:Error?
    private var last=Date.distantPast
    init(_ progress:@escaping @Sendable(Int64,Int64)->Void) {self.progress=progress}
    func run(from url:URL,configuration:URLSessionConfiguration) async throws -> (URL,URLResponse) {
        let session=URLSession(configuration:configuration,delegate:self,delegateQueue:nil)
        defer {session.invalidateAndCancel()}
        return try await withTaskCancellationHandler(operation:{
            try await withCheckedThrowingContinuation {continuation in
                lock.lock()
                if cancelled {lock.unlock();continuation.resume(throwing:CancellationError());return}
                self.continuation=continuation
                let task=session.downloadTask(with:url);self.task=task
                lock.unlock()
                task.resume()
            }
        },onCancel:{self.cancel()})
    }
    private func cancel() {
        lock.lock();cancelled=true;let current=task;lock.unlock()
        current?.cancel()
    }
    func urlSession(_ session:URLSession,downloadTask:URLSessionDownloadTask,didFinishDownloadingTo location:URL) {
        // The delegate's temporary file disappears on return. Move it once;
        // never buffer/copy the image into RAM or assume the async wrapper's
        // internal download delegate forwards progress events.
        let destination=FileManager.default.temporaryDirectory.appendingPathComponent("trames-download-"+UUID().uuidString)
        do {try FileManager.default.moveItem(at:location,to:destination);downloaded=destination}
        catch {fileError=error}
    }
    func urlSession(_ session:URLSession,downloadTask:URLSessionDownloadTask,didWriteData bytesWritten:Int64,totalBytesWritten:Int64,totalBytesExpectedToWrite:Int64) {
        let now=Date()
        if now.timeIntervalSince(last)>=0.5 || totalBytesWritten==totalBytesExpectedToWrite {
            last=now;progress(totalBytesWritten,totalBytesExpectedToWrite)
        }
    }
    func urlSession(_ session:URLSession,task:URLSessionTask,didCompleteWithError error:Error?) {
        lock.lock()
        let completion=continuation;continuation=nil;self.task=nil
        let wasCancelled=cancelled
        lock.unlock()
        if let failure=wasCancelled ? CancellationError() : (fileError ?? error) {
            if let downloaded {try? FileManager.default.removeItem(at:downloaded)}
            completion?.resume(throwing:failure)
        } else if let downloaded,let response=task.response {
            completion?.resume(returning:(downloaded,response))
        } else {
            if let downloaded {try? FileManager.default.removeItem(at:downloaded)}
            completion?.resume(throwing:URLError(.badServerResponse))
        }
    }
}
public enum ConnectivityRetry {
    public static func perform<T>(
        operation: @Sendable () async throws -> T,
        delay: @Sendable (UInt64) async throws -> Void = { try await Task.sleep(nanoseconds:$0) }
    ) async throws -> T {
        for attempt in 0..<3 {
            try Task.checkCancellation()
            do { return try await operation() }
            catch {
                try Task.checkCancellation()
                guard attempt < 2, let error=error as? URLError,
                      [.notConnectedToInternet,.networkConnectionLost,.cannotFindHost,
                       .cannotConnectToHost,.dnsLookupFailed,.timedOut,.dataNotAllowed].contains(error.code)
                else {throw error}
                try await delay(UInt64(attempt+1)*500_000_000)
            }
        }
        throw URLError(.notConnectedToInternet)
    }
    public static func download(from url: URL, progress:(@Sendable(Int64,Int64)->Void)? = nil) async throws -> (URL,URLResponse) {
        let configuration=URLSessionConfiguration.ephemeral
        #if canImport(Darwin)
        configuration.waitsForConnectivity=true
        #endif
        configuration.timeoutIntervalForRequest=60
        // This is the entire transfer's deadline, not an inactivity timeout.
        // Android images can approach 1 GB; active downloads must outlive a minute.
        configuration.timeoutIntervalForResource=60*60
        return try await perform {
            progress?(0,NSURLSessionTransferSizeUnknown)
            return try await DownloadObserver(progress ?? {_,_ in}).run(from:url,configuration:configuration)
        }
    }
}
