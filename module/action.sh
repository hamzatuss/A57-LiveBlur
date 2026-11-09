#!/system/bin/sh
umask 077
MODDIR=${0%/*}
BB=/data/adb/ksu/bin/busybox
. "$MODDIR/renderer.conf" || exit 1
touch "$MODDIR/renderer-disabled"
actual=$("$BB" nsenter -t 1 -m /system/bin/sha256sum /system/bin/surfaceflinger)
if [ "${actual%% *}" = "$DONOR_SHA256" ]; then
    if "$BB" nsenter -t 1 -m /system/bin/umount /system/bin/surfaceflinger; then
        actual=$("$BB" nsenter -t 1 -m /system/bin/sha256sum /system/bin/surfaceflinger)
        if [ "${actual%% *}" = "$STOCK_SHA256" ]; then
            echo 'Original renderer restored. Restarting display service.'
            setprop ctl.restart surfaceflinger
            exit 0
        fi
    fi
fi
echo 'Renderer component disabled. Reboot to use the original renderer. JNI bridge stays enabled.'
