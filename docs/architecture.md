# Architecture and pinned inputs

## Target

SM-A576B, A576BXXS6BZJ2_OXM6BZJ2; the exact fingerprint is in `module/source/expected_build.hpp` and `module/renderer.conf`.

## JNI donor

Source release: https://github.com/enderbk0/a36toflagship/releases/tag/1.0.6

Original donor runtime SHA-256: `94426b4178088372552ecbb35905bd4ebfe3ce726cbe751690b72c618e1c3125`.

The release does not establish the original donor device/build. Only the SONAME was renamed in the supplied donor; executable sections were checked unchanged. The SurfaceControl registrar is pinned at `0x1724d0`; the extended-blur routine is at `0x2276cc`. The loader verifies the registrar offset and routine entry bytes.

The module temporarily substitutes the current thread's JNI RegisterNatives callback while invoking the donor registrar to initialize caches. It suppresses 154 unrelated registrations and restores the original JNI table. In v1.5.0, donor mappings are prepared during app specialization and that JNI initialization is deferred until the first extended-blur call. Initialization is serialized per app process; the original native entry remains a fallback if initialization reports failure. The extended-blur method is hooked across eligible app processes; system_server, core process launchers and child zygotes are excluded.

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

## v1.5.0 app-wide coverage

The package allowlist was removed from preAppSpecialize. The app callback accepts named app processes while excluding child zygotes and core names system_server, system, zygote, zygote64 and webview_zygote. The server-specialization callback still unloads without hooking. This affects only processes receiving the module from Zygisk; its configured exclusions remain effective.

The loader maps and validates donor libraries before app sandbox restrictions apply. It does not initialize donor JNI caches at that point. The first extended-blur call initializes caches under a mutex, publishes completion with acquire/release synchronization, and then invokes the original donor routine. Apps that never call the API retain no donor JNI-cache initialization. Native Samsung executable sections remain unchanged.

Eighteen isolated process-scope scenarios passed on the supported phone: app/chooser/SystemUI names, app subprocesses, a multi-user UID case, a no-blur case, and server/core/child-zygote exclusions. Eleven rendered checkerboard cases produced baseline 0xff000000, ordinary blur 0xff757575 and Samsung curve 0xffcdcdcd. Eight attached threads completed concurrent first use without errors; the log showed exactly one donor cache initialization. The no-blur case installed the hook without initializing those caches. These fixtures simulate specialization arguments in root runtimes rather than launching real apps under their SELinux domains.

The Samsung donor libraries, renderer and v1.4.3 GLES/recovery scripts are unchanged. The v1.4.3 renderer was verified active with skiaglthreaded and a passed boot trial. Individual app UI confirmation for v1.5.0 remains pending installation.

## v1.5.1 retained executable mount after updates

The 2026-11-10 00:43:02 donor startup used SkiaGL but failed ANGLE Vulkan initialization with error -3, EGL_NOT_INITIALIZED, then no suitable EGLConfig. Its executable was marked deleted. Driver loading logged fallback to the current namespace instead of sphal. The stock fallback started successfully five seconds later using the same Samsung ANGLE driver.

The module update had replaced the donor file behind a retained bind mount. The startup script accepted its matching checksum and reused the unlinked inode. Android's linker resolves /proc/self/exe with realpath before selecting a configuration; if that fails it falls back to /proc/self/exe, which matches no system directory section. The default configuration then lacks the exported system/sphal namespaces. This behavior is implemented in AOSP linker/linker_main.cpp (get_executable_info) and linker/linker_config.cpp (parse_config_file): https://android.googlesource.com/platform/bionic/+/refs/heads/main/linker/

A private-namespace test with a harmless executable reproduced the sequence: healthy bind had sphal=1/default=1; source unlink and replacement left realpath failing and sphal=0/default=0; a fresh bind restored both namespaces. The production startup script now checks device/inode identity in init's mount namespace. A healthy current-file mount is reused. Stale known donor mounts are removed with a bounded loop, the underlying stock hash is checked, and the current verified donor is bound again with executable/read-only flags. Unknown executables and unmount failures are refused. Recovery is unchanged.

