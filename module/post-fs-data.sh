#!/system/bin/sh
umask 077
MODDIR=${0%/*}
BB=/data/adb/ksu/bin/busybox
. "$MODDIR/renderer.conf" || exit 1
status() { echo "$*" >> "$MODDIR/renderer-status.log"; }
[ -e "$MODDIR/renderer-disabled" ] && { status 'Renderer disabled; JNI bridge remains enabled'; exit 0; }
[ "$(getprop ro.build.fingerprint)" = "$EXPECTED_BUILD" ] || { status 'Skipped: firmware mismatch'; exit 0; }
if [ -e "$MODDIR/trial-pending" ]; then
    status 'Previous trial did not complete; keeping stock renderer'
    touch "$MODDIR/renderer-disabled"
    exit 0
fi
actual=$("$BB" nsenter -t 1 -m /system/bin/sha256sum /system/bin/surfaceflinger)
[ "${actual%% *}" = "$STOCK_SHA256" ] || { status 'Skipped: original renderer differs'; exit 0; }
actual=$(sha256sum "$MODDIR/renderer/surfaceflinger")
[ "${actual%% *}" = "$DONOR_SHA256" ] || { status 'Skipped: donor checksum mismatch'; exit 0; }
chcon u:object_r:surfaceflinger_exec:s0 "$MODDIR/renderer/surfaceflinger" || { status 'Skipped: executable label failed'; exit 0; }
touch "$MODDIR/trial-pending"
if ! "$BB" nsenter -t 1 -m /system/bin/mount -o bind "$MODDIR/renderer/surfaceflinger" /system/bin/surfaceflinger; then
    status 'Skipped: private renderer bind failed'
    rm -f "$MODDIR/trial-pending"
    exit 0
fi
if ! "$BB" nsenter -t 1 -m "$BB" mount -o remount,bind,ro,suid,dev,exec "$MODDIR/renderer/surfaceflinger" /system/bin/surfaceflinger; then
    "$BB" nsenter -t 1 -m /system/bin/umount /system/bin/surfaceflinger
    status 'Skipped: executable mount flags failed'
    rm -f "$MODDIR/trial-pending"
    exit 0
fi
status 'Flagship renderer mounted read-only; boot trial started'
"$BB" nsenter -t 1 -m /system/bin/sh "$MODDIR/renderer-watch.sh" "$MODDIR" >> "$MODDIR/renderer-status.log" 2>&1 < /dev/null &
