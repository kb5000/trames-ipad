# Trames iPad — App Review Notes & Submission Guide (Draft)

> **How to use this document**
> 1. Complete every blocking item in **Section 1 (Internal Pre-Flight Checklist)** before uploading an archive.
> 2. Fill all `[PLACEHOLDER]` fields with verified values from the exact distribution-signed archive.
> 3. Copy **only Section 2** into **App Store Connect → App Review Information → Notes**.

---

## Section 1: Internal Pre-Flight Checklist (Do Not Submit to Apple)

### 1.1 Initial App Store Release Scope (Build Gates)
- [ ] **Picture in Picture Excluded**: Confirm `WindowPictureInPicture` UI/code and `UIBackgroundModes: [audio]` are compiled out / removed from `Trames-Info.plist` for the initial App Store release so backgrounding cleanly pauses the local interpreter with zero Guideline 2.5.4 surface.
- [ ] **Android Excluded**: Confirm the experimental Android environment option and `GuestBionic` payload are compiled out / excluded from the initial App Store archive.
- [ ] **Archive & Source Identity**:
  - App Apple ID: `6839642990` (Team `T73WQBSU73`)
  - Bundle IDs: `org.trames.Trames` + `org.trames.Trames.FileProvider` (App Group `group.org.trames.Trames`)
  - Archived Version & Build: `[PLACEHOLDER: e.g., 1.5.4 (50)]`
  - Tested Hardware / OS: `[PLACEHOLDER: e.g., iPad Air (M3), iPadOS 18.x]`
  - Immutable Source Revisions:
    - `trames-ipad`: `[TRAMES_IPAD_TAG_OR_COMMIT]`
    - `trames-runtime`: `[TRAMES_RUNTIME_COMMIT]`
    - `trames-cli`: `[TRAMES_CLI_COMMIT]`
- [ ] **Bundled Guest Payloads Confirmed**: Confirm exact bundled folders (`GuestMusl`, `GuestGlibc`, `GuestVulkan`, and whether LGPL FFmpeg `GuestVideo` is included) and verify `THIRD_PARTY_NOTICES.txt` matches.

### 1.2 Technical & Compliance Gates
- [ ] **Project-Owned `PrivacyInfo.xcprivacy` Added**: Add a project-owned `PrivacyInfo.xcprivacy` for Trames app code declaring `UserDefaults`, `ProcessInfo.systemUptime`, and volume-capacity APIs with accurate Apple-approved reasons.
- [ ] **Microphone Consent Verified**: Confirm microphone access is requested only when the user enables audio capture for an environment and matches `NSMicrophoneUsageDescription` (or remove the key if audio capture is disabled in the initial build).
- [ ] **Export Compliance Completed**: Complete App Store Connect's encryption determination for HTTPS/TLS and any linked cryptographic libraries in the final archive. Do not set `ITSAppUsesNonExemptEncryption` before finishing that determination.
- [ ] **Rehearsal on Distribution-Signed Archive**: Execute every step in Section 2 on a freshly installed, distribution-signed build and attach a short demo video covering Wayland graphics and Debian setup.

### 1.3 App Store Connect Metadata
- [ ] **Pricing & Availability**: Free ($0.00), no IAP, no subscriptions; all Apple-available regions.
- [ ] **Review Contact**:
  - Name: `[PLACEHOLDER: OWNER FULL LEGAL NAME]`
  - Phone: `[PLACEHOLDER: PHONE WITH COUNTRY CODE]`
  - Email: `a123edcrfv@gmail.com`
- [ ] **Public URLs Live**:
  - Privacy Policy URL: `[PRIVACY_POLICY_URL]`
  - Support URL: `[SUPPORT_URL]`
- [ ] **Demo Video Attached**: `[ATTACHMENT_FILENAME]`

---

## Section 2: App Store Connect Review Notes (Submit to Apple)

Dear Apple App Review Team,

Thank you for reviewing **Trames** (Apple ID `6839642990`, Version `[APP_VERSION_AND_BUILD]`).

