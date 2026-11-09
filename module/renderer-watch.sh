#!/system/bin/sh
umask 077
MODDIR=$1
. "$MODDIR/renderer.conf" || exit 1
cleanup() {
    if [ -f "$MODDIR/renderer-watch.pid" ] && [ "$(cat "$MODDIR/renderer-watch.pid")" = "$" ]; then
        rm -f "$MODDIR/renderer-watch.pid"
    fi
}
trap cleanup EXIT
rollback() {
    echo "Rollback: $*"
    touch "$MODDIR/renderer-disabled"
    if umount /system/bin/surfaceflinger; then
        actual=$(sha256sum /system/bin/surfaceflinger)
        if [ "${actual%% *}" = "$STOCK_SHA256" ]; then
            echo 'Original renderer restored; restarting display service'
            setprop ctl.restart surfaceflinger
        else
            echo 'Unexpected original hash; leaving restart to init'
        fi
    else
        echo 'Unmount failed; renderer trial disabled for next boot'
    fi
    exit 0
}
last_pid=; changes=0; stable=0; elapsed=0; started=0; donor_seen=0
while [ "$elapsed" -lt 180 ]; do
    sleep 2
    elapsed=$((elapsed+2))
    [ ! -e "$MODDIR/renderer-disabled" ] || { echo 'Watch stopped: renderer explicitly disabled'; exit 0; }
    current_pid=$(pidof surfaceflinger)
    if [ -n "$current_pid" ]; then
        started=1
        if [ "$current_pid" != "$last_pid" ]; then
            [ -z "$last_pid" ] || changes=$((changes+1))
            last_pid=$current_pid
            actual=$(sha256sum "/proc/$current_pid/exe" 2>/dev/null)
            if [ "${actual%% *}" = "$DONOR_SHA256" ]; then donor_seen=1; else donor_seen=0; fi
            stable=0
        else
            stable=$((stable+2))
        fi
    else
        stable=0
    fi
    [ "$changes" -lt 3 ] || rollback 'Display service restarted repeatedly'
    if [ "$elapsed" -ge 40 ] && [ "$started" = 0 ]; then rollback 'Display service did not start'; fi
    if [ "$elapsed" -ge 40 ] && [ "$donor_seen" = 0 ]; then rollback 'Donor renderer did not activate'; fi
    if [ "$donor_seen" = 1 ] && [ "$stable" -ge 30 ] && [ "$(getprop sys.boot_completed)" = 1 ]; then
        rm -f "$MODDIR/trial-pending"
        echo "Boot trial passed: display PID $current_pid stable for $stable seconds"
        exit 0
    fi
done
rollback 'Boot completion deadline exceeded'
