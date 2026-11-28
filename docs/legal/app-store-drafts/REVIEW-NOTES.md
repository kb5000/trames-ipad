# Review notes for the Trames App Store and TestFlight drafts

These files are local review drafts only. They do not adopt a license change,
authorize distribution, create a contributor agreement, or represent an App
Store submission. They should receive legal review and build-specific technical
review before use.

## Drafting approach

The source-side draft uses an affirmative additional permission under GPLv3
section 7. It is intentionally not a proprietary relicense and not a promise to
ignore every violation. The grant is limited to conflicts caused solely by
Apple's required production App Store or TestFlight terms and technical
controls, while retaining source, notice, build-information, modification, and
other GPL duties where they can still be performed.

The draft permission is available to downstream licensees that meet its
conditions. That choice is explicit rather than hidden in definitions. The
owner approved this downstream scope on 2026-11-26; the permission is not to be
limited to one named Apple Developer account.

## Owner decisions recorded

- **Approved 2026-11-26 — deferred personal completion:** leave the owner's
  legal name and other personal identity fields as explicit placeholders for
  later personal completion. This deferral is intentional and is not an open
  policy question to repeat. Do not infer a name, address, jurisdiction, or
  telephone number from repository, account, or contact information.
- **Approved 2026-11-26 — downstream distribution:** downstream versions may be
  redistributed through both production Apple App Store Services and
  TestFlight Services when they satisfy the permission and all other applicable
  licenses.
- **Approved 2026-11-26 — source hosting:** use the existing
  `project-trames/trames-ipad`, `project-trames/trames-runtime`, and
  `project-trames/trames-cli` GitHub repositories as the source entry points.
  Identify each distributed App Store or TestFlight build by its actual
  version/build, immutable iPad tag or commit, and paired Runtime and CLI
  submodule commits. This does not claim that a future build's release mapping
  or a separate source archive has already been produced or verified.
- **Approved 2026-11-26 — official initial-release pricing:** the official first
  App Store release is free, with no purchase price, in-app purchases, or
  subscriptions. This decision is limited to the initial release. It is not a
  perpetual-free promise or a decision about later official releases, and it
  does not restrict a downstream GPL distributor from charging for a permitted
  conveyance.
- **Approved 2026-11-26 — initial-release support:** the official initial free
  release offers support by email at `a123edcrfv@gmail.com` on a best-effort
  basis. No fixed response, resolution, or repair time is promised, and not
  every Linux program is guaranteed to be compatible. This policy does not
  exclude or limit any maintenance, support, warranty, remedy, or other
  responsibility that applicable law makes non-waivable.
- **Approved 2026-11-26 — AI-driven public collaboration:** Trames is a fully
  AI-driven project. Unless an exception is expressly stated for a channel or
  submission, public GitHub issues, pull requests, discussions, and similar
  public project submissions are normally processed and analyzed with external
  AI tools. No additional opt-in is required solely to analyze an
  already-public, nonpersonal submission under this disclosed default. Public
  submission does not itself assign copyright, grant commercial relicensing or
  model-training permission, or authorize unrelated reuse. The public default
  does not override applicable personal-data disclosure and permission rules.
- **Approved 2026-11-26 — private voluntary support material:** private support
  email, private security reports, nonpublic attachments, and other nonpublic
  material are not covered by the public default and are not automatically
  uploaded on that basis. A private log or problem description voluntarily
  provided for support may be analyzed externally only for relevant diagnosis
  after credentials, personal information, and other sensitive content have
  been screened out and removed. Redaction is not represented as irreversible
  anonymization. Applicable disclosure and permission requirements remain for
  any personal data shared with a third-party AI provider. This policy is not
  automatic app telemetry or log uploading, an in-app AI feature, public
  posting of private material, copyright assignment, commercial relicensing,
  model training, or authorization for unrelated use. It does not automatically
  extend to Apple-provided TestFlight crash logs, usage data, or feedback.