Trames is a free, open-source, local Linux terminal and developer workspace for iPadOS. It allows developers, students, and hobbyists to run unmodified ARM64 Linux command-line tools and optional Wayland graphical utilities inside the standard iPadOS sandbox, with multi-tab terminals, iPadOS Files integration, and USB file sharing.

- **No Account Required**: No sign-in, registration, or cloud account is required.
- **No Monetization**: Completely free—no in-app purchases, subscriptions, or ads.
- **Strictly Within the App Sandbox**: Trames operates solely inside standard iPadOS windows. It does not replace the Home Screen or act as an alternate app store.

---

### 1. Quick Review Walkthrough (~3 Minutes)

#### Step 1: Launch & Initialize the Default Alpine Linux Environment
1. Launch **Trames**.
2. On a fresh install, Trames automatically opens the default **Alpine** environment, downloads the pinned Alpine ARM64 rootfs over HTTPS, verifies its SHA-256 checksum, and unpacks it inside the app's private sandbox container (~10–20 seconds on a standard connection).
   - *(Optional: To create another environment such as Debian 13 or a custom ARM64 rootfs, tap **+** in the top bar → **Linux Environments** → **New Environment** → select the distribution → tap **Create Environment**, then tap **Open Environment** in the detail view.)*

#### Step 2: Verify Local Userspace Execution
Once the shell prompt (`#`) appears, run:
```sh
uname -m
cat /etc/os-release
printf "Trames review check OK\n"
```
**Expected result**: `aarch64`, Alpine Linux release info, and `Trames review check OK`.

#### Step 3: Verify Sandboxed Persistence & Files Integration
In the terminal, create a test file:
```sh
echo "Persistent sandbox test" > /root/review-check.txt
cat /root/review-check.txt
```
- **In-app Files panel**: Tap **+** (top right) → **Files** to import or export files between iPadOS and the active environment.
- **iPadOS Files app**: Open the system **Files** app → **Browse** → enable **Trames** if needed → open **Trames → Alpine → root → review-check.txt**.
- Force-quit and relaunch Trames to confirm `/root/review-check.txt` persists inside the app container.

#### Step 4: Optional Package Installation
```sh
apk update && apk add file
file /bin/busybox
```
Packages are fetched from Alpine's standard repositories and unpacked strictly inside the private guest virtual filesystem.

*(Optional Wayland graphical windows and Debian setup are demonstrated in the attached video `[ATTACHMENT_FILENAME]`.)*

---

### 2. Compliance & Architecture Summary

Trames is engineered specifically to comply with standard iPadOS security and App Review requirements:

1. **Pure Software Interpretation — Compliant with Guideline 2.5.2 & DPLA 3.3.1(B)**
   - Trames **does not use JIT compilation**, does not map writable-executable (`W^X`) pages, and declares **no JIT or dynamic-codesigning entitlements**.
   - All guest ARM64 Linux ELF binaries are executed by an in-process Rust software interpreter (`trames-interpreter`). Guest code never executes as native Mach-O code on the host, cannot link against iPadOS frameworks, and cannot invoke private or arbitrary host APIs.
   - Trames is a local programming and command-line workspace for developers and learners; all guest commands are explicitly entered, visible, and executed by the user inside the sandbox without altering the host app's own binary or features.

2. **Signed Static Graphics Bridge & Helpers — Compliant with Guideline 4.7.2**
   - All native host code—including the optional Wayland compositor, the Vulkan-to-Metal presentation bridge (via static MoltenVK), and bounds-checked memory/compute helpers—is statically compiled and signed by the developer as part of the App Store binary itself.
   - No native host code is ever downloaded or dynamically loaded at runtime. Guest software communicates only through constrained software protocols (Wayland/Vulkan protocol messages) to render surfaces inside Trames's own iPadOS views, with zero unrestricted access to host platform APIs.

