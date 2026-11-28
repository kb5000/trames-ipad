// SPDX-License-Identifier: GPL-3.0-or-later
import Network
import SwiftUI
import TramesKit
import WebKit

struct WebBookmark: Codable, Identifiable, Equatable {
    var id = UUID()
    var title: String
    var url: String
}

@MainActor
final class WebBookmarkStore: ObservableObject {
    static let shared = WebBookmarkStore()
    private static let storageKey = "trames.webBookmarks"
    static let defaults: [WebBookmark] = [
        WebBookmark(title: "localhost:3000", url: "http://127.0.0.1:3000"),
        WebBookmark(title: "localhost:5173", url: "http://127.0.0.1:5173"),
        WebBookmark(title: "localhost:8000", url: "http://127.0.0.1:8000"),
        WebBookmark(title: "localhost:8080", url: "http://127.0.0.1:8080"),
        WebBookmark(title: "172.17.0.2:80", url: "http://172.17.0.2:80"),
    ]

    @Published var bookmarks: [WebBookmark] = []

    private init() {
        load()
    }

    private func load() {
        if let data = UserDefaults.standard.data(forKey: Self.storageKey),
           let decoded = try? JSONDecoder().decode([WebBookmark].self, from: data) {
            bookmarks = decoded
        } else {
            bookmarks = Self.defaults
        }
    }

    private func save() {
        if let data = try? JSONEncoder().encode(bookmarks) {
            UserDefaults.standard.set(data, forKey: Self.storageKey)
        }
    }

    func contains(url: String) -> Bool {
        let normalized = WebTabSession.normalizeURL(url)?.absoluteString ?? url
        return bookmarks.contains { (WebTabSession.normalizeURL($0.url)?.absoluteString ?? $0.url) == normalized }
    }

    func toggle(title: String, url: String) {
        let normalized = WebTabSession.normalizeURL(url)?.absoluteString ?? url
        if let idx = bookmarks.firstIndex(where: { (WebTabSession.normalizeURL($0.url)?.absoluteString ?? $0.url) == normalized }) {
            bookmarks.remove(at: idx)
        } else {
            let label = title.trimmingCharacters(in: .whitespacesAndNewlines)
            bookmarks.append(WebBookmark(title: label.isEmpty ? normalized : label, url: normalized))
        }
        save()
    }

    func upsert(id: UUID?, title: String, rawURL: String) -> Bool {
        guard let normalized = WebTabSession.normalizeURL(rawURL.trimmingCharacters(in: .whitespacesAndNewlines))?.absoluteString else {
            return false
        }
        let trimmedTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)
        let displayTitle = trimmedTitle.isEmpty ? normalized.replacingOccurrences(of: "http://", with: "").replacingOccurrences(of: "https://", with: "") : trimmedTitle
        if let id, let idx = bookmarks.firstIndex(where: { $0.id == id }) {
            bookmarks[idx].title = displayTitle
            bookmarks[idx].url = normalized
        } else {
            bookmarks.append(WebBookmark(title: displayTitle, url: normalized))
        }
        save()
        return true
    }

    func remove(id: UUID) {
        bookmarks.removeAll { $0.id == id }
        save()
    }

    func resetDefaults() {
        bookmarks = Self.defaults
        save()
    }
}

@MainActor
final class WebTabSession: NSObject, ObservableObject, Identifiable, WKNavigationDelegate, WKUIDelegate {
    let id = UUID()
    @Published var title: String = L10n.text("内置浏览器")
    @Published var addressText: String = ""
    @Published var currentURL: URL?
    @Published var isLoading = false
    @Published var estimatedProgress: Double = 0
    @Published var canGoBack = false
    @Published var canGoForward = false
    @Published var desktopMode = true
    @Published var errorMessage: String?
    @Published var listeningPorts: [LocalListeningPort] = []
    @Published private(set) var proxyPort: UInt16?
    @Published private(set) var webViewGeneration = 0

