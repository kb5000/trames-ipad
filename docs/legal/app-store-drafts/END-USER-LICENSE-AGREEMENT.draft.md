# Trames End-User License Agreement (EULA — Draft)

> **Pre-Submission Instructions (Remove Before Pasting into App Store Connect)**
> 1. Complete every `[PLACEHOLDER]` field below after personal, legal, and build verification.
> 2. Confirm the Privacy Policy URL is live and reflects both production and TestFlight data flows.
> 3. Convert only the **Proposed Final Text** section below to plain text for App Store Connect's custom EULA field.
> 4. Do not add a governing-law, exclusive-venue, or monetary-cap clause without region-specific legal review.

---

## Required Completion Fields (Not Part of the EULA)

- **Effective Date**: `[PLACEHOLDER: EFFECTIVE DATE]`
- **Developer Legal Name**: `[PLACEHOLDER: OWNER FULL LEGAL NAME]`
- **Postal Address**: `[PLACEHOLDER: FULL POSTAL ADDRESS]`
- **Telephone**: `[PLACEHOLDER: PHONE WITH COUNTRY CODE]`
- **Contact Email**: `a123edcrfv@gmail.com`
- **Privacy Policy URL**: `[PRIVACY_POLICY_URL]`
- **Release Mapping**:
  - App Version & Build: `[APP_VERSION_AND_BUILD]`
  - `trames-ipad`: `[TRAMES_IPAD_TAG_OR_COMMIT]`
  - `trames-runtime`: `[TRAMES_RUNTIME_COMMIT]`
  - `trames-cli`: `[TRAMES_CLI_COMMIT]`
- **Initial Official Release Model**: Free ($0.00), no IAP or subscriptions, all Apple-available regions.

---

## Proposed Final Text

**END-USER LICENSE AGREEMENT FOR TRAMES**

Effective Date: `[PLACEHOLDER: EFFECTIVE DATE]`

### 1. Acknowledgement
This End-User License Agreement ("Agreement") is concluded between you ("End-User" or "you") and `[PLACEHOLDER: OWNER FULL LEGAL NAME]` ("Developer"), and not with Apple Inc. ("Apple"). Subject to the independent open-source and third-party licenses described in Section 3, the Developer—not Apple—is solely responsible for the Trames application ("Application") and its content.

By downloading, installing, or using the Application, you agree to this Agreement. If you do not agree, do not install or use the Application. This Agreement does not conflict with the Apple Media Services Terms and Conditions, which also govern your use of Apple's distribution services.

### 2. Scope of App Store License
Subject to this Agreement and the open-source rights preserved in Section 3, the Developer grants you a non-transferable license to use the Application on any Apple-branded products that you own or control, as permitted by the Usage Rules set forth in the Apple Media Services Terms and Conditions, except that the Application may also be accessed and used by other accounts associated with you via Family Sharing or volume purchasing where enabled by Apple.

This Section applies only to the binary Application obtained through Apple's distribution channels and does not limit any rights granted directly to you under open-source licenses.

### 3. Open-Source Software and Source Availability
The Application incorporates substantial open-source software. Original Trames components are licensed under the GNU General Public License, version 3, or (at your option) any later version (GPL-3.0-or-later), supplemented by an Additional Permission under GPLv3 Section 7 for Apple App Store and TestFlight distribution. Bundled third-party libraries and guest software remain subject to their respective open-source licenses (including GPL, LGPL, Apache 2.0, MIT, BSD, and ISC).

- **Precedence of Open-Source Licenses**: Nothing in this Agreement restricts, overrides, or adds conditions to any right granted to you under an applicable open-source license. If any term of this Agreement conflicts with an applicable open-source license, the open-source license controls for that component.
- **Corresponding Source**: Corresponding source code for the exact distributed build (`[APP_VERSION_AND_BUILD]`) is made available at:
  - `https://github.com/project-trames/trames-ipad` (`[TRAMES_IPAD_TAG_OR_COMMIT]`)
  - `https://github.com/project-trames/trames-runtime` (`[TRAMES_RUNTIME_COMMIT]`)
  - `https://github.com/project-trames/trames-cli` (`[TRAMES_CLI_COMMIT]`)
- **Origin and Attribution**: Nothing in this Agreement authorizes you to misrepresent an unofficial build as the official Trames release or to use the official Trames app icon in a misleading manner. Factual attribution and compatibility statements are permitted.

### 4. Local Linux Environment and User Responsibility
The Application provides a local userspace Linux runtime and workspace on iPadOS that allows you to install and run ARM64 Linux distributions, packages, and command-line or graphical tools inside the app sandbox.

- **Your Workloads**: You are solely responsible for the Linux images, packages, scripts, credentials, and data you import or execute inside the Application, and for ensuring you have the right to use them in compliance with applicable law.
- **Operational Limits**: Userspace emulation is not a hardware virtualization boundary and does not guarantee compatibility with every Linux program. Guest workloads consume device battery, CPU, memory, and storage. You are responsible for backing up important files stored inside your environments.
- **Acceptable Use**: You must not use the Application to distribute malware, conduct unauthorized network attacks, bypass host platform security controls, or violate applicable law or third-party rights.

### 5. External Services and Networks
At your direction, the Application or guest software may connect to external package mirrors, SSH servers, container registries, or other third-party network services ("External Services"). External Services are operated independently of the Developer and Apple. Neither the Developer nor Apple controls or warrants External Services, and your use of them is governed by their own terms and privacy policies.