- **Confirmed 2026-11-26 — production privacy behavior:** the production app has
  no Developer-controlled usage analytics or usage-statistics integration and
  no integration that automatically uploads logs to the Developer. Diagnostics
  are provided only when a user voluntarily chooses to share them. This is not
  a blanket “Data Not Collected” declaration, does not deny guest-selected
  networking or package downloads, and does not describe Apple's separate
  TestFlight Beta Testing Data flow.
- **Approved 2026-11-26 — initial availability preference:** offer the official
  initial release in all countries and regions that Apple makes available for
  this app in App Store Connect. This records the intended reach, not a finding
  that legal, tax, sanctions, encryption, content, consumer-law, or Apple
  eligibility requirements have already been satisfied in every territory.
- **Approved 2026-11-26 — unofficial listing identity:** an unofficial listing
  may include `Trames` in its app name, but must add a distinct identifying part
  that clearly distinguishes the independent project or release from the
  official app. It must use a separate, nonofficial icon that is visually
  distinct from the official icon and that the distributor has the right to
  use, and it must not imply official affiliation. The literal word `Unofficial`
  is not required. This applies consistently to modified forks and re-signed or
  relisted copies. The allowed use of `Trames` in such a distinctive listing
  name does not authorize the official icon or misleading claims of official
  affiliation, and it does not determine what separate rights may exist under
  trademark or other applicable law.
- The listing identity rule is a marking/non-misrepresentation condition, not a
  ban on GPL forks or copies. A downstream listing may truthfully say “Based on
  Trames,” make truthful compatibility statements, share official download
  links, and retain `Trames`, `trames-ipad`, or other project terms in factual
  attribution, source history, and internal source, module, crate, package,
  protocol, or other code identifiers. Original copyright notices, license
  notices, source history, and required attribution must remain intact where
  applicable licenses require them.
- **Legal qualification — project identity, not a registration claim:** no
  trademark-registry search or jurisdiction-by-jurisdiction rights analysis was
  performed. These drafts therefore neither call `Trames` a registered
  trademark nor conclude that no trademark rights exist. Trademark protection
  may arise through registration and, in some jurisdictions, through use. GPLv3
  section 7(c) expressly permits reasonable requirements to mark modified
  versions and to prohibit misrepresentation of their origin. Section 7(e)
  preserves a refusal to grant rights under trademark law only to the extent
  such rights actually exist; it does not establish their existence. The
  section 7(c) language should not be treated as automatically validating every
  name or icon restriction on a re-signed or relisted unmodified copy. For such
  copies, the draft states clear-origin presentation as a condition of relying
  on the separate Apple-distribution permission, subject to final legal review.
- **Approved 2026-11-26 — TestFlight coverage:** the additional permission
  covers TestFlight beta distribution as well as production App Store
  distribution. TestFlight's beta-use restrictions and default confidentiality
  for nonpublic beta content are treated as channel-specific Required Apple
  Terms, not as optional restrictions that the distributor may broaden.
- TestFlight automatically shares crash logs and usage data with Apple and the
  Application Provider, without a tester opt-out. Optional tester feedback may
  contain screenshots, comments, and personally identifying information. The
  beta privacy disclosure and policy must address that TestFlight-specific
  flow; it does not establish that the production app independently collects
  the same analytics.

The iSH `LICENSE.IOS` text was not copied. iSH states a non-enforcement covenant
for violations arising solely from an Apple/GPL conflict. This draft instead
grants affirmative permission. An affirmative permission can state what
licensees may do, but it can apply only to material for which every grantor has
actual authority.

The consumer EULA is separate. It contains the ten minimum topics in Exhibit B
to Schedule 1 of the Apple Developer Program License Agreement, while expressly
preserving rights granted independently by GPL, LGPL, and other open-source
licenses. A custom EULA cannot waive Apple service terms, and neither Apple nor
the EULA can grant rights in third-party GPL/LGPL material.

