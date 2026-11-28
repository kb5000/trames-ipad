// SPDX-License-Identifier: GPL-3.0-or-later
import UIKit
import SwiftTerm

/// Vend one accessory view to UIKit. The original upstream row owns its key
/// layout/actions; the surrounding view reserves the system resize corner and
/// replaces unclear upstream controls with explicit terminal controls.
final class TerminalKeyboardAccessory: UIInputView {
    let accessory: TerminalAccessory
    private weak var terminal: TerminalView?
    private let rowHeight: CGFloat
    private var notificationObservers: [NSObjectProtocol] = []
    private var boundsObserver: NSKeyValueObservation?
    private var layerBoundsObserver: NSKeyValueObservation?
    private var sublayersObserver: NSKeyValueObservation?
    private var bindingScheduled = false
    private weak var mouseButton: UIButton?
    private weak var keyboardButton: UIButton?
    var onMouseReportingChanged: ((Bool) -> Void)?
    static let trailingClearance: CGFloat = 48

    init(accessory: TerminalAccessory, terminal: TerminalView) {
        self.accessory = accessory
        self.terminal = terminal
        rowHeight = max(36, accessory.bounds.height)
        super.init(frame: CGRect(x: 0, y: 0, width: accessory.bounds.width, height: rowHeight), inputViewStyle: .keyboard)
        allowsSelfSizing = true
        accessory.allowsSelfSizing = false
        accessibilityIdentifier = "terminal-accessory"
        addSubview(accessory)

        // SwiftTerm rebuilds its buttons in every bounds setter (`setupUI()`),
        // removing old buttons and adding fresh ones. Observe UIView.bounds,
        // CALayer.bounds, CALayer.sublayers, and keyboard presentation events
        // so custom icons/actions are never lost on initial launch or resize.
        boundsObserver = accessory.observe(\.bounds, options: [.new]) { [weak self] _, _ in
            MainActor.assumeIsolated { self?.scheduleKeyBinding() }
        }
        layerBoundsObserver = accessory.layer.observe(\.bounds, options: [.new]) { [weak self] _, _ in
            MainActor.assumeIsolated { self?.scheduleKeyBinding() }
        }
        sublayersObserver = accessory.layer.observe(\.sublayers, options: [.new]) { [weak self] _, _ in
            MainActor.assumeIsolated { self?.scheduleKeyBinding() }
        }

        // SwiftTerm casts inputAccessoryView to TerminalAccessory. A wrapper
        // instead bridges its original sticky Ctrl to the public modifier API.
        let center = NotificationCenter.default
        notificationObservers.append(
            center.addObserver(forName: .terminalViewControlModifierReset, object: terminal, queue: .main) { [weak self] _ in
                MainActor.assumeIsolated { self?.accessory.controlModifier = false }
            }
        )
        for name: Notification.Name in [
            UIResponder.keyboardWillShowNotification,
            UIResponder.keyboardDidShowNotification,
            UIResponder.keyboardDidChangeFrameNotification,
        ] {
            notificationObservers.append(
                center.addObserver(forName: name, object: nil, queue: .main) { [weak self] _ in
                    MainActor.assumeIsolated {
                        self?.bindKeys()
                        self?.scheduleKeyBinding()
                    }
                }
            )
        }

        registerForTraitChanges([UITraitUserInterfaceStyle.self]) { (view: TerminalKeyboardAccessory, _: UITraitCollection) in
            view.bindKeys()
            view.scheduleKeyBinding()
        }

        bindKeys()
        scheduleKeyBinding()
    }

    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }
    deinit {
        for observer in notificationObservers {
            NotificationCenter.default.removeObserver(observer)
        }
    }
    override var intrinsicContentSize: CGSize { CGSize(width: UIView.noIntrinsicMetric, height: rowHeight) }

    override func safeAreaInsetsDidChange() {
        super.safeAreaInsetsDidChange()
        setNeedsLayout()
        scheduleKeyBinding()
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        let rect = CGRect(x: safeAreaInsets.left, y: 0, width: max(0, bounds.width - safeAreaInsets.left - safeAreaInsets.right - Self.trailingClearance), height: rowHeight)
        if accessory.frame != rect || !accessory.subviews.contains(where: { $0 is UIButton }) {
            let control = accessory.controlModifier
            accessory.frame = rect
            accessory.setupUI()
            accessory.controlModifier = control
        }
        accessory.layoutIfNeeded()
        bindKeys()
        // UIKit can assign the nested input view's bounds after this parent's
        // layout. SwiftTerm recreates every key in that setter, so rebind the
        // final buttons once the native layout pass has completed.
        scheduleKeyBinding()
    }

    private func scheduleKeyBinding() {
        guard !bindingScheduled else { return }
        bindingScheduled = true
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            self.bindingScheduled = false
            self.bindKeys()
        }
    }

    private func bindKeys() {
        for case let button as UIButton in accessory.subviews {
            let actions = button.actions(forTarget: accessory, forControlEvent: .touchDown) ?? []
            if actions.contains("toggleInputKeyboard:") || button.accessibilityIdentifier == "terminal-extended-keyboard-toggle" {
                button.accessibilityIdentifier = "terminal-extended-keyboard-toggle"
                button.accessibilityLabel = L10n.text("扩展键盘")
                button.toolTip = L10n.text("切换扩展功能键面板")
                keyboardButton = button
                refreshKeyboardButton(button)
            }
            if actions.contains("toggleTouch:") || button.accessibilityIdentifier == "terminal-mouse-mode-toggle" {
                // Upstream SwiftTerm renders `hand.draw` with inverted selection
                // for `allowMouseReporting`. Replace it with a clear mouse icon,
                // positive selection state, tooltip, and status feedback.
                button.removeTarget(accessory, action: NSSelectorFromString("toggleTouch:"), for: .touchDown)
                button.removeTarget(self, action: #selector(toggleMouseReporting), for: .touchDown)
                button.addTarget(self, action: #selector(toggleMouseReporting), for: .touchDown)
                button.accessibilityIdentifier = "terminal-mouse-mode-toggle"
                button.accessibilityLabel = L10n.text("终端鼠标模式")
                button.toolTip = L10n.text("终端鼠标模式：开启后将点击与滚动发送给终端应用；关闭后用于滚屏与选字")
                mouseButton = button
                refreshMouseButton(button)
            }
            if actions.contains("ctrl:") {
                // Upstream ctrl: only toggles controlModifier. Forward that
                // same operation to the public fallback modifier as well.
                button.removeTarget(accessory, action: NSSelectorFromString("ctrl:"), for: .touchDown)
                button.removeTarget(self, action: #selector(toggleControl), for: .touchDown)
                button.addTarget(self, action: #selector(toggleControl), for: .touchDown)
                button.accessibilityIdentifier = "terminal-control-modifier"
            }
        }
    }

    func refreshKeyboardState() {
        bindKeys()
        if let keyboardButton { refreshKeyboardButton(keyboardButton) }
    }

    private func resolvedNormalTint(for button: UIButton) -> UIColor {
        (button.titleColor(for: .normal) ?? UIColor.label).resolvedColor(with: traitCollection)
    }

    private func refreshKeyboardButton(_ button: UIButton) {
        let expanded = terminal?.inputView != nil
        button.isSelected = false
        button.accessibilityValue = L10n.text(expanded ? "已展开" : "已收起")
        let tint = resolvedNormalTint(for: button)
        let icon = Self.keyboardArrowIcon(expanded: expanded, tint: tint)
        button.setImage(icon, for: .normal)
        button.setImage(icon, for: .selected)
        button.setImage(icon, for: .highlighted)
    }

    private static func keyboardArrowIcon(expanded: Bool, tint: UIColor) -> UIImage? {
        let size = CGSize(width: 20, height: 18)
        let renderer = UIGraphicsImageRenderer(size: size)
        let keyboardConfig = UIImage.SymbolConfiguration(pointSize: 11.5, weight: .regular)
        let arrowConfig = UIImage.SymbolConfiguration(pointSize: 9.5, weight: .semibold)
        guard let keyboard = UIImage(systemName: "keyboard", withConfiguration: keyboardConfig)?
                .withTintColor(tint, renderingMode: .alwaysOriginal),
              let arrow = UIImage(systemName: expanded ? "chevron.compact.down" : "chevron.compact.up", withConfiguration: arrowConfig)?
                .withTintColor(tint, renderingMode: .alwaysOriginal) else {
            return nil
        }
        return renderer.image { _ in
            let kbSize = keyboard.size
            let arSize = arrow.size
            let kbOrigin = CGPoint(x: (size.width - kbSize.width) / 2, y: 0.5)
            keyboard.draw(in: CGRect(origin: kbOrigin, size: kbSize))
            let arOrigin = CGPoint(x: (size.width - arSize.width) / 2, y: size.height - arSize.height - 0.5)
            arrow.draw(in: CGRect(origin: arOrigin, size: arSize))
        }.withRenderingMode(.alwaysOriginal)
    }

    private func refreshMouseButton(_ button: UIButton) {
        let enabled = terminal?.allowMouseReporting ?? true
        button.isSelected = enabled
        button.accessibilityValue = L10n.text(enabled ? "已开启" : "已关闭")
        let symbolName = enabled ? "computermouse.fill" : "computermouse"
        let tint = enabled ? UIColor.white : resolvedNormalTint(for: button)
        if let image = UIImage(systemName: symbolName, withConfiguration: UIImage.SymbolConfiguration(pointSize: 14.0)) {
            let rendered = image.withTintColor(tint, renderingMode: .alwaysOriginal)
            button.setImage(rendered, for: .normal)
            button.setImage(rendered, for: .selected)
            button.setImage(rendered, for: .highlighted)
        }
    }

    @objc private func toggleMouseReporting() {
        guard let terminal else { return }
        UIDevice.current.playInputClick()
        terminal.allowMouseReporting.toggle()
        if let mouseButton { refreshMouseButton(mouseButton) }
        onMouseReportingChanged?(terminal.allowMouseReporting)
    }

    @objc private func toggleControl() {
        accessory.controlModifier.toggle()
        terminal?.controlModifier = accessory.controlModifier
    }

    override func didMoveToWindow() {
        super.didMoveToWindow()
        if window != nil {
            bindKeys()
            scheduleKeyBinding()
        }
        if window == nil {
            // Preserve upstream autorepeat cancellation when the wrapped row
            // leaves UIKit's keyboard hierarchy during dismissal or teardown.
            for case let button as UIButton in accessory.subviews { button.sendActions(for: .touchCancel) }
        }
    }
}
