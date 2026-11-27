Samsung Native Live Blur v1.6.6
Requires the supported SM-A576B firmware, KernelSU and Zygisk.
Install through KernelSU and fully reboot.
Edit excluded-apps.txt as root to exclude apps from the JNI bridge and donor libraries. Package entries include colon-named subprocesses. Restart an app after changing its entry. Duck Detector is excluded by default.
The renderer and crash recovery are unchanged from v1.5.1.
Action shows status. Disable/remove the module and reboot to undo it.
Blur-only release: no AI APKs, AI feature overrides or HighEnd animation override are included.
Enables the built-in AOD-to-lockscreen clock transition with the clocktransition AOD feature token. Fullscreen AOD wallpaper is enabled; brightness and refresh-rate settings are unchanged. Enable Always On Display in Settings.
Includes fullscreen AOD in the same package. The installer disables fullaodenable if present to prevent competing XML overlays. Enable the wallpaper option in AOD Settings after reboot; battery use may increase.
Includes a static resource-only overlay for the missing AOD light-reveal drawing coordinate. The wallpaper switch remains controlled by Settings; no preference is forced. The fallback drawing origin is the screen midpoint (1170px), not a measured hardware-button coordinate.
Adds an animation-only refresh listener after boot. Temporary minimum-rate requests are released after 1.5s; the helper does not write settings or change dimming. A short partial wake guard keeps the release deadline running. To disable only this helper, create refresh-disabled in the module directory and reboot.
SystemUI-only in-memory helper requests View-level pacing for 1.5s during AOD/screen transitions and restores the previous value. Clock glass-state correction requires AOD 8.9.67.3, skips an active internal clock animation, and never edits lock-screen opacity. Create clock-aod-disabled in the module directory and reboot to disable this UI helper only.