## Remaining substantive owner policy decisions

None are currently recorded.

Personal identity values are intentionally deferred completion fields, not
policy questions. The tasks below require legal, Apple-account, or build
evidence and should not be repeatedly presented as new owner-policy choices
unless that evidence exposes a genuine alternative.

## Objective legal, account, and build completion tasks

1. Audit contribution history in `trames-ipad`, `trames-runtime`, and
   `trames-cli`. Complete the permission's Schedule by right holder and material.
   A new CLA cannot retroactively clear an earlier contribution without that
   contributor's valid agreement.
2. Obtain final legal review of how the section 7 permission operates if a
   recipient elects a GPL version later than version 3. Do not infer that a
   later version necessarily handles the permission identically.
3. Review the custom EULA and mandatory consumer law for every selected region.
   Do not add a governing jurisdiction, exclusive venue, or monetary cap without
   a valid legal basis, and do not infer one from the all-regions preference.
4. Personally complete the intentionally deferred legal name, postal address,
   jurisdiction, and telephone fields before adoption or submission.
5. For the actual archive, fill the App version/build, immutable `trames-ipad`
   tag or commit, and paired `trames-runtime` and `trames-cli` submodule commits.
   Verify those repositories and revisions contain the source, notices,
   patches, and build/install material required for that binary and its bundled
   components. Do not claim a separate complete source archive exists unless it
   has actually been produced and verified.
6. Confirm the contents of the archived App Store build. Generated local
   `GuestVideo` resources currently include FFmpeg libraries, but that does not
   prove the submitted archive will include them.
7. Obtain Apple guidance on the relationship among Guideline 2.5.2, Developer
   Program License Agreement section 3.3.1(B), and Guideline 4.7. If Apple treats
   guest software as software offered under 4.7, obtain prior permission before
   submitting any build that exposes Metal/Vulkan or other native platform
   technologies under 4.7.2.
8. Do not assume the existing environment-level host grant satisfies 4.7.3's
   consent requirement for each individual item of offered software or each
   instance. After Apple clarifies the applicable classification, implement and
   verify any consent flow it requires before submission.
9. Resolve the 4.7.1, 4.7.4, and 4.7.5 implications of package repositories,
    including indexing/universal links, content controls, reporting/blocking,
    and age restrictions, if Apple classifies the feature under 4.7.
10. Ask whether a guest browser displayed through Wayland engages Guideline
    2.5.6. Do not claim an alternative-browser-engine entitlement exists.
11. Complete a build-specific privacy data-flow audit, App Privacy answers,
    privacy policy, and required-reason API manifest. Select only Apple-approved
    reasons that accurately match the code. Verify the production archive has
    no Developer-controlled usage telemetry or automatic log uploader and that
    diagnostics are shared only through a voluntary user action. Separately
    describe TestFlight's automatic crash-log and usage-data sharing and
    optional tester feedback; do not assume production collection is identical.
    Before using an external AI tool, identify the actual provider and verify
    its terms, privacy and security practices, actual retention/deletion
    behavior, data location and use, and an arrangement consistent with the
    prohibition on model training. Publish or present the applicable public
    channel disclosure and document the private-material screening and
    redaction process. Under App Review Guideline 5.1.2(i), clearly disclose
    where personal data will be shared with third parties, including third-party
    AI, and obtain explicit permission before sharing it unless otherwise
    permitted by law. Do not turn that personal-data rule into a repeated
    individual opt-in solely for AI analysis of already-public, nonpersonal
    project submissions. A public-channel notice or public submission is not
    permission to share personal data from a separate private communication.
12. Complete App Store Connect's encryption/export determination for the SSH and
    cryptographic libraries in the exact build. Do not set an exemption key
    before that determination.
13. Verify that the declared `audio` background mode is used only for an allowed
    audio purpose. Remove it if it is being used to keep the general runtime
    alive. Verify microphone consent and the final purpose string.
