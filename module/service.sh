#!/system/bin/sh
# Animation-only refresh helper. Never restarts Android or changes preferences.
MODDIR=${0%/*}
[ ! -e "$MODDIR/refresh-disabled" ] || exit 0
[ "$(getprop ro.build.fingerprint)" = 'samsung/a57xnaeea/a57x:17/CP2A.260605.016/A576BXXS6BZJ2_OXM6BZJ2:user/release-keys' ] || exit 0
i=0
while [ "$(getprop sys.boot_completed)" != 1 ]; do
    [ "$i" -lt 180 ] || exit 0
    sleep 1
    i=$((i+1))
done
[ ! -e "$MODDIR/disable" ] && [ ! -e "$MODDIR/refresh-disabled" ] || exit 0
[ -z "$(pidof a57-aod-refresh)" ] || exit 0
CLASSPATH="$MODDIR/refresh/aod-refresh.dex" /system/bin/app_process /system/bin --nice-name=a57-aod-refresh dev.a57.liveblur.AodRefresh >> "$MODDIR/refresh-status.log" 2>&1 < /dev/null &
