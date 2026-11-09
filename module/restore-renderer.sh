#!/system/bin/sh
umask 077
MODDIR=${0%/*}
BB=/data/adb/ksu/bin/busybox
. "$MODDIR/renderer.conf" || exit 1
if [ "${1:-}" != --init-namespace ]; then
    exec "$BB" nsenter -F -t 1 -m /system/bin/sh "$0" --init-namespace
fi
. "$MODDIR/renderer-common.sh" || exit 1
touch "$MODDIR/renderer-disabled"
if restore_stock_renderer; then
    rm -f "$MODDIR/trial-pending"
    restart_stock_renderer_if_needed
else
    echo 'Renderer component disabled. Full reboot required to restore stock.'
fi
