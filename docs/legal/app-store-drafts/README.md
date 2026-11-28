# Trames iPad — App Store & TestFlight Release Package (Drafts)

This directory holds the review drafts prepared for Trames iPad's initial App Store and TestFlight distribution.

> **Status**: Working drafts only. Nothing in this directory is legally adopted, submitted to Apple, or effective until the copyright holders complete all required fields, resolve the open verification items in [`REVIEW-NOTES.md`](REVIEW-NOTES.md), and formally adopt the texts for a specific release archive.

---

## Document Index

### Legal & Open-Source Licensing
| Document | Purpose | Audience |
| :--- | :--- | :--- |
| [`APP-STORE-DISTRIBUTION-EXCEPTION.draft.md`](APP-STORE-DISTRIBUTION-EXCEPTION.draft.md) | Proposed GPLv3 Section 7 additional permission reconciling GPL-3.0-or-later with Apple's mandatory App Store and TestFlight terms. | Copyright holders, downstream distributors, users |
| [`END-USER-LICENSE-AGREEMENT.draft.md`](END-USER-LICENSE-AGREEMENT.draft.md) | Custom App Store EULA satisfying Apple Exhibit B minimum terms while preserving open-source rights under the GPL and third-party licenses. | End users (paste plain text into App Store Connect) |
| [`PRIVACY-POLICY.draft.md`](PRIVACY-POLICY.draft.md) | Public privacy policy covering production app behavior, permissions, external guest networking, TestFlight diagnostics, and community/support channels. | End users, App Store Connect Privacy Policy URL |

### App Store Connect & Apple Review
| Document | Purpose | Audience |
| :--- | :--- | :--- |
| [`APP-REVIEW-NOTES.draft.md`](APP-REVIEW-NOTES.draft.md) | Pre-flight checklist and submission notes for Apple App Review, including a reproducible walkthrough, architecture disclosures, and open guideline questions. | Internal release prep + Apple App Review |
| [`APP-STORE-METADATA.draft.md`](APP-STORE-METADATA.draft.md) | English and Simplified Chinese App Store listing copy, keywords, and TestFlight beta test information. | App Store Connect listing & TestFlight |

### Internal Reference (Do Not Submit)
| Document | Purpose |
| :--- | :--- |
| [`REVIEW-NOTES.md`](REVIEW-NOTES.md) | Authoritative log of owner policy decisions, verified repository facts, legal qualifications, primary sources, and remaining objective blockers. |

---

## Core Boundaries

- **Separate legal instruments**: The GPLv3 Section 7 additional permission is a source-side licensing permission, not an EULA. The EULA governs binary distribution via Apple channels and cannot restrict rights granted independently under the GPL.
- **No CLA or proprietary relicensing**: These drafts do not create a Contributor License Agreement, dual license, or proprietary relicense.
- **No fabricated claims**: The drafts intentionally preserve placeholders for deferred personal legal identity fields and exact archive build/commit references, and they do not represent unresolved Apple guideline, privacy-manifest, or export-compliance items as completed.

---

## Verified App Store Connect Baseline

- **App Apple ID**: `6839642990`
- **Development Team**: `T73WQBSU73`
- **Bundle Identifier**: `org.trames.Trames` (File Provider extension: `org.trames.Trames.FileProvider`)
- **App Group**: `group.org.trames.Trames`
- **Draft Version**: `1.5.4` (manual release; final build number and immutable `trames-ipad` / `trames-runtime` / `trames-cli` commits to be filled from the archived build)
- **App Store Connect Record**: [Distribution Page](https://appstoreconnect.apple.com/apps/6839642990/distribution)

---

## Pre-Submission Gate Summary

Before adopting these documents or submitting a build, complete the objective tasks tracked in [`REVIEW-NOTES.md`](REVIEW-NOTES.md#objective-legal-account-and-build-completion-tasks):

1. **Right-holder & GPL review**: Complete the contribution history audit across `trames-ipad`, `trames-runtime`, and `trames-cli`; finalize the Section 7 Schedule and later-GPL-version review.
2. **Privacy manifest & data-flow audit**: Add a project-owned `PrivacyInfo.xcprivacy` for Trames app code (declaring `UserDefaults`, system uptime, and volume-capacity APIs with accurate Apple-approved reasons) and finalize App Privacy answers separately for production vs. TestFlight.
3. **Archive & payload inventory**: Confirm the exact build number, immutable Git commits, bundled third-party notices, and whether LGPL FFmpeg (`GuestVideo`) is included in the submitted archive.
4. **Permissions & background mode check**: Verify that `UIBackgroundModes` (`audio`) is used strictly for an Apple-permitted purpose (remove it if not required) and verify on-demand microphone consent.
5. **Apple guideline clarification**: Address Guideline 2.5.2, DPLA 3.3.1(B), Guideline 4.7 (4.7.1–4.7.5, including 4.7.2 native graphics bridge), and Guideline 2.5.6 openly with App Review.
6. **Personal & regional completion**: Fill deferred legal identity/contact fields, publish the Privacy Policy and Support URLs, complete App Store Connect export compliance, and rehearse every walkthrough step on the distribution-signed archive.
