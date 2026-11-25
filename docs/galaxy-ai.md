# Donor features and compatibility

## Regular v1.6.0 build

Selects `SEC_FLOATING_FEATURE_LAUNCHER_CONFIG_ANIMATION_TYPE=HighEnd` in both existing floating-feature overlays, matching the SM-S942B S942BXXU5BZJA donor. The installed A57 One UI Home APK contains that feature key and HighEnd mode. No launcher APK, animation scale, AOD, camera setting, audio driver or framework is replaced.

This selects an existing launcher path. It does not establish that every transition matches the flagship: other effects can depend on SystemUI, framework, launcher version, hardware and display support. The native blur module, per-app exclusions and renderer recovery remain unchanged.

## Optional AI preview

`Samsung-Native-Live-Blur-v1.6.0-AI-Preview.zip` is a separate, experimental variant of the complete module. It uses the same `enable_live_blur` module ID and replaces the regular build. Choose one ZIP; do not install both as separate modules.

It adds the original donor Interpreter 1.2.15.14 APK (`com.samsung.android.app.interpreter`) and its original privileged/signature permission XMLs. It sets Interpreter audio configuration to `1`, native-AI disable to `FALSE`, and AI version to donor value `20263`. Those global gates may expose unavailable AI features in other installed apps; this is why the preview is separate from the regular build.

No Samsung AI Core, offline language-model stub, camera changes, model spoofing or SELinux changes are included. The APK is unmodified and remains Samsung-signed. Microphone permission and any required language downloads still belong to the app's normal setup flow. The package does not grant runtime microphone access or accept Samsung terms automatically.

After installation and a full reboot, check for Interpreter in Quick Settings or Settings. Speech, language downloads, service compatibility, memory use and effects on other AI-dependent apps have not been tested on this phone. Static checks do not establish that translation will work. If the app fails or unrelated AI menus misbehave, reinstall the regular ZIP and fully reboot. If needed, disable the whole module in KernelSU and reboot. Downloaded app data is not automatically deleted.

## Findings

| Component | Evidence | Decision |
| --- | --- | --- |
| HighEnd launcher animations | Donor XML specifies HighEnd; target launcher contains the key and mode | Included in regular build |
| Interpreter | Target has Samsung voice-intelligence and core services; platform certificate matches; all eight APK native libraries pass dependency/strong-symbol checks | Optional preview only |
| Samsung AI Core | Donor embeds build flavor `slsi_s5e9965`; target is `s5e8865` and does not advertise `android.hardware.npu`, which the APK declares | Omitted; no hardware-feature spoofing |
| Offline Language Model | Donor contains a stub APK, not the actual 3B model | Omitted; no 3B or LLM flags enabled |
| Photo AI flags | Target already enables object/shadow/reflection eraser and several other photo flags | Existing settings retained |
| AOD brightness animation | Donor setting differs, but target display compatibility is not established | Unchanged |
| Camera AI/portrait flags | Donor camera and ISP hardware differ | Unchanged |

Samsung notes that Galaxy AI availability depends on device/software, and some features require a Samsung account or network access: [Samsung support](https://www.samsung.com/us/support/answer/ANS10000753/).

## Reproduce the preview

Obtain these unchanged files from the pinned donor's system partition:

- `system/priv-app/BixbyInterpreter/BixbyInterpreter.apk`
- `system/etc/permissions/privapp-permissions-com.samsung.android.app.interpreter.xml`
- `system/etc/permissions/signature-permissions-com.samsung.android.app.interpreter.xml`

Place the three files in one local directory and run:

```sh
node tools/package.js
node tools/build-ai-preview.js /path/to/donor-files
```

The builder verifies all three SHA-256 hashes, copies the regular payload into a temporary build directory, and applies only the preview additions there. The production `module/` tree is not modified. Outputs are in `dist/`; no APK is installed and no service is restarted. The preview's `AI_PREVIEW.json` records hashes and feature changes.
