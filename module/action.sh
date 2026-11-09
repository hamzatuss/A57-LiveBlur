#!/system/bin/sh
MODDIR=${0%/*}
BB=/data/adb/ksu/bin/busybox
. "$MODDIR/renderer.conf" || exit 1
echo 'Samsung Native Live Blur — status'
if [ -e "$MODDIR/renderer-disabled" ]; then echo 'Renderer disabled marker: present'; else echo 'Renderer disabled marker: absent'; fi
actual=$("$BB" nsenter -t 1 -m /system/bin/sha256sum /system/bin/surfaceflinger)
case "${actual%% *}" in
    "$DONOR_SHA256") echo 'Mounted renderer: Samsung flagship' ;;
    "$STOCK_SHA256") echo 'Mounted renderer: stock A57 (no Samsung color curve)' ;;
    *) echo "Mounted renderer: unknown (${actual%% *})" ;;
esac
sf_pid=$(pidof surfaceflinger)
if [ -n "$sf_pid" ]; then
    actual=$(sha256sum "/proc/$sf_pid/exe" 2>/dev/null)
    case "${actual%% *}" in
        "$DONOR_SHA256") echo 'Running renderer: Samsung flagship' ;;
        "$STOCK_SHA256") echo 'Running renderer: stock A57' ;;
        *) echo 'Running renderer: unavailable or unknown' ;;
    esac
fi
[ ! -f "$MODDIR/renderer-status.log" ] || tail -n 12 "$MODDIR/renderer-status.log"
echo 'Status only. Use a full phone reboot after installing an update.'
