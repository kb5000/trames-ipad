# Trames Privacy Policy (Draft)

> **Status**: Review draft for the public Privacy Policy URL required by App Store Connect. Complete all `[PLACEHOLDER]` fields and verify exact production and AI-provider details before publishing.

**Effective Date**: `[PLACEHOLDER: EFFECTIVE DATE]`
**Developer**: `[PLACEHOLDER: OWNER FULL LEGAL NAME]`
**Contact**: `a123edcrfv@gmail.com`

---

## 1. Summary

Trames is a local-first Linux workspace for iPadOS. Your Linux environments, files, terminal sessions, and SSH configurations stay on your device.

- **No Developer Analytics or Ads**: The production App Store version of Trames contains no third-party analytics SDKs, advertising frameworks, cross-app tracking, or automatic background log uploaders.
- **No User Account**: Trames does not require registration or sign-in and does not operate a user database.
- **Separate Channels**: Data flows differ between (a) the production App Store app, (b) Apple TestFlight beta builds, (c) third-party network services you connect to from inside the app, and (d) public vs. private support channels. Each is described below.

---

## 2. Production App Store Build

### 2.1 Data Stored Locally on Your Device
All application data is stored locally within the iPadOS app sandbox and its dedicated App Group container (for the File Provider extension):
- Linux root filesystems, installed packages, and guest files
- Environment profiles (TOML configurations) and app preferences (`UserDefaults`)
- Scoped folder bookmarks you explicitly grant through the iPadOS Files picker

The Developer cannot access this local data.

### 2.2 Device Permissions
Trames requests iPadOS permissions strictly on demand for user-initiated features:
- **Local Network**: Used only when you connect to local network services via SSH or from inside a Linux environment.
- **Microphone**: Used only if you explicitly enable audio capture for a Linux environment.
- **Files and Folders**: Used only when you explicitly import/export files, grant a scoped folder bookmark, or browse your environments through the iPadOS Files app.

Granting these permissions allows the local app to function on your device; it does not send data to the Developer.

### 2.3 Diagnostics and Crash Reports
The production app does not automatically upload logs or crash reports to the Developer.
- If you enable Apple's system-wide **"Share With App Developers"** setting in iPadOS, Apple may provide anonymized crash and energy reports through App Store Connect.
- Any other diagnostic log is shared only if you manually export and send it to us.

---

## 3. User-Initiated Network Connections

When you download a Linux distribution image, run package managers (such as `apk` or `apt`), connect via SSH/SFTP, or use network tools inside a Linux environment, your device communicates directly with those external servers (for example, Alpine or Debian mirrors, GitHub, or your own SSH hosts).

Those external servers are not operated by the Developer. Their handling of IP addresses, requests, and metadata is governed by their respective privacy policies.

---

## 4. TestFlight Beta Builds

If you install a pre-release build of Trames through **Apple TestFlight**, Apple automatically collects crash logs and beta usage information and shares them with Apple and the Developer under Apple's TestFlight Terms of Service. If you submit optional TestFlight feedback, it may include screenshots, comments, and any information you choose to include.

This TestFlight diagnostic flow is built into Apple's beta testing platform and is separate from the production App Store release.

---

## 5. Community Channels, Support, and AI-Assisted Workflows

Trames is an open-source, AI-driven project. How your communications are handled depends on whether you post publicly or contact us privately:

### 5.1 Public Submissions (GitHub Issues, Pull Requests, Discussions)
- Information you post in public repositories or discussions is public.
- Unless stated otherwise for a specific channel, public submissions are normally analyzed and processed using external AI tools (`[PLACEHOLDER: VERIFIED AI PROVIDER DETAILS IF REQUIRED]`) to triage bugs, review patches, and maintain documentation.
- **Please do not post passwords, SSH keys, personal data, or sensitive logs in public issues or pull requests.**
- Public submission does not grant model-training rights, transfer copyright, or override applicable personal-data protection laws.

### 5.2 Private Support and Security Reports
- Emails sent to `a123edcrfv@gmail.com` and private security disclosures are **not** covered by the public-channel default and are never posted publicly without your permission.
- We use your private communication solely to respond to your inquiry, diagnose the reported issue, or coordinate a security fix.
- If you voluntarily share private logs for troubleshooting and external diagnostic assistance is needed, credentials, tokens, and personal identifying information are screened out and removed first (while noting that manual/automated redaction is not represented as irreversible anonymization). Where applicable law or platform rules (including Apple Guideline 5.1.2(i)) require explicit permission before sharing personal data with a third party, we will obtain your consent first.

---

## 6. Data Retention and Deletion

- **On-Device Data**: Delete any individual environment inside Trames (**More → Delete Environment…**) or uninstall Trames to permanently remove all local data and sandboxed files from your iPad.
- **Support Emails**: Retained only as long as needed to resolve your support request or meet legal obligations. You may request deletion of your support correspondence at any time by emailing `a123edcrfv@gmail.com`.

---

## 7. Children's Privacy

Trames is a developer and general-purpose computing tool not directed at children under 13 (or the applicable minimum age in your jurisdiction), and the Developer does not knowingly collect personal information from children.

---

## 8. Changes to This Policy

We may update this Privacy Policy as Trames evolves (for example, when features change or legal requirements are updated). Material changes will be reflected by updating the Effective Date at the top of this page and in the project repository.

---

## 9. Contact

For privacy questions, data-deletion requests, or support:

- **Developer**: `[PLACEHOLDER: OWNER FULL LEGAL NAME]`
- **Email**: `a123edcrfv@gmail.com`
- **Address**: `[PLACEHOLDER: FULL POSTAL ADDRESS]`
