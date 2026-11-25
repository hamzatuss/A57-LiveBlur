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
