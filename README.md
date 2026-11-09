# A57 LiveBlur

> **AI disclosure:** This project was made with AI assistance (OpenAI Codex).

Samsung flagship live blur and color curves for the Galaxy A57 5G, packaged as a KernelSU + Zygisk module.

## Supported build

This release is specific to **SM-A576B, Android 17 / One UI 9**, with this firmware fingerprint:

`samsung/a57xnaeea/a57x:17/CP2A.260605.016/A576BXXS6BZJ2_OXM6BZJ2:user/release-keys`

The installer and JNI bridge check the fingerprint. Other firmware needs a separate compatibility review and rebuild.

## Download and install

[Download v1.4 Combined](https://github.com/hamzatuss/A57-LiveBlur/releases/download/v1.4/Samsung-Native-Live-Blur-v1.4-Combined.zip) · [SHA-256](https://github.com/hamzatuss/A57-LiveBlur/releases/download/v1.4/SHA256SUMS)

1. Have KernelSU and a working Zygisk implementation enabled. The original setup used Zygisk Next.
2. Install the ZIP through KernelSU.
3. Reboot.

The package updates the existing `enable_live_blur` module. During installation it disables the old standalone `native_blur_renderer` module, if present, to prevent duplicate renderer mounts. The combined package contains both components.

The two standalone components were confirmed working on the author's phone. The combined v1.4 ZIP passed archive, payload and shell checks; combined installation has not yet been confirmed separately.

## How it works

- Enables the required background-blur property and Samsung floating-feature XML flag.
- Selectively hooks `SurfaceControl.nativeSetBlurRegionsEx` in SystemUI, Samsung launcher, AOD and Dressroom. The hook forwards the original arguments to unchanged Samsung donor machine code.
- Retains the stock installed `libandroid_runtime.so`. The donor is loaded separately, with unrelated JNI registrations suppressed.
- Uses the original SurfaceFlinger from **SM-S942B S942BXXU5BZJA**. It contains Samsung's `ColorCurveEffect` shader, which is absent from the target A57 renderer.
- Binds that renderer into init's mount namespace at boot. No system partition is flashed.

The donor's executable sections remain unchanged. Four unavailable, unrelated BLAST imports in the JNI donor have aborting guards; the tested blur calls did not use them. Native implementation provenance is recorded in [module/donor/PROVENANCE.json](module/donor/PROVENANCE.json) and [module/PROVENANCE.json](module/PROVENANCE.json).

## Rollback

The boot check verifies the running renderer hash, watches for repeated crashes, and checks boot completion. A failed trial attempts to restore stock through unmounting and restarting the display service. It sets `renderer-disabled`, leaving the JNI bridge enabled. An unfinished previous trial also skips the renderer on the next boot.

Use the module's **Action** button to restore only the stock renderer. To undo the whole combined module, disable/remove `enable_live_blur` in KernelSU and reboot.

Logs:

`su -c 'cat /data/adb/modules/enable_live_blur/renderer-status.log'`

`su -c 'logcat -d -s NativeBlurDonor'`

## Source and packaging

- [module/source](module/source): native source used by the combined module: Zygisk bridge, donor loader, guarded imports, and pinned Zygisk API header.
- [module](module): complete flashable module payload, including the working native binaries.
- [tools](tools): native build and ZIP packaging tools.
- [GitHub Releases](https://github.com/hamzatuss/A57-LiveBlur/releases): combined installable ZIP and checksum.

To repack the included payload with Node.js 24 or newer:

`node tools/package.js`

To rebuild the JNI bridge and loader in Termux on the supported phone, install Clang and Node.js, then run:

`sh tools/build-native.sh`

`node tools/package.js`

The build uses the included, pinned donor library and the phone's system libraries. It does not rebuild Samsung's proprietary renderer or donor runtime. Output goes to `dist/`, and packaging refreshes the payload checksum manifest. Building and packaging do not install the module.

## Validation and provenance

The original investigation verified LP geometry/metadata SHA-256, the complete Android firmware package's ZIP CRC and length, renderer dependencies and symbols, layer-state layout/defaults, isolated renderer rollback, unfinished-trial handling, and read-only executable mount flags. Earlier offscreen tests showed ordinary blur working while color-curve parameters had no visual effect with the A57 renderer.

Build-specific measurements and provenance are in [docs/architecture.md](docs/architecture.md). Samsung binaries remain Samsung components; this repository does not claim ownership of them. The included Zygisk API header retains its upstream copyright and permission notice.
