Samsung Native Live Blur v1.6.2
Requires the supported SM-A576B firmware, KernelSU and Zygisk.
Install through KernelSU and fully reboot.
Edit excluded-apps.txt as root to exclude apps from the JNI bridge and donor libraries. Package entries include colon-named subprocesses. Restart an app after changing its entry. Duck Detector is excluded by default.
The renderer and crash recovery are unchanged from v1.5.1.
Action shows status. Disable/remove the module and reboot to undo it.
Blur-only release: no AI APKs, AI feature overrides or HighEnd animation override are included.
Enables the built-in AOD-to-lockscreen clock transition with the clocktransition AOD feature token. Display-specific brightness/fullscreen/refresh-rate settings are unchanged. Enable Always On Display in Settings.