### 6. Privacy, Diagnostics, and Community Channels
- **Production App Behavior**: The production App Store build does not include Developer-controlled usage analytics, tracking SDKs, or automatic log uploaders. Your Linux environments, terminal history, and files remain on your device unless you choose to export, share, or transmit them.
- **System Permissions**: iPadOS permissions (such as Local Network, Microphone, or Files access) are requested on demand solely for user-initiated features and do not grant the Developer access to your data.
- **Public Community Submissions**: Trames is an AI-driven open-source project. Unless expressly stated otherwise, public GitHub issues, pull requests, and public discussions are normally processed and analyzed with external AI tools. Do not post passwords, private keys, or personal data in public channels. Public submission does not transfer copyright, authorize commercial relicensing, or grant model-training rights, and does not override applicable personal-data protection rules.
- **Private Support Communications**: Private support emails and private security reports are not covered by the public-channel default and are not published. If you voluntarily send private logs for troubleshooting, sensitive credentials and personal data are screened out before any relevant diagnostic analysis, subject to applicable privacy disclosure and consent rules.

For complete details, see the Privacy Policy at `[PRIVACY_POLICY_URL]`.

### 7. TestFlight Pre-Release Builds
If you install the Application via Apple TestFlight:
- **Pre-Release Status**: TestFlight builds are beta software provided for testing and may change or expire at any time.
- **Apple TestFlight Diagnostics**: Apple automatically collects crash logs and usage information during TestFlight sessions and shares them with Apple and the Developer as part of TestFlight's standard service; optional feedback you submit may include screenshots and comments. This TestFlight data flow is separate from the production App Store build.
- **Open-Source Confidentiality Carve-Out**: Where Apple's TestFlight terms impose confidentiality on non-public beta builds, the Developer does not designate GPL-licensed source code, copyright or license notices, or public project documentation as confidential.

### 8. Maintenance and Support
The Developer—not Apple—is solely responsible for providing any maintenance and support for the Application as stated in this Agreement or required by applicable law. Apple has no obligation whatsoever to furnish any maintenance or support services with respect to the Application.

For this initial free release, support is provided on a best-effort basis via email at `a123edcrfv@gmail.com`. No fixed response time, repair timeline, or universal Linux compatibility guarantee is promised, without limiting any non-waivable rights under applicable law.

### 9. Warranty
To the maximum extent permitted by applicable law and open-source licenses, the Application is provided "AS IS" and "AS AVAILABLE," without warranty of any kind.

The Developer is solely responsible for any product warranties, whether express or implied by law, to the extent not effectively disclaimed. In the event of any failure of the Application to conform to any applicable warranty, you may notify Apple, and Apple will refund the purchase price (if any) for the Application to you. To the maximum extent permitted by applicable law, Apple will have no other warranty obligation whatsoever with respect to the Application, and any other claims, losses, liabilities, damages, costs, or expenses attributable to any failure to conform to any warranty will be the Developer's sole responsibility.

### 10. Product Claims
You and the Developer acknowledge that the Developer, not Apple, is responsible for addressing any claims by you or any third party relating to the Application or your possession and/or use of the Application, including, but not limited to: (i) product liability claims; (ii) any claim that the Application fails to conform to any applicable legal or regulatory requirement; and (iii) claims arising under consumer protection, privacy, or similar legislation. This Agreement does not limit the Developer's liability to you beyond what is permitted by applicable law.

### 11. Intellectual Property Rights
In the event of any third-party claim that the Application or your possession and use of the Application infringes that third party's intellectual property rights, the Developer, not Apple, will be solely responsible for the investigation, defense, settlement, and discharge of any such intellectual property infringement claim.

### 12. Limitation of Liability
To the extent permitted by applicable law, neither the Developer nor any open-source contributor is liable for indirect, incidental, special, exemplary, or consequential damages (including loss of data or interruption of work) arising out of your use of or inability to use the Application. Nothing in this Agreement excludes or limits liability that cannot be excluded or limited under applicable mandatory law.

### 13. Legal Compliance
You represent and warrant that: (i) you are not located in a country that is subject to a U.S. Government embargo, or that has been designated by the U.S. Government as a "terrorist supporting" country; and (ii) you are not listed on any U.S. Government list of prohibited or restricted parties.

### 14. Developer Name and Contact Information
Questions, complaints, or claims regarding the Application should be directed to:

- **Developer**: `[PLACEHOLDER: OWNER FULL LEGAL NAME]`
- **Address**: `[PLACEHOLDER: FULL POSTAL ADDRESS]`
- **Telephone**: `[PLACEHOLDER: PHONE WITH COUNTRY CODE]`
- **Email**: `a123edcrfv@gmail.com`

### 15. Third-Party Terms of Agreement
You must comply with applicable third-party terms of agreement when using the Application (for example, your wireless data service agreement or the terms of third-party package repositories and remote servers you connect to).

### 16. Third-Party Beneficiary
You and the Developer acknowledge and agree that Apple, and Apple's subsidiaries, are third-party beneficiaries of this Agreement, and that, upon your acceptance of the terms and conditions of this Agreement, Apple will have the right (and will be deemed to have accepted the right) to enforce this Agreement against you as a third-party beneficiary thereof.

### 17. Termination and Severability
This Agreement remains effective until terminated. Your binary license under Section 2 terminates automatically if you fail to comply with any material term of this Agreement, in which case you must cease using and delete the binary Application. Termination of this Agreement does not affect any independent rights granted to you under the GNU GPL or other open-source licenses. If any provision of this Agreement is found unenforceable, the remaining provisions remain in effect.
