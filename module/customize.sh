#!/system/bin/sh
[ "$(getprop ro.build.fingerprint)" = 'samsung/a57xnaeea/a57x:17/CP2A.260605.016/A576BXXS6BZJ2_OXM6BZJ2:user/release-keys' ] || abort 'Firmware mismatch'
[ "$(getprop ro.product.cpu.abi)" = arm64-v8a ] || abort 'Requires arm64-v8a'
actual=$(sha256sum /system/bin/surfaceflinger)
case "${actual%% *}" in 5bee19df69e0f2b9cb32dce7fd02c37857c01cd7c328e2f5a9daab04402e1dad|103c5493947529c46bd46ad7e164c581ed39095ab24aee973e3d87b8932842dc) ;; *) abort 'Unexpected current renderer' ;; esac
(cd "$MODPATH" && sha256sum -c SHA256SUMS) || abort 'Module checksum failed'
set_perm_recursive "$MODPATH" 0 0 0755 0644
for script in post-fs-data.sh renderer-watch.sh action.sh restore-renderer.sh; do set_perm "$MODPATH/$script" 0 0 0755; done
set_perm "$MODPATH/renderer/surfaceflinger" 0 0 0755 u:object_r:surfaceflinger_exec:s0
for old in /data/adb/modules/native_blur_renderer /data/adb/modules_update/native_blur_renderer; do
    if [ -d "$old" ]; then touch "$old/disable" || abort 'Cannot disable old standalone renderer'; fi
done
rm -f "$MODPATH/renderer-disabled" "$MODPATH/trial-pending" "$MODPATH/renderer-watch.pid"
ui_print 'Samsung native JNI bridge, feature flags and original renderer'
ui_print 'Updates enable_live_blur; standalone native_blur_renderer is disabled'
ui_print 'Renderer rollback preserves the JNI bridge. Reboot required.'