    var onFetchListeningPorts: (() -> [LocalListeningPort])?
    private var _webView: WKWebView?
    private var observations: [NSKeyValueObservation] = []

    init(
        initialURL: String? = nil,
        proxyPort: UInt16? = nil,
        onFetchListeningPorts: (() -> [LocalListeningPort])? = nil
    ) {
        self.proxyPort = proxyPort
        self.onFetchListeningPorts = onFetchListeningPorts
        super.init()
        refreshListeningPorts()
        if let initialURL, !initialURL.isEmpty {
            addressText = initialURL
            load(address: initialURL)
        }
    }

    func refreshListeningPorts() {
        listeningPorts = onFetchListeningPorts?() ?? []
    }

    func showStartPage() {
        currentURL = nil
        addressText = ""
        errorMessage = nil
        title = L10n.text("内置浏览器")
        refreshListeningPorts()
    }

    var webView: WKWebView {
        if let existing = _webView { return existing }
        let created = makeWebView(proxyPort: proxyPort)
        _webView = created
        return created
    }

    func updateProxyPort(_ newPort: UInt16?) {
        guard proxyPort != newPort else { return }
        proxyPort = newPort
        let reloadURL = currentURL
        observations.removeAll()
        _webView?.stopLoading()
        _webView = nil
        webViewGeneration &+= 1
        if let reloadURL {
            _ = webView.load(URLRequest(url: reloadURL))
        }
    }

    func load(address raw: String) {
        let trimmed = raw.trimmingCharacters(in: .whitespacesAndNewlines)
        guard let url = Self.normalizeURL(trimmed) else {
            errorMessage = L10n.text("无效的网址或端口")
            return
        }
        errorMessage = nil
        addressText = url.absoluteString
        currentURL = url
        _ = webView.load(URLRequest(url: url))
    }

