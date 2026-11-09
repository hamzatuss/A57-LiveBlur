# Architecture and pinned inputs

## Target

SM-A576B, A576BXXS6BZJ2_OXM6BZJ2; the exact fingerprint is in `module/source/expected_build.hpp` and `module/renderer.conf`.

## JNI donor

Source release: https://github.com/enderbk0/a36toflagship/releases/tag/1.0.6

Original donor runtime SHA-256: `94426b4178088372552ecbb35905bd4ebfe3ce726cbe751690b72c618e1c3125`.

The release does not establish the original donor device/build. Only the SONAME was renamed in the supplied donor; executable sections were checked unchanged. The SurfaceControl registrar is pinned at `0x1724d0`; the extended-blur routine is at `0x2276cc`. The loader verifies the registrar offset and routine entry bytes.

The module temporarily substitutes the current thread's JNI RegisterNatives callback while invoking the donor registrar to initialize caches. It suppresses 154 unrelated registrations and restores the original JNI table. Only the extended-blur method is hooked in selected app processes; system_server and ordinary apps are excluded.

Zygisk API header: https://github.com/topjohnwu/zygisk-module-sample at commit `7bb941ac8edfcffd1d23761e401c45ca95409dc1`, API version 4.

## Renderer

Official Samsung firmware input: SM-S942B / EUX / S942BXXU5BZJA.
Archive: `SM-S942B_2_20261016025731_0iaygqx1ga_fac.zip.enc4`.

The firmware's Android package was range-streamed and decrypted; its super image was decoded and only `system_a` reconstructed. LP metadata checksums and the full Android package ZIP CRC/length passed. The 8.29 GiB system image was mounted read-only for file extraction.

SurfaceFlinger SHA-256: `103c5493947529c46bd46ad7e164c581ed39095ab24aee973e3d87b8932842dc`.
Stock A57 renderer SHA-256: `5bee19df69e0f2b9cb32dce7fd02c37857c01cd7c328e2f5a9daab04402e1dad`.

The donor linked against the existing target libraries with no unavailable libraries or missing strong symbols in the 92-library dependency check. Layer-state constructor field layout and five default vectors matched. These checks support this build-specific port; they do not establish cross-device compatibility.

Boot scripts perform a guarded bind mount and remount the file read-only, executable, with service-domain transitions allowed. A separate namespace test checked those mount flags. Rollback unmount was exercised privately with the display restart request intercepted, so the live display was not restarted during that test.

## Combined packaging

Module ID remains `enable_live_blur`. The installer disables the old standalone `native_blur_renderer` on flash. The combined payload has no root `skip_mount` marker, allowing its floating-feature XML overlays to load. Renderer rollback uses a component marker rather than disabling the combined module.

The GitHub Release ZIP is built from the combined module payload. Standalone module documentation is excluded. Repacking refreshes the checksum manifest; native binaries remain identical unless explicitly rebuilt.

## v1.4.1 startup regression checks

The installed v1.4 instance was running the stock A57 executable, had a renderer-disabled marker, and logged an earlier original-renderer mismatch. Its Action handler created that marker. The startup guard also rejected a verified donor already mounted by the older module.

v1.4.1 accepts that donor without stacking a bind, ties unfinished trials to the kernel boot ID, makes Action read-only, moves manual restore to restore-renderer.sh, and clears stale trial/disabled markers only in the staged installer directory.

Root integration tests in tools/test-renderer-startup.js use a private mount namespace, a temporary target file, and fixture copies of scripts. Service restart requests are intercepted. Tests exercise initial/repeated startup, donor adoption, disabled and failed-boot guards, unknown hash refusal, status-only Action, actual rollback unmount, and staged marker cleanup/migration. The tests do not modify installed modules or the live renderer.

## v1.4.2 chooser coverage

Package-manager resolution of a text-file VIEW intent identifies android/com.android.internal.app.ResolverActivity running as system:ui. Framework DEX inspection shows SemBlurHelper checking FW_WINDOW_BLUR_SUPPORTED and applying SemBlurInfo. That feature flag comes from the already-enabled 3D surface transition floating feature. The previous process allowlist omitted system:ui.

The update adds an exact system:ui process match and com.android.intentresolver package matching. The real rebuilt module was loaded through its Zygisk entry point in isolated Android runtimes. Offscreen checkerboard pixels were baseline 0xff000000, ordinary blur 0xff757575, and Samsung curve 0xffcdcdcd for system:ui, com.android.intentresolver and com.android.systemui. System_server (server callback), system, system:ui:unrelated and an unrelated app received no hook. Tests are reproducible with tools/test-native-blur.js.

The Samsung donor libraries and renderer are unchanged. Actual chooser UI behavior remains pending user installation and a full reboot.

## v1.4.3 startup failure and recovery

The v1.4.2 failure was captured in five SurfaceFlinger tombstones at 23:23:33 through 23:23:53 on 2026-11-09. The archived boot log selected Threaded RenderEngine with SkiaVk Backend (Ganesh), then reported Could not find any physical devices, bailing and Could not initialize Vulkan RenderEngine. Android performed a rescue reboot. The unfinished-trial guard then kept the stock A57 executable active.

The repaired module sets debug.renderengine.backend=skiaglthreaded in its early property payload and verifies the setting before any donor bind. The supplied Samsung executable reads that property in the selector near 0x538b58; its threaded GLES branch is at 0x538c7c. The executable itself is unchanged. The corresponding AOSP backend-selection code is in services/surfaceflinger/SurfaceFlinger.cpp: https://android.googlesource.com/platform/frameworks/native/+/refs/heads/main/services/surfaceflinger/SurfaceFlinger.cpp

The old watchdog sampled every two seconds, could miss one-second startup attempts and had a literal-dollar PID-cleanup typo. v1.4.3 polls every quarter second, checks init.svc.surfaceflinger for restarting, detects loss of a confirmed donor PID, and uses nsenter -F so the launch PID and shell PID agree. Cleanup uses the correct shell PID. Recovery can bind a checksum-verified, executable stock backup if ordinary unmount fails.

Private integration tests verified backend pin ordering, pin-failure refusal, existing-donor adoption, disabled/previous-boot/unknown-hash guards, status-only Action, first-restart rollback, next-poll PID-loss rollback, PID cleanup, stock-backup recovery and staged update marker cleanup. Android property writes and service restarts were intercepted. No live renderer or installed module was modified during these tests. Real cold-boot confirmation remains pending user installation.
