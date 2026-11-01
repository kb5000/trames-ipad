// SPDX-License-Identifier: GPL-3.0-or-later
import SwiftUI
import UIKit

/// Let UIKit track the keyboard and accessory, including hardware-keyboard
/// changes that can precede this controller's notification registration.
/// SwiftUI keyboard avoidance stays disabled to avoid applying the inset twice.
struct WorkspaceKeyboardContainer:UIViewControllerRepresentable {
    let workspace:WorkspaceModel
    func makeUIViewController(context:Context)->WorkspaceKeyboardController { WorkspaceKeyboardController(workspace:workspace) }
    func updateUIViewController(_ controller:WorkspaceKeyboardController,context:Context) {}
    func sizeThatFits(_ proposal:ProposedViewSize,uiViewController:WorkspaceKeyboardController,context:Context)->CGSize? {
        guard let width=proposal.width,let height=proposal.height else {return nil}
        return CGSize(width:max(0,width),height:max(0,height))
    }
}
final class WorkspaceKeyboardController:UIViewController {
    private let content:UIHostingController<WorkspaceView>
    init(workspace:WorkspaceModel) {
        content=UIHostingController(rootView:WorkspaceView(workspace:workspace))
        super.init(nibName:nil,bundle:nil)
        content.safeAreaRegions = .container
    }
    required init?(coder:NSCoder) { fatalError("init(coder:) has not been implemented") }
    override func loadView() {
        view=UIView();view.backgroundColor = .black
        addChild(content)
        let child=content.view!
        child.translatesAutoresizingMaskIntoConstraints=false
        child.clipsToBounds=true
        view.addSubview(child)
        let keyboard=view.keyboardLayoutGuide
        keyboard.followsUndockedKeyboard=false
        keyboard.usesBottomSafeArea=true
        NSLayoutConstraint.activate([
            child.topAnchor.constraint(equalTo:view.safeAreaLayoutGuide.topAnchor),
            child.leadingAnchor.constraint(equalTo:view.safeAreaLayoutGuide.leadingAnchor),
            child.trailingAnchor.constraint(equalTo:view.safeAreaLayoutGuide.trailingAnchor),
            child.bottomAnchor.constraint(equalTo:keyboard.topAnchor)
        ])
        content.didMove(toParent:self)
    }
}
