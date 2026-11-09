# A57 LiveBlur

> **AI disclosure:** This project was made with AI assistance (OpenAI Codex).

Samsung flagship live blur and color curves for the Galaxy A57 5G, packaged as a KernelSU + Zygisk module.

## Supported build

This release is specific to **SM-A576B, Android 17 / One UI 9**, with this firmware fingerprint:

`samsung/a57xnaeea/a57x:17/CP2A.260605.016/A576BXXS6BZJ2_OXM6BZJ2:user/release-keys`

The installer and JNI bridge check the fingerprint. Other firmware needs a separate compatibility review and rebuild.

## Download and install

[Download v1.4.3](https://github.com/hamzatuss/A57-LiveBlur/releases/download/v1.4.3/Samsung-Native-Live-Blur-v1.4.3.zip) · [SHA-256](https://github.com/hamzatuss/A57-LiveBlur/releases/download/v1.4.3/SHA256SUMS.txt)

1. Have KernelSU and a working Zygisk implementation enabled. The original setup used Zygisk Next.
2. Install the ZIP through KernelSU.
3. Perform a full phone reboot. A framework-only restart does not rerun KernelSU boot scripts.

The package updates the existing `enable_live_blur` module. During installation it disables the old standalone `native_blur_renderer` module, if present, to prevent duplicate renderer mounts. The package contains both components.

The standalone components were confirmed working on the author's phone. Version 1.4 could skip an already-mounted flagship renderer, and its Action button could disable that renderer. Version 1.4.1 fixes both behaviors. Version 1.4.2 adds the framework app chooser and Android IntentResolver processes. Notification blur and color curves previously worked, but a v1.4.2 boot failed while initializing direct Skia Vulkan and Android rescue-rebooted. Version 1.4.3 pins the working GLES backend and improves first-failure recovery. Cold-boot activation and live chooser behavior after this update still need confirmation.

## How it works

- Enables the required background-blur property and Samsung floating-feature XML flag.
- Selectively hooks `SurfaceControl.nativeSetBlurRegionsEx` in SystemUI, Samsung launcher, AOD, Dressroom, framework UI (`system:ui`) and Android IntentResolver. The hook forwards the original arguments to unchanged Samsung donor machine code.
- Retains the stock installed `libandroid_runtime.so`. The donor is loaded separately, with unrelated JNI registrations suppressed.
- Uses the original SurfaceFlinger from **SM-S942B S942BXXU5BZJA**. It contains Samsung's `ColorCurveEffect` shader, which is absent from the target A57 renderer.
- Selects `skiaglthreaded` before starting the renderer, matching the working Skia GLES/ANGLE path on this phone.
- Binds that renderer into init's mount namespace at boot. No system partition is flashed.

The donor's executable sections remain unchanged. Four unavailable, unrelated BLAST imports in the JNI donor have aborting guards; the tested blur calls did not use them. Native implementation provenance is recorded in [module/donor/PROVENANCE.json](module/donor/PROVENANCE.json) and [module/PROVENANCE.json](module/PROVENANCE.json).

## Rollback

The boot check verifies the running renderer hash and boot completion. It polls every 0.25 seconds and restores stock on the first service restarting state or loss of a confirmed donor process. Recovery first unmounts the donor; if that fails, it can bind the verified stock backup. It restarts the service only when stock is not already running. It sets `renderer-disabled`, leaving the JNI bridge enabled. An unfinished previous trial also skips the renderer on the next boot.

The module's **Action** button displays status without changing the renderer. To explicitly restore stock while keeping the JNI bridge, run `su -c 'sh /data/adb/modules/enable_live_blur/restore-renderer.sh'`. To undo the whole module, disable/remove `enable_live_blur` in KernelSU and reboot.

Installing an update clears stale renderer retry/disabled markers in the staged module. Boot trials record the kernel boot ID so repeated setup in the same boot is not mistaken for a failed previous boot.

Logs:

`su -c 'cat /data/adb/modules/enable_live_blur/renderer-status.log'`

`su -c 'logcat -d -s NativeBlurDonor'`

## Source and packaging

- [module/source](module/source): native source used by the module: Zygisk bridge, donor loader, guarded imports, and pinned Zygisk API header.
- [module](module): complete flashable module payload, including the working native binaries.
- [tools](tools): native build and ZIP packaging tools.
- [GitHub Releases](https://github.com/hamzatuss/A57-LiveBlur/releases): installable ZIP and checksum.

To repack the included payload with Node.js 24 or newer:

`node tools/package.js`

To rebuild the JNI bridge and loader in Termux on the supported phone, install Clang and Node.js, then run:

`sh tools/build-native.sh`

`node tools/package.js`

The build uses the included, pinned donor library and the phone's system libraries. It does not rebuild Samsung's proprietary renderer or donor runtime. Output goes to `dist/`, and packaging refreshes the payload checksum manifest. Building and packaging do not install the module.

## Startup regression checks

`su -c '/data/data/com.termux/files/usr/bin/node tools/test-renderer-startup.js'`

Run from the repository directory on the supported phone. Tests use a private namespace and temporary files. Android property writes and display-service restart requests are mocked; installed modules and live properties remain unchanged. They cover backend selection before mount, failed-pin refusal, adoption, boot guards, immediate recovery, PID cleanup and backup-bind recovery.

## Chooser coverage

The system resolves “Open with” to `android/com.android.internal.app.ResolverActivity` in process `system:ui`. Its Samsung blur helper requests a blur, but earlier releases did not hook that process. Version 1.4.2 adds an exact `system:ui` match and the separate `com.android.intentresolver` package. Coverage remains an explicit process allowlist.

To run production-module scope and offscreen rendering checks in Termux on the supported phone with the flagship renderer active:

`node tools/test-native-blur.js`

The harness uses separate root app_process runtimes and a private offscreen layer stack. It checks chooser/SystemUI blur and curves and verifies that system_server and unrelated processes are excluded. It does not flash the module or capture the visible UI.

## Validation and provenance

The original investigation verified LP geometry/metadata SHA-256, the complete Android firmware package's ZIP CRC and length, renderer dependencies and symbols, layer-state layout/defaults, isolated renderer rollback, unfinished-trial handling, and read-only executable mount flags. Earlier offscreen tests showed ordinary blur working while color-curve parameters had no visual effect with the A57 renderer.

Build-specific measurements and provenance are in [docs/architecture.md](docs/architecture.md). Samsung binaries remain Samsung components; this repository does not claim ownership of them. The included Zygisk API header retains its upstream copyright and permission notice.