    func reloadOrStop() {
        if isLoading {
            webView.stopLoading()
        } else if let url = currentURL {
            errorMessage = nil
            if webView.url == nil {
                _ = webView.load(URLRequest(url: url))
            } else {
                webView.reload()
            }
        } else if !addressText.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            load(address: addressText)
        }
    }

    func goBack() {
        if webView.canGoBack { webView.goBack() }
    }

    func goForward() {
        if webView.canGoForward { webView.goForward() }
    }

    func toggleDesktopMode() {
        desktopMode.toggle()
        webView.configuration.defaultWebpagePreferences.preferredContentMode = desktopMode ? .desktop : .mobile
        if webView.url != nil { webView.reload() }
    }

    private func makeWebView(proxyPort: UInt16?) -> WKWebView {
        let config = WKWebViewConfiguration()
        config.allowsInlineMediaPlayback = true
        config.defaultWebpagePreferences.preferredContentMode = desktopMode ? .desktop : .mobile
        let store = WKWebsiteDataStore.nonPersistent()
        if let port = proxyPort, let nwPort = NWEndpoint.Port(rawValue: port) {
            let endpoint = NWEndpoint.hostPort(host: .ipv4(.loopback), port: nwPort)
            store.proxyConfigurations = [ProxyConfiguration(socksv5Proxy: endpoint)]
        }
        config.websiteDataStore = store
        let view = WKWebView(frame: .zero, configuration: config)
        view.navigationDelegate = self
        view.uiDelegate = self
        view.allowsBackForwardNavigationGestures = true
        observe(view)
        return view
    }

    private func observe(_ view: WKWebView) {
        observations = [
            view.observe(\.title, options: [.new]) { [weak self] view, _ in
                Task { @MainActor [weak self] in
                    let t = view.title?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
                    if !t.isEmpty { self?.title = t }
                    else if let host = view.url?.host { self?.title = host }
                }
            },
            view.observe(\.url, options: [.new]) { [weak self] view, _ in
                Task { @MainActor [weak self] in
                    guard let self, let url = view.url else { return }
                    self.currentURL = url
                    self.addressText = url.absoluteString
                    if self.title == L10n.text("内置浏览器"), let host = url.host {
                        self.title = host
                    }
                }
            },
            view.observe(\.isLoading, options: [.new]) { [weak self] view, _ in
                Task { @MainActor [weak self] in self?.isLoading = view.isLoading }
            },
            view.observe(\.estimatedProgress, options: [.new]) { [weak self] view, _ in
                Task { @MainActor [weak self] in self?.estimatedProgress = view.estimatedProgress }
            },
            view.observe(\.canGoBack, options: [.new]) { [weak self] view, _ in
                Task { @MainActor [weak self] in self?.canGoBack = view.canGoBack }
            },
            view.observe(\.canGoForward, options: [.new]) { [weak self] view, _ in
                Task { @MainActor [weak self] in self?.canGoForward = view.canGoForward }
            },
        ]
    }

    static func normalizeURL(_ raw: String) -> URL? {
        guard !raw.isEmpty else { return nil }
        if raw.allSatisfy(\.isNumber), let port = UInt16(raw), port > 0 {
            return URL(string: "http://127.0.0.1:\(port)")
        }
        if raw.hasPrefix(":"), raw.dropFirst().allSatisfy(\.isNumber), let port = UInt16(raw.dropFirst()), port > 0 {
            return URL(string: "http://127.0.0.1:\(port)")
        }
        let normalizedRaw: String
        if raw.contains("://") {
            normalizedRaw = raw
        } else {
            let hostPart = raw.split(separator: "/").first.map(String.init) ?? raw
            let hostOnly: String
            if hostPart.hasPrefix("["), let close = hostPart.firstIndex(of: "]") {
                hostOnly = String(hostPart[...close])
            } else {
                hostOnly = hostPart.split(separator: ":").first.map(String.init) ?? hostPart
            }
            let scheme = isLocalOrPrivateHost(hostOnly) ? "http" : "https"
            normalizedRaw = "\(scheme)://\(raw)"
        }
        guard var components = URLComponents(string: normalizedRaw) else {
            return URL(string: normalizedRaw)
        }
        if components.host == "::" || components.host == "[::]" {
            components.host = "::1"
        } else if components.host == "0.0.0.0" {
            components.host = "127.0.0.1"
        }
        return components.url ?? URL(string: normalizedRaw)
    }

    static func isLocalOrPrivateHost(_ host: String) -> Bool {
        let trimmed = host.trimmingCharacters(in: CharacterSet(charactersIn: "[]")).lowercased()
        if trimmed == "localhost" || trimmed == "127.0.0.1" || trimmed == "0.0.0.0"
            || trimmed == "::1" || trimmed == "::"
            || trimmed.hasPrefix("fc") || trimmed.hasPrefix("fd") || trimmed.hasPrefix("fe80:")
            || trimmed.hasSuffix(".local") {
            return true
        }
        let octets = trimmed.split(separator: ".").compactMap { UInt8($0) }
        if octets.count == 4 {
            if octets[0] == 10 || octets[0] == 127 { return true }
            if octets[0] == 192 && octets[1] == 168 { return true }
            if octets[0] == 172 && (16...31).contains(octets[1]) { return true }
            if octets[0] == 169 && octets[1] == 254 { return true }
        }
        return false
    }

    // MARK: - WKNavigationDelegate

    func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
        errorMessage = nil
    }

    func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
        let ns = error as NSError
        if ns.domain == NSURLErrorDomain && ns.code == NSURLErrorCancelled { return }
        errorMessage = error.localizedDescription
    }

    func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
        let ns = error as NSError
        if ns.domain == NSURLErrorDomain && ns.code == NSURLErrorCancelled { return }
        errorMessage = error.localizedDescription
    }

    func webView(
        _ webView: WKWebView,
        didReceive challenge: URLAuthenticationChallenge,
        completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void
    ) {
        if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
           Self.isLocalOrPrivateHost(challenge.protectionSpace.host),
           let trust = challenge.protectionSpace.serverTrust {
            completionHandler(.useCredential, URLCredential(trust: trust))
            return
        }
        completionHandler(.performDefaultHandling, nil)
    }

    // Handle target="_blank" inside the same tab instead of silently dropping it.
    func webView(
        _ webView: WKWebView,
        createWebViewWith configuration: WKWebViewConfiguration,
        for navigationAction: WKNavigationAction,
        windowFeatures: WKWindowFeatures
    ) -> WKWebView? {
        if navigationAction.targetFrame == nil, let url = navigationAction.request.url {
            webView.load(URLRequest(url: url))
        }
        return nil
    }
}

