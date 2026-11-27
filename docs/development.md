# Development

## Source and packaging

- [module/source](../module/source): native source used by the module: Zygisk bridge, donor loader, guarded imports, and pinned Zygisk API header.
- [module](../module): complete flashable module payload, including the working native binaries.
- [tools](../tools): native build and ZIP packaging tools.
- [GitHub Releases](https://github.com/hamzatuss/A57-LiveBlur/releases): installable ZIP and checksum.

To repack the included payload with Node.js 24 or newer:

`node tools/package.js`

To rebuild the JNI bridge and loader in Termux on the supported phone, install Clang and Node.js, then run:

`sh tools/build-native.sh`

`node tools/package.js`

The build uses the included, pinned donor library and the phone's system libraries. It does not rebuild Samsung's proprietary renderer or donor runtime. Output goes to `dist/`, and packaging refreshes the payload checksum manifest. Building and packaging do not install the module.

## Startup regression checks

Build the harmless linker fixture in Termux first:

`mkdir -p build`

`clang -O2 -fPIE -pie tools/linker-namespace-fixture.c -ldl -Wl,--no-as-needed /system/lib64/libdl_android.so -Wl,--dynamic-linker=/system/bin/linker64 -o build/linker-namespace-fixture`

Then run:

`su -c '/data/data/com.termux/files/usr/bin/node tools/test-renderer-startup.js'`

Run from the repository directory on the supported phone. Tests use a private namespace and temporary files. Android property writes and display-service restart requests are mocked; installed modules and live properties remain unchanged. They cover backend selection before mount, failed-pin refusal, healthy adoption, stale-inode refresh after updates, real Android linker namespace loss/recovery, boot guards, immediate recovery, PID cleanup and backup-bind recovery.

## Native blur tests

To run production-module scope, concurrent first-use and offscreen rendering tests in Termux on the supported phone with the flagship renderer active:

`node tools/test-native-blur.js`

The harness uses separate root app_process test runtimes, simulated specialization arguments and a private offscreen layer stack. It checks broader app scope, an unused API that leaves caches uninitialized, and an eight-thread first-use race that initializes once. Pixel samples verify ordinary blur and Samsung color curves. These tests do not substitute for live APK testing under each app’s sandbox.

## Implementation details

See [architecture.md](architecture.md) for pinned inputs, implementation history and validation records. Component attribution is in [NOTICE.md](../NOTICE.md).

## AOD resource overlay

The packaged APK is built from [overlay/aod-light-reveal](../overlay/aod-light-reveal) with `sh tools/build-aod-overlay.sh`. Requires aapt2, keytool and apksigner plus the supported phone’s framework-res.apk. The builder creates a local signing key in ignored build output and produces the static code-free overlay in module/system/product/overlay. Packaging requires this APK. No installation or overlay activation occurs during the build.

## Transition refresh listener

Set ANDROID_JAR to an API 35+ SDK android.jar and D8_JAR to build-tools lib/d8.jar, then run `sh tools/build-aod-refresh.sh`. Source is in module/source/aod-refresh; output is module/refresh/aod-refresh.dex. The helper uses Samsung display APIs only on the pinned firmware. `--self-test` briefly requests a vote and verifies timed release; `--probe` is a diagnostic panel-policy test, not a UI FPS benchmark. Run diagnostics only on the supported phone. The helper writes no preferences.

## SystemUI clock and View-pacing helper

With ANDROID_JAR and D8_JAR set, run `sh tools/build-clock-guard.sh` before repackaging changed Java source. Native bootstrap is in module/source/clock_guard.hpp and is rebuilt by tools/build-native.sh. Run `sh tools/test-clock-guard.sh` for isolated Android-view state/pacing tests. Set AOD_APK to the installed 8.9.67.3 base.apk and run `sh tools/test-samsung-clock-color.sh` for the actual Samsung color calculation on an unattached object. This last test uses constructor-free allocation only inside its standalone diagnostic process. No live SystemUI view is accessed.

`node tools/test-native-blur.js --clock-bootstrap-only` checks in-memory DEX loading through the production native entry point; the full command also checks blur, color curves and process scope. Its synthetic application uses the framework package and does not register the live SystemUI callbacks. The view fixtures exercise those state operations separately.