3. **Standard Linux Developer Package Managers — Compliant with Guidelines 4.7.1, 4.7.3, 4.7.4 & 4.7.5**
   - Standard Linux package utilities (`apk`, `apt`) run inside the user's terminal strictly to install open-source developer and CLI dependencies (compilers, editors, Python, Git, libraries) into the user's isolated Linux rootfs—matching the core purpose of a local Linux development environment.
   - Trames is not a mini-app catalog, game store, or alternate iPadOS app distribution channel. There is no third-party mini-app storefront, no in-app commerce, and no sharing of user personal data with third-party software. Any access to host files or clipboard is explicitly gated by iPadOS system permissions and per-environment host-access controls.

4. **Not a Web Browser — Compliant with Guideline 2.5.6**
   - Trames is a terminal and Linux workspace, not a web browser. It does not bundle a web browser, does not register `http`/`https` URL schemes, and does not use a custom web engine to render any part of the app's UI or web content (any external links open through standard iPadOS system handling).

---

### 3. Entitlements, Permissions & Background Behavior

- **App Group (`group.org.trames.Trames`)**: Shared strictly between the main app and the embedded `TramesFileProvider` extension so users can browse and edit Linux environment files from the iPadOS Files app.
- **Increased Memory Limit (`com.apple.developer.kernel.increased-memory-limit`)**: Provides headroom for memory-intensive developer tasks (such as compiling code or running language runtimes) on supported iPads; falls back gracefully where unavailable.
- **Local Network (`NSLocalNetworkUsageDescription`)**: Requested on demand when a user connects to local network services from inside a Linux environment (e.g., `ssh`, `curl`, local dev servers).
- **Microphone (`NSMicrophoneUsageDescription`)**: Requested strictly on demand only if the user explicitly enables audio capture for an environment.
- **Background Behavior**: Entering the background immediately suspends GPU work, pauses environment downloads, and pauses the local Linux interpreter. Trames does not run a background daemon and declares no persistent background execution mode.

---

### 4. Privacy, Encryption & Open-Source Provenance

- **Production Privacy**: The production App Store binary contains **no third-party analytics SDKs, no tracking libraries, and no automatic log uploader**. Diagnostics are shared only if the user explicitly exports or sends them (or opts into Apple's system crash sharing).
- **TestFlight**: TestFlight beta builds are subject to Apple's standard TestFlight crash-log and usage-data collection, which is disclosed separately in our Privacy Policy (`[PRIVACY_POLICY_URL]`).
- **Privacy Manifest**: `[PLACEHOLDER: CONFIRM PROJECT-OWNED PrivacyInfo.xcprivacy IS BUNDLED IN THIS ARCHIVE BEFORE SUBMITTING]`.
- **Export Compliance**: Trames uses standard HTTPS/TLS for verified rootfs downloads, and users may run standard networking/SSH tools inside guest Linux environments. `[PLACEHOLDER: CONFIRM EXACT EXPORT COMPLIANCE DETERMINATION FOR THE FINAL ARCHIVE]`.
- **Open-Source Licensing**: Trames is licensed under GPL-3.0-or-later with a GPLv3 Section 7 additional permission for App Store and TestFlight distribution. Exact corresponding source for this build is published at:
  - `https://github.com/project-trames/trames-ipad` (`[TRAMES_IPAD_TAG_OR_COMMIT]`)
  - `https://github.com/project-trames/trames-runtime` (`[TRAMES_RUNTIME_COMMIT]`)
  - `https://github.com/project-trames/trames-cli` (`[TRAMES_CLI_COMMIT]`)

---

### 5. Contact Information

If you need any clarification, an additional demo video, or an adjusted build, please contact:

- **Name**: `[PLACEHOLDER: OWNER FULL LEGAL NAME]`
- **Email**: `a123edcrfv@gmail.com`
- **Phone**: `[PLACEHOLDER: PHONE WITH COUNTRY CODE]`
- **Support**: `[SUPPORT_URL]`

Thank you for your time and review.
