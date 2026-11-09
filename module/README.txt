Samsung Native Live Blur v1.4
Target build: samsung/a57xnaeea/a57x:17/CP2A.260605.016/A576BXXS6BZJ2_OXM6BZJ2:user/release-keys
Payload: JNI bridge, donor libraries, feature overlays, and original SM-S942B SurfaceFlinger. Native binaries are unchanged from the working setup.
Install manually through KernelSU, then reboot. Requires working Zygisk Next.
Uses enable_live_blur ID and updates that module. On installation, automatically disables the old native_blur_renderer ID to prevent duplicate mounts.
No skip_mount marker: feature XML overlays are retained. Renderer is outside system/ and uses its guarded read-only bind mount.
Renderer failures create renderer-disabled and restore stock; JNI bridge remains enabled. An unfinished previous trial also skips the renderer.
KernelSU Action restores only the stock renderer; disabling/removing this module and rebooting undoes all module changes.
Renderer status: /data/adb/modules/enable_live_blur/renderer-status.log