14. Rehearse every proposed review step on the distribution-signed archive and
    supply a video for slow or difficult graphics/Android paths. Complete the
    TestFlight beta description, features-to-test text, feedback contact,
    tester configuration, Beta App Review material where required, and
    build-specific export compliance. An App Review appointment is
    informational; it cannot preapprove the app or a feature.

## Verified repository facts

The following were checked in the working tree on 2026-11-26. They describe the
current source tree, not an App Store archive:

- `LICENSE`, `runtime/LICENSE`, and `cli/LICENSE` contain the official GPLv3
  text, including section 7. All three first-party repositories identify their
  project code as GPL-3.0-or-later.
- `apps/ios/project.yml` currently sets bundle prefix `org.trames`, main bundle
  identifier `org.trames.Trames`, App Group `group.org.trames.Trames`, iOS 17.0
  deployment target, marketing version 1.5.4, and project build number 49.
- `apps/ios/Trames.entitlements` contains the App Group and
  `com.apple.developer.kernel.increased-memory-limit`. It contains no JIT or
  dynamic-code-signing entitlement.
- `apps/ios/TramesFileProvider.entitlements` contains the matching App Group and
  no increased-memory or JIT entitlement.
- `apps/ios/Trames-Info.plist` declares the `audio` background mode and a
  microphone purpose string. These require release-behavior review; their
  presence is not proof that the behavior is approved.
- Current source implements an AArch64 guest interpreter and also contains
  signed native Rust helpers and platform bridges. Guest Linux ELF code is not
  host-native Mach-O. The graphics path includes native MoltenVK/Metal bridging,
  so “everything is interpreted” would be false.
- Current UI and documentation cover Alpine, Debian, Android, and custom ARM64
  rootfs profiles; terminal, persistent storage, networking, Wayland/Xwayland,
  package installation, and Files integration are present. Individual features
  still need archive-specific rehearsal.
- Built-in Alpine/Debian rootfs acquisition uses fixed sources and recorded
  digests. Custom and Android setup uses user-selected Files archives; Android
  can additionally download planned image components with recorded size and
  SHA-256 values.
- `docs/security/dependencies.md`,
  `runtime/docs/security/dependencies.md`, and
  `cli/docs/security/dependencies.md` record the reviewed dependency sets;
  each repository has `THIRD_PARTY_NOTICES.txt`.
- The current guest-video record describes FFmpeg 7.1.1 as LGPL-2.1-or-later,
  dynamically used as guest ELF, without `--enable-gpl` or `--enable-nonfree`.
  FFmpeg's own compliance checklist still requires exact source/build material
  and appropriate LGPL handling.
- The glibc 2.41 source subset retains LGPL-2.1-or-later terms. The native build
  does not compile the original glibc C files into the host library. That fact
  does not erase LGPL duties for redistributed original or derivative material
  and does not make the whole Runtime LGPL.
- Source uses `UserDefaults`, `ProcessInfo.systemUptime`, and volume-capacity APIs.
  Those usages require privacy-manifest analysis. Swift dependencies include
  SSH and cryptographic code from Citadel, SwiftNIO SSH, Swift Crypto, and
  BoringSSL-derived components.
- The working tree was already dirty in the iPad repository and Runtime
  submodule. These drafts do not modify that work.

## Corrections and qualifications to the supplied context

- The broad statement that no `PrivacyInfo.xcprivacy` exists anywhere under
  `apps/ios` is no longer literally true. Generated dependency checkout/build
  artifacts and scanned release archives contain privacy manifests supplied by
  Swift Crypto and SwiftNIO. However, no project-owned privacy manifest is
  tracked for Trames app code in the checked repositories. Dependency manifests
  do not declare the app's own API use.
- “No provisioning/signing has been performed” is too broad. Project
  documentation records earlier Personal Team development provisioning,
  matching App Group signing, physical installation, and File Provider tests.
  There is still no evidence of an App Store distribution upload, App Review
  submission, approval, or App Store permission for the native bridges.