private struct WebViewHost: UIViewRepresentable {
    let webView: WKWebView
    func makeUIView(context: Context) -> WKWebView { webView }
    func updateUIView(_ uiView: WKWebView, context: Context) {}
}

struct WebBrowserPane: View {
    @ObservedObject var tab: WebTabSession
    @ObservedObject private var bookmarkStore = WebBookmarkStore.shared
    @FocusState private var addressFocused: Bool
    @State private var editingBookmarks = false
    @State private var showingBookmarkEditor = false
    @State private var editingBookmarkID: UUID?
    @State private var draftTitle = ""
    @State private var draftURL = ""
    @State private var draftError: String?

    private var currentIsBookmarked: Bool {
        guard let url = tab.currentURL?.absoluteString else { return false }
        return bookmarkStore.contains(url: url)
    }

    private func bookmarkPrimaryTitle(_ bookmark: WebBookmark) -> String {
        let trimmed = bookmark.title.trimmingCharacters(in: .whitespacesAndNewlines)
        if !trimmed.isEmpty && trimmed != bookmark.url {
            return trimmed
        }
        if let components = URLComponents(string: bookmark.url), let host = components.host {
            if let port = components.port {
                return "\(host):\(port)"
            }
            return host
        }
        return bookmark.url.replacingOccurrences(of: "http://", with: "").replacingOccurrences(of: "https://", with: "")
    }