Regression checks exercise the production startup refresh using identical-checksum deleted source files and independently run the real linker fixture. These tests do not start SurfaceFlinger, change live properties or modify installed modules. Samsung binaries and app hook scope are unchanged from v1.5.0. Installation/full-boot and live UI confirmation remain pending.

## v1.6.2 AOD clock transition

The donor AOD configuration contains aodversion=7,clocktransition,coverboldfont; the target system configuration contained only aodversion=7. Static DEX code inspection found FRAMEWORK_CONFIG_AOD_ITEM and clocktransition constants in the installed SystemUI LsRune static initializer and AOD configuration initializer (obfuscated class aod.ha6). This update adds only clocktransition, preserving aodversion=7, in both overlays. It does not copy coverboldfont or alter AOD brightness animation, refresh rate, fullscreen support, launcher animation type, AI flags, native code or recovery. Enable AOD in Settings; actual animation behavior depends on the selected clock and app settings and has not been visually checked for this update.

## v1.6.3 fullscreen AOD integration

Sets SEC_FLOATING_FEATURE_LCD_CONFIG_AOD_FULLSCREEN=1 in both existing floating-feature overlays, matching the pinned Samsung donor value. The standalone AOD FullScreen Enabler by GueRapii and Gustavoppw uses the same feature flag: https://github.com/GueRapii/AODFullScreen-Enabler/tree/096a4c92085737755f2e21edbe32bfc2c6072547 . Its installer and license were inspected; no upstream script/code is bundled or adapted. The existing donor-based XML is independently configured. Its module ID fullaodenable is disabled in both active and staged directories at install to prevent competing overlays. No separate AOD APK, boot script, brightness override or refresh-rate override is added. Native blur and recovery remain unchanged. Fullscreen wallpaper rendering and battery consumption require on-device confirmation.

## v1.6.4 AOD wallpaper switch/dimming prerequisite

The wallpaper setting was 0, yet wallpaper remained visible with both a custom wallpaper and Samsung ImageWallpaper. Installed SystemUI reported supportAODLightReveal=false. DeviceType.isSupportLightReveal reads framework dimen physical_power_button_center_screen_location_y (0x010503a4) and rejects 0; framework-res.apk and live overlay lookup both returned 0px. LsRune.AOD_LIGHT_REVEAL also requires clocktransition support, already enabled in the module. AODAmbientWallpaperHelper.getAlpha returns 1 for the wallpaper-off path and approximately 0.6 for ordinary fullscreen wallpaper-on mode, using SettingsHelper.isAODShowLockWallpaper. Thus fullscreen capability alone omitted a required drawing-layer prerequisite.

A static resource-only overlay, dev.a57.liveblur.aodlightreveal, supplies 1170px for that single dimension: a fallback midpoint for the pinned 1080×2340 screen, not a measured hardware-button location. It may affect the visual origin of power-button reveal effects. It carries no DEX, components, requested permissions or settings writes. Brightness, refresh rate, fullscreen feature XMLs, native code and recovery remain unchanged. Disabling/removing the module and rebooting removes the overlay. Source is in overlay/aod-light-reveal and the builder is tools/build-aod-overlay.sh. The local signing keystore remains in ignored build output.

Android idmap2 successfully mapped this resource under product policy without ignore-overlayable; lookup resolved the nonzero dimension, and APK signature verification passed. These are static/resource integration checks, not a visual test of AOD. A full reboot after user installation is required to reload the cached SystemUI gate. Action displays the wallpaper preference and resolved drawing coordinate for diagnosis.

## v1.6.5 bounded transition refresh requests