- The current `project.yml` build number is 49. Other development or installed
  builds may have used a different number, so the App Review draft deliberately
  leaves version/build as a placeholder until an archive is selected.
- The main information property list also declares background audio and
  microphone access, which are relevant to review even though they were not
  listed with the entitlement summary.
- Local generated `GuestVideo` files currently exist, including FFmpeg shared
  objects. Their existence does not establish what a future submitted archive
  contains, and older published packages without FFmpeg do not answer that
  release-specific question.

## Primary sources checked

- [App Review Guidelines](https://developer.apple.com/app-store/review/guidelines/),
  especially 2.5.2, 2.5.4, 2.5.6, 2.5.8, 4.7.1–4.7.5, and 5.1.2(i).
- [Apple Developer Program License Agreement](https://developer.apple.com/support/terms/apple-developer-program-license-agreement/),
  especially 3.3.1(B) and Exhibit B to Schedule 1.
- [App Store Connect: provide a custom license agreement](https://developer.apple.com/help/app-store-connect/manage-app-information/provide-a-custom-license-agreement).
- [Apple Standard EULA](https://www.apple.com/legal/internet-services/itunes/dev/stdeula/),
  including its open-source carve-out.
- [Apple TestFlight Terms of Service](https://www.apple.com/legal/internet-services/itunes/testflight/),
  including beta-use restrictions, confidentiality, and automatic Beta Testing
  Data sharing.
- [App Store Connect TestFlight overview](https://developer.apple.com/help/app-store-connect/test-a-beta-version/testflight-overview/),
  including test information, build duration, tester groups, Beta App Review,
  metrics, and feedback.
- [App Store Connect export-compliance overview](https://developer.apple.com/help/app-store-connect/manage-app-information/overview-of-export-compliance).
- [Apple required-reason API guidance](https://developer.apple.com/documentation/bundleresources/describing-use-of-required-reason-api).
- [Apple increased-memory-limit entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.developer.kernel.increased-memory-limit).
- [Apple App Review appointment guidance](https://developer.apple.com/help/app-review/before-submitting-for-review/app-review-appointment/),
  which says appointments are informational and cannot preapprove an app.
- [GNU GPL FAQ on incompatible libraries and exceptions](https://www.gnu.org/licenses/gpl-faq.html#GPLIncompatibleLibs),
  read together with the unmodified local GPL section 7 text.
- [GNU General Public License version 3](https://www.gnu.org/licenses/gpl-3.0.en.html),
  especially section 7(c) on modified-version marking and origin, and section
  7(e) on rights under trademark law.
- [WIPO overview of trademark protection](https://www.wipo.int/en/web/trademarks/protection),
  noting that protection generally follows registration and may also arise
  through use in some countries.
- [iSH `LICENSE.IOS`](https://github.com/ish-app/ish/blob/master/LICENSE.IOS).
- [FFmpeg License and Legal Considerations](https://ffmpeg.org/legal.html).

## Files in this review set

- `README.md`: release package index and pre-submission gate summary.
- `APP-STORE-DISTRIBUTION-EXCEPTION.draft.md`: proposed source-side GPLv3
  section 7 additional permission for production App Store and TestFlight
  distribution.
- `END-USER-LICENSE-AGREEMENT.draft.md`: proposed consumer EULA with separate
  production App Store and TestFlight beta provisions.
- `PRIVACY-POLICY.draft.md`: proposed public Privacy Policy covering production
  app behavior, external guest networking, TestFlight diagnostics, and public
  vs. private support channels.
- `APP-REVIEW-NOTES.draft.md`: proposed review disclosure and demonstration
  instructions, with unresolved submission questions stated openly.
- `APP-STORE-METADATA.draft.md`: English and Simplified Chinese App Store
  Connect listing copy and TestFlight beta test notes.
- `REVIEW-NOTES.md`: this internal decision and evidence record.
