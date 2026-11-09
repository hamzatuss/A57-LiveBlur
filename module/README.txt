Samsung Native Live Blur v1.5.0
Target build: samsung/a57xnaeea/a57x:17/CP2A.260605.016/A576BXXS6BZJ2_OXM6BZJ2:user/release-keys
Payload: JNI bridge, donor libraries, feature overlays, and original SM-S942B SurfaceFlinger. Samsung donor and renderer binaries are unchanged; the JNI module covers eligible app processes.
Install manually through KernelSU, then reboot. Requires working Zygisk Next.
Uses enable_live_blur ID and updates that module. On installation, automatically disables the old native_blur_renderer ID to prevent duplicate mounts.
No skip_mount marker: feature XML overlays are retained. Renderer is outside system/ and uses its guarded read-only bind mount.
Renderer failures create renderer-disabled and restore stock; JNI bridge remains enabled. An unfinished previous trial also skips the renderer.
KernelSU Action displays status only. Explicit restore-renderer.sh restores stock and disables only the renderer. Disabling/removing this module and rebooting undoes all module changes.
Renderer status: /data/adb/modules/enable_live_blur/renderer-status.log
v1.4.1 accepts the verified flagship renderer when already mounted, resets stale retry markers during installation, and distinguishes same-boot monitoring from a failed previous boot.
After installing, perform a full phone reboot; restarting only Android services does not rerun KernelSU boot scripts.
v1.4.2 hooks the exact system:ui process used by Open with, plus com.android.intentresolver for the separate chooser. Ordinary apps and system_server remain excluded.
v1.4.3 selects debug.renderengine.backend=skiaglthreaded before starting the donor renderer. This avoids the direct Skia Vulkan startup path that failed on this phone.
The watchdog now reacts to the first restarting state or confirmed donor exit, uses nsenter -F for correct PID tracking, and has a verified stock-backup bind fallback.
These paths passed isolated tests. A real reboot with this update still needs confirmation.
v1.5.0 replaces the app allowlist with app-wide coverage, including Samsung apps, third-party apps and subprocesses. system_server, the core system process and child zygotes are excluded. Existing Zygisk exclusions still apply.
Donor libraries are mapped during specialization; donor JNI caches initialize only on the first extended-blur call, with initialization serialized across threads. Apps and framework widgets control where blur is requested.