Samsung PluginAODManager imposes 60Hz limits while entering AOD and 30Hz during doze. This update adds a separate system-UID listener using the existing Samsung acquireRefreshRateMinLimitToken API. It requests the highest supported rate on interactive-state changes, detects those changes via display callbacks with a 100ms non-wakeup polling fallback, and releases each vote after 1500ms. It reads the user refresh-rate preference via an external Settings provider handle because an unmanaged app_process cannot use an ordinary app ContentResolver. It skips boosting for 60Hz mode, power saving, or an unreadable preference. It never writes settings.

A partial wake lock bounded to 1800ms keeps the 1500ms release callback running through the transition; no wake lock is held during steady AOD. Binder ownership also removes tokens when the helper process dies. The listener is an additional app_process runtime after boot; it consumes memory and briefly increases power use during transitions. It drops to UID 1000 before accessing framework services, has a firmware gate, and is not restarted in a failure loop. The launcher waits up to 180 seconds for boot completion and refuses duplicate processes. A refresh-disabled marker disables only this listener on the next reboot.

The native display-vote probe changed the requested physical range to 120–120Hz while a synthetic 60Hz max vote existed, and SurfaceFlinger reported the 120Hz physical mode. The headless test process still reported 60Hz and the global render rate was 60Hz in that test: panel mode is distinct from delivered UI frames. The actual listener self-test successfully registered display callbacks and released both the vote and wake guard after 1500ms. These tests establish API/lifetime behavior, not improved SystemUI animation FPS. The previous dimming resource, AOD feature XMLs, native renderer and recovery are unchanged. Private traces/logs are not part of the repository.

## v1.6.6 SystemUI frame pacing and AOD clock state

The panel-only refresh helper did not resolve reported stutter. This update additionally requests the supported high rate with View.setRequestedFrameRate on SystemUI window roots around screen/AOD transitions. It retains each original value (including NaN/no preference), restores it after 1500ms, and leaves an intervening owner request alone. Roots are invalidated to deliver both the request and its removal. A bounded partial wake guard keeps the release callback running; 60Hz mode and power saving skip the request. Delivered FPS is still limited by UI work and other system policies.

The zero-opacity glass-clock issue was reproduced mathematically in the actual installed Samsung AOD APK: versionCode 896703000 / 8.9.67.3, SHA256 430e33b3e0a7a7f7502e07dcb3bc267807135440b8f6a11ca8ae36e5420f10d2. Using a constructor-free, unattached AnalogClockView test object, the clock color method returned 0x00ffffff with stale glass enabled and editor opacity 0, versus 0xffffffff with AOD glass disabled. This supports correcting transient widget state rather than changing the saved opacity.

The helper checks that AOD app version and the expected AbsClockView schema: public setDozeAmount/setBlurEffectEnabled APIs plus Boolean u and float t0 state fields. It saves the transient glass setting, disables glass for AOD, and selects the completed doze endpoint only while non-interactive with AOD enabled. It skips hidden clocks, already-correct native state, and an active clock internal animation. Bounded delayed rechecks cover late attachment. On wake it restores normal doze/glass state only if the widget still retains the state applied by this helper, preserving a subsequent native-owner update. No editor preference, palette opacity field, settings value or database is written. The scan has a 2048-node budget. Unsupported AOD app versions/schema skip the clock correction.

The existing Zygisk bridge reads the small helper DEX only for com.android.systemui before specialization, then loads it in memory with the app class loader on its first eligible blur callback. Bootstrap failures clear helper exceptions and retain native blur. No extra Java/ART method hooks, SystemUI APK replacement, LSPosed module or additional root daemon are added. The existing v1.6.5 panel helper remains. clock-aod-disabled skips this new UI helper on the next reboot.

Tests cover Samsung color calculation independently, Android-view selection/state restoration and animation exclusion, actual View pacing acceptance/restoration, and production JNI in-memory DEX bootstrap in a separate runtime. Native pixel/scope fixtures remain distinct from real SystemUI testing. None of these tests establish delivered 120fps or verify the visible home-to-AOD path; both require on-device confirmation after user installation. Private traces, phone data and signing keys are excluded from source.