    var body: some View {
        VStack(spacing: 0) {
            HStack(spacing: 6) {
                Button { tab.goBack() } label: {
                    Image(systemName: "chevron.left").frame(width: 34, height: 34)
                }
                .disabled(!tab.canGoBack)
                .buttonStyle(.plain)
                .accessibilityLabel(L10n.text("后退"))

                Button { tab.goForward() } label: {
                    Image(systemName: "chevron.right").frame(width: 34, height: 34)
                }
                .disabled(!tab.canGoForward)
                .buttonStyle(.plain)
                .accessibilityLabel(L10n.text("前进"))

                Button { tab.reloadOrStop() } label: {
                    Image(systemName: tab.isLoading ? "xmark" : "arrow.clockwise").frame(width: 34, height: 34)
                }
                .buttonStyle(.plain)
                .accessibilityLabel(tab.isLoading ? L10n.text("停止") : L10n.text("刷新"))

                Button { tab.showStartPage() } label: {
                    Image(systemName: "house").frame(width: 34, height: 34)
                }
                .buttonStyle(.plain)
                .accessibilityLabel(L10n.text("起始页"))

                HStack(spacing: 6) {
                    Image(systemName: tab.proxyPort != nil ? "network" : "globe")
                        .font(.caption)
                        .foregroundStyle(tab.proxyPort != nil ? .green : .secondary)
                    TextField(L10n.text("输入网址、端口号或容器 IP（如 3000 或 172.17.0.2:8080）"), text: $tab.addressText)
                        .textFieldStyle(.plain)
                        .font(.system(.callout, design: .monospaced))
                        .keyboardType(.URL)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                        .submitLabel(.go)
                        .focused($addressFocused)
                        .onSubmit { tab.load(address: tab.addressText) }
                        .accessibilityIdentifier("web-address-field")
                    if !tab.addressText.isEmpty {
                        Button { tab.addressText = "" } label: {
                            Image(systemName: "xmark.circle.fill").foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.horizontal, 10)
                .padding(.vertical, 6)
                .background(Color(uiColor: .tertiarySystemFill), in: RoundedRectangle(cornerRadius: 8))

                if let currentURL = tab.currentURL?.absoluteString {
                    Button {
                        bookmarkStore.toggle(title: tab.title, url: currentURL)
                    } label: {
                        Image(systemName: currentIsBookmarked ? "star.fill" : "star")
                            .foregroundStyle(currentIsBookmarked ? .yellow : .primary)
                            .frame(width: 34, height: 34)
                    }
                    .buttonStyle(.plain)
                    .accessibilityLabel(currentIsBookmarked ? L10n.text("取消收藏") : L10n.text("收藏当前网页"))
                }

                Button { tab.toggleDesktopMode() } label: {
                    Image(systemName: tab.desktopMode ? "desktopcomputer" : "ipad")
                        .frame(width: 34, height: 34)
                }
                .buttonStyle(.plain)
                .accessibilityLabel(tab.desktopMode ? L10n.text("桌面版网页") : L10n.text("移动版网页"))
            }
            .padding(.horizontal, 10)
            .padding(.vertical, 6)
            .background(Color(uiColor: .secondarySystemBackground))

            if tab.isLoading && tab.estimatedProgress < 1.0 {
                ProgressView(value: tab.estimatedProgress)
                    .progressViewStyle(.linear)
                    .tint(.accentColor)
            }

            if let errorMessage = tab.errorMessage {
                HStack(spacing: 8) {
                    Image(systemName: "exclamationmark.triangle.fill").foregroundStyle(.orange)
                    Text(errorMessage).font(.caption).lineLimit(2)
                    Spacer()
                    Button(L10n.text("重试")) { tab.reloadOrStop() }
                        .buttonStyle(.bordered)
                        .controlSize(.mini)
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(Color.orange.opacity(0.12))
            }

            if tab.currentURL == nil {
                ScrollView {
                    VStack(spacing: 20) {
                        Image(systemName: "globe")
                            .font(.system(size: 44))
                            .foregroundStyle(Color.accentColor)
                        Text(L10n.text("内置网页预览"))
                            .font(.title2.weight(.semibold))
                        Text(tab.proxyPort != nil
                             ? L10n.text("已接入当前 Linux 环境网络，可直接访问 localhost 端口与 Docker 容器 bridge 地址，无需端口映射或切换后台。")
                             : L10n.text("可预览本地开发服务器或网页；启动 Linux 环境后将自动接入容器网络。"))
                            .font(.callout)
                            .foregroundStyle(.secondary)
                            .multilineTextAlignment(.center)
                            .frame(maxWidth: 460)

                        VStack(alignment: .leading, spacing: 10) {
                            HStack {
                                Label(L10n.text("正在监听的端口"), systemImage: "dot.radiowaves.left.and.right")
                                    .font(.subheadline.weight(.semibold))
                                Spacer()
                                Button {
                                    tab.refreshListeningPorts()
                                } label: {
                                    Label(L10n.text("刷新"), systemImage: "arrow.clockwise")
                                }
                                .font(.caption)
                                .buttonStyle(.bordered)
                                .controlSize(.small)
                            }
                            if tab.listeningPorts.isEmpty {
                                Text(tab.proxyPort != nil
                                     ? L10n.text("当前未检测到监听中的 TCP 端口。在终端启动服务后点击刷新即可一键打开。")
                                     : L10n.text("启动 Linux 环境后，会自动检测并列出监听中的本地端口。"))
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                                    .frame(maxWidth: .infinity, alignment: .leading)
                                    .padding(12)
                                    .background(Color(uiColor: .secondarySystemBackground), in: RoundedRectangle(cornerRadius: 10))
                            } else {
                                LazyVGrid(columns: [GridItem(.adaptive(minimum: 240), spacing: 10)], spacing: 10) {
                                    ForEach(tab.listeningPorts) { endpoint in
                                        Button {
                                            tab.load(address: endpoint.url)
                                        } label: {
                                            HStack(spacing: 10) {
                                                Circle().fill(.green).frame(width: 8, height: 8)
                                                VStack(alignment: .leading, spacing: 2) {
                                                    Text(verbatim: "\(endpoint.address):\(endpoint.port)")
                                                        .font(.subheadline.weight(.medium))
                                                        .lineLimit(1)
                                                        .minimumScaleFactor(0.85)
                                                    Text(verbatim: endpoint.url)
                                                        .font(.system(.caption, design: .monospaced))
                                                        .foregroundStyle(.secondary)
                                                        .lineLimit(1)
                                                        .minimumScaleFactor(0.8)
                                                }
                                                Spacer(minLength: 0)
                                                Image(systemName: "arrow.up.right")
                                                    .font(.caption2)
                                                    .foregroundStyle(.secondary)
                                            }
                                            .frame(maxWidth: .infinity, minHeight: 38, alignment: .leading)
                                            .padding(.vertical, 10)
                                            .padding(.horizontal, 12)
                                            .background(Color(uiColor: .secondarySystemBackground), in: RoundedRectangle(cornerRadius: 10))
                                        }
                                        .buttonStyle(.plain)
                                    }
                                }
                            }
                        }
                        .frame(maxWidth: 640)

                        VStack(alignment: .leading, spacing: 10) {
                            HStack {
                                Text(L10n.text("常用快捷地址"))
                                    .font(.subheadline.weight(.semibold))
                                Spacer()
                                Button {
                                    editingBookmarkID = nil
                                    draftTitle = ""
                                    draftURL = ""
                                    draftError = nil
                                    showingBookmarkEditor = true
                                } label: {
                                    Label(L10n.text("添加"), systemImage: "plus")
                                }
                                .font(.caption)
                                .buttonStyle(.bordered)
                                .controlSize(.small)

                                Button(editingBookmarks ? L10n.text("完成") : L10n.text("编辑")) {
                                    editingBookmarks.toggle()
                                }
                                .font(.caption)
                                .buttonStyle(.bordered)
                                .controlSize(.small)
                            }

                            if bookmarkStore.bookmarks.isEmpty {
                                HStack {
                                    Text(L10n.text("暂无收藏地址，点右上角添加或在浏览网页时点击星标收藏。"))
                                        .font(.caption)
                                        .foregroundStyle(.secondary)
                                    Spacer()
                                    Button(L10n.text("恢复默认")) {
                                        bookmarkStore.resetDefaults()
                                    }
                                    .font(.caption)
                                    .buttonStyle(.bordered)
                                    .controlSize(.small)
                                }
                                .padding(12)
                                .background(Color(uiColor: .secondarySystemBackground), in: RoundedRectangle(cornerRadius: 10))
                            } else {
                                LazyVGrid(columns: [GridItem(.adaptive(minimum: 240), spacing: 10)], spacing: 10) {
                                    ForEach(bookmarkStore.bookmarks) { bookmark in
                                        Button {
                                            if editingBookmarks {
                                                editingBookmarkID = bookmark.id
                                                draftTitle = bookmark.title
                                                draftURL = bookmark.url
                                                draftError = nil
                                                showingBookmarkEditor = true
                                            } else {
                                                tab.load(address: bookmark.url)
                                            }
                                        } label: {
                                            HStack(spacing: 10) {
                                                VStack(alignment: .leading, spacing: 2) {
                                                    Text(verbatim: bookmarkPrimaryTitle(bookmark))
                                                        .font(.subheadline.weight(.medium))
                                                        .lineLimit(1)
                                                        .minimumScaleFactor(0.85)
                                                    Text(verbatim: bookmark.url)
                                                        .font(.system(.caption, design: .monospaced))
                                                        .foregroundStyle(.secondary)
                                                        .lineLimit(1)
                                                        .minimumScaleFactor(0.8)
                                                }
                                                Spacer(minLength: 0)
                                                if editingBookmarks {
                                                    Image(systemName: "pencil")
                                                        .font(.caption)
                                                        .foregroundStyle(Color.accentColor)
                                                    Button {
                                                        bookmarkStore.remove(id: bookmark.id)
                                                    } label: {
                                                        Image(systemName: "minus.circle.fill")
                                                            .foregroundStyle(.red)
                                                    }
                                                    .buttonStyle(.plain)
                                                } else {
                                                    Image(systemName: "arrow.up.right")
                                                        .font(.caption2)
                                                        .foregroundStyle(.secondary)
                                                }
                                            }
                                            .frame(maxWidth: .infinity, minHeight: 38, alignment: .leading)
                                            .padding(.vertical, 10)
                                            .padding(.horizontal, 12)
                                            .background(Color(uiColor: .secondarySystemBackground), in: RoundedRectangle(cornerRadius: 10))
                                        }
                                        .buttonStyle(.plain)
                                        .contextMenu {
                                            Button(L10n.text("编辑"), systemImage: "pencil") {
                                                editingBookmarkID = bookmark.id
                                                draftTitle = bookmark.title
                                                draftURL = bookmark.url
                                                draftError = nil
                                                showingBookmarkEditor = true
                                            }
                                            Button(L10n.text("删除"), systemImage: "trash", role: .destructive) {
                                                bookmarkStore.remove(id: bookmark.id)
                                            }
                                        }
                                    }
                                }
                                if editingBookmarks {
                                    HStack {
                                        Spacer()
                                        Button(L10n.text("恢复默认")) {
                                            bookmarkStore.resetDefaults()
                                        }
                                        .font(.caption)
                                        .buttonStyle(.borderless)
                                    }
                                }
                            }
                        }
                        .frame(maxWidth: 640)
                    }
                    .padding(32)
                    .frame(maxWidth: .infinity)
                }
                .onAppear { tab.refreshListeningPorts() }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .background(Color(uiColor: .systemBackground))
            } else {
                WebViewHost(webView: tab.webView)
                    .id(tab.webViewGeneration)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .sheet(isPresented: $showingBookmarkEditor) {
            NavigationStack {
                Form {
                    Section {
                        TextField(L10n.text("名称（可选）"), text: $draftTitle)
                        TextField(L10n.text("网址、端口或容器 IP（如 3000）"), text: $draftURL)
                            .keyboardType(.URL)
                            .textInputAutocapitalization(.never)
                            .autocorrectionDisabled()
                    }
                    if let draftError {
                        Section {
                            Text(draftError).foregroundStyle(.red)
                        }
                    }
                }
                .navigationTitle(editingBookmarkID == nil ? L10n.text("添加快捷地址") : L10n.text("编辑快捷地址"))
                .navigationBarTitleDisplayMode(.inline)
                .toolbar {
                    ToolbarItem(placement: .cancellationAction) {
                        Button(L10n.text("取消")) { showingBookmarkEditor = false }
                    }
                    ToolbarItem(placement: .confirmationAction) {
                        Button(L10n.text("保存")) {
                            if bookmarkStore.upsert(id: editingBookmarkID, title: draftTitle, rawURL: draftURL) {
                                showingBookmarkEditor = false
                            } else {
                                draftError = L10n.text("无效的网址或端口")
                            }
                        }
                        .disabled(draftURL.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)
                    }
                }
            }
            .presentationDetents([.medium])
        }
    }
}
