# Apple App Store and TestFlight Additional Permission (Draft)

**Supplementary Permission under GNU GPLv3, Section 7, for Covered Trames Components**

> **Status**: Review draft only. This permission is not in effect until formally adopted by the copyright holders listed in the Schedule for the specific repositories and revision boundaries identified below.

---

## 1. Purpose and Relationship to the GNU GPL

The software identified in the Schedule below (the **"Covered Material"**) is licensed under the GNU General Public License, version 3, or (at your option) any later version (the **"GPL"**).

Pursuant to **Section 7 of GPLv3**, each Grantor listed in the Schedule grants this additional, non-exclusive permission to any licensee who conveys the Covered Material. All terms of the GPL remain in full force. This permission exists solely to reconcile the GPL with the mandatory platform terms of Apple's App Store and TestFlight; it is **not** a dual license, a contributor license agreement, or a proprietary relicense.

If you convey the Covered Material under a version of the GPL later than version 3, this permission applies only to the extent recognized and permitted by that later version.

---

## 2. Definitions

- **Apple Distribution Services**: Apple Inc.'s production App Store and TestFlight beta distribution services, including Apple's technical mechanisms for code signing, sandboxing, installation, updates, and beta management on iPadOS.
- **Required Apple Terms**: The mandatory, non-negotiable legal agreements, usage rules, and technical restrictions imposed by Apple Inc. as an unavoidable condition of distributing an application through Apple Distribution Services (including the Apple Developer Program License Agreement, Apple Media Services Terms and Conditions, and TestFlight Terms of Service). Required Apple Terms do **not** include optional restrictions, proprietary EULAs, or technical locks voluntarily added by a distributor.
- **Covered Material**: The original copyrightable work owned or validly sublicensed by the Grantors as specified in the Schedule. Covered Material excludes third-party upstream libraries, vendored dependencies, and third-party guest distributions or payloads.
- **Covered App**: Any iPadOS application bundle containing Covered Material that is conveyed through Apple Distribution Services.

---

## 3. Grant of Additional Permission

You are granted permission to convey a **Covered App** (whether modified or unmodified) through **Apple Distribution Services**, notwithstanding any conflict between the GPL and the **Required Apple Terms** that arises solely because the Covered App is distributed through those services.

This permission is granted equally to all qualifying downstream licensees and distributors. It is not restricted to official maintainers or any specific Apple Developer account, and it does not restrict a downstream distributor from charging a fee for a permitted conveyance as allowed by the GPL.

### Limits of the Grant

This permission excuses only conflicts strictly caused by Required Apple Terms. It does **not** permit you to:
1. Distribute closed-source or proprietary modifications of Covered Material;
2. Withhold or restrict access to Corresponding Source; or
3. Add DRM, obfuscation, or technical restrictions beyond those automatically enforced by Apple Distribution Services.

---

## 4. Conditions of Reliance

To rely on this additional permission when conveying a Covered App through Apple Distribution Services, you must satisfy all of the following conditions:

### 4.1 Corresponding Source
- You must make the complete **Corresponding Source** for the exact version of the Covered App conveyed available at no charge (via a durable public network server or an accompanying written offer as permitted by the GPL), including all build scripts, configuration files, and assets under your control needed to build the Covered App.
- Each release must identify its exact immutable Git tags or commit hashes across `trames-ipad`, `trames-runtime`, and `trames-cli` (or your corresponding forks).

### 4.2 License and Attribution Notices
- Within the Covered App's user interface, settings, or bundled documentation, you must retain and make accessible:
  1. The full text of the GNU General Public License v3;
  2. The full text of this Additional Permission;
  3. All original copyright, author, and third-party license notices; and
  4. Clear instructions and URLs for obtaining the Corresponding Source.

### 4.3 Consumer Terms and Technical Restrictions
- Any End-User License Agreement (EULA) or terms accompanying the Covered App must expressly preserve the recipient's independent rights under the GPL and state that, in any conflict, the GPL and this permission govern for the Covered Material.
- You must not introduce encryption, obfuscation, or anti-inspection measures beyond Apple's mandatory platform mechanisms.

### 4.4 TestFlight Confidentiality Boundaries
- Where TestFlight imposes default confidentiality on pre-release builds, you must not designate GPL-licensed source code, copyright notices, license texts, Corresponding Source, or public project documentation as confidential. Testers remain free to inspect, fork, and publicly discuss the open-source code.

### 4.5 Clear Origin and Distinct Listing Identity
As a condition of relying on this Apple-distribution permission (and, for modified versions, in alignment with GPLv3 Section 7(c)):
1. **Distinct Listing Name**: Any unofficial or third-party listing (whether a modified fork or a re-signed/relisted copy) must use a listing name that clearly distinguishes it from the official Trames release. The listing name may include the word `Trames` provided it includes a distinct identifying element (for example, *"Trames Community Edition"* or *"Acme Shell for Trames"*) and does not imply official endorsement or origin.
2. **Distinct App Icon**: An unofficial listing must use an original, lawfully usable app icon that is visually distinct from the official Trames app icon.
3. **Permitted Factual References**: This condition does not restrict truthful attribution (such as *"Based on Trames"*), accurate compatibility statements, links to official releases, or retention of original source history, copyright notices, crate/module names, protocol identifiers (`TRMEXEC1`, `TRAMES02`), or other internal code identifiers.
4. **No Trademark Registration Claim**: Nothing in this document claims that `Trames` is a registered trademark or grants or waives any separate rights that may or may not exist under applicable trademark law.

---

## 5. Third-Party Software

This permission is granted solely by the Grantors for the Covered Material listed in the Schedule. Grantors have no authority to grant exceptions for third-party software—including, without limitation, FFmpeg, glibc, musl, Mesa, MoltenVK, Xwayland, SwiftNIO, Citadel, or Apple system libraries. Every distributor remains independently responsible for complying with the license of each third-party component included in a Covered App.

---

## 6. Removal of Additional Permission

In accordance with Section 7 of GPLv3, any recipient may remove this additional permission when conveying modified or unmodified versions of the Covered Material. Removing this permission terminates the right to rely on it for distribution through Apple Distribution Services.

---

## Schedule of Covered Material and Grantors

*(To be completed and verified upon formal adoption for an official release.)*

| Repository | Scope of Covered Material | Revision / Tag Boundary | Grantor(s) | Adoption Date |
| :--- | :--- | :--- | :--- | :--- |
| `project-trames/trames-ipad` | Original commits owned by Grantor | `[COMMIT / TAG]` | `[LEGAL NAME]` | `[DATE]` |
| `project-trames/trames-runtime` | Original commits owned by Grantor | `[COMMIT / TAG]` | `[LEGAL NAME]` | `[DATE]` |
| `project-trames/trames-cli` | Original commits owned by Grantor | `[COMMIT / TAG]` | `[LEGAL NAME]` | `[DATE]` |

### Completion Record (Not Part of the Permission)
- **Contact**: `a123edcrfv@gmail.com`
- **Grantor Jurisdiction**: `[PLACEHOLDER: JURISDICTION]`
- **Legal Reviewer**: `[PLACEHOLDER: REVIEWER]`
- **Associated Release Build**: `[APP_VERSION_AND_BUILD]`
- **Immutable Pins**:
  - `trames-ipad`: `[TRAMES_IPAD_TAG_OR_COMMIT]`
  - `trames-runtime`: `[TRAMES_RUNTIME_COMMIT]`
  - `trames-cli`: `[TRAMES_CLI_COMMIT]`
