#!/system/bin/sh
umask 077
MODDIR=$1
BB=/data/adb/ksu/bin/busybox
. "$MODDIR/renderer.conf" || exit 1
. "$MODDIR/renderer-common.sh" || exit 1
cleanup() {
    if [ -f "$MODDIR/renderer-watch.pid" ]; then
        IFS= read -r recorded_pid < "$MODDIR/renderer-watch.pid"
        [ "$recorded_pid" != "$$" ] || rm -f "$MODDIR/renderer-watch.pid"
    fi
}
trap cleanup EXIT
trap 'echo "Watch interrupted; unfinished trial retained for next boot"; exit 0' HUP INT TERM
rollback() {
    echo "Rollback: $*"
    touch "$MODDIR/renderer-disabled"
    if restore_stock_renderer; then
        rm -f "$MODDIR/trial-pending"
        restart_stock_renderer_if_needed
    else
        echo 'Stock restore failed; renderer is disabled for next boot'
    fi
    exit 0
}
last_pid=; last_hash=; donor_pid=; stable_ticks=0; ticks=0
while [ "$ticks" -lt 720 ]; do
    sleep 0.25
    ticks=$((ticks+1))
    [ ! -e "$MODDIR/renderer-disabled" ] || { echo 'Watch stopped: renderer explicitly disabled'; exit 0; }
    service_state=$(getprop init.svc.surfaceflinger)
    [ "$service_state" != restarting ] || rollback 'SurfaceFlinger entered restarting state'
    current_pid=$(pidof surfaceflinger)
    if [ -n "$donor_pid" ] && [ "$current_pid" != "$donor_pid" ]; then
        rollback 'Flagship renderer process exited or restarted'
    fi
    if [ -n "$current_pid" ]; then
        if [ "$current_pid" != "$last_pid" ]; then
            last_pid=$current_pid
            last_hash=$(renderer_hash "/proc/$current_pid/exe")
            stable_ticks=0
        fi
        if [ "$last_hash" = "$DONOR_SHA256" ]; then
            donor_pid=$current_pid
            stable_ticks=$((stable_ticks+1))
        else
            stable_ticks=0
        fi
    else
        stable_ticks=0
    fi
    [ "$ticks" -lt 120 ] || [ -n "$donor_pid" ] || rollback 'Flagship renderer did not activate within startup grace period'
    if [ "$stable_ticks" -ge 120 ] && [ "$(getprop sys.boot_completed)" = 1 ]; then
        rm -f "$MODDIR/trial-pending"
        echo "Boot trial passed: flagship PID $current_pid stable; requested backend $(getprop debug.renderengine.backend)"
        exit 0
    fi
done
rollback 'Boot completion deadline exceeded'
