#!/system/bin/sh
# Sourced in the renderer's mount namespace. Requires MODDIR, BB and hashes.
renderer_hash() {
    result=$(sha256sum "$1" 2>/dev/null) || return 1
    printf '%s\n' "${result%% *}"
}
restore_stock_renderer() {
    current=$(renderer_hash /system/bin/surfaceflinger)
    [ "$current" != "$STOCK_SHA256" ] || return 0
    [ "$current" = "$DONOR_SHA256" ] || { echo 'Refusing to replace unknown renderer'; return 1; }
    for attempt in 1 2 3; do
        umount /system/bin/surfaceflinger 2>/dev/null || break
        current=$(renderer_hash /system/bin/surfaceflinger)
        [ "$current" != "$STOCK_SHA256" ] || return 0
        [ "$current" = "$DONOR_SHA256" ] || break
    done
    # A bind may have been absorbed or hidden by the mount manager. A verified
    # stock-file bind provides an independent recovery path in that case.
    current=$(renderer_hash /system/bin/surfaceflinger)
    [ "$current" = "$DONOR_SHA256" ] || return 1
    backup="$MODDIR/backup/surfaceflinger.stock"
    [ "$(renderer_hash "$backup")" = "$STOCK_SHA256" ] || { echo 'Stock backup checksum failed'; return 1; }
    chmod 0755 "$backup" || return 1
    chcon u:object_r:surfaceflinger_exec:s0 "$backup" || return 1
    /system/bin/mount -o bind "$backup" /system/bin/surfaceflinger || return 1
    "$BB" mount -o remount,bind,ro,suid,dev,exec "$backup" /system/bin/surfaceflinger || return 1
    [ "$(renderer_hash /system/bin/surfaceflinger)" = "$STOCK_SHA256" ] || return 1
    echo 'Stock renderer restored through verified backup bind'
}
restart_stock_renderer_if_needed() {
    sf_pid=$(pidof surfaceflinger)
    if [ -n "$sf_pid" ] && [ "$(renderer_hash "/proc/$sf_pid/exe")" = "$STOCK_SHA256" ]; then
        echo 'Stock renderer is already running; no restart needed'
    else
        echo 'Original renderer restored; restarting display service'
        setprop ctl.restart surfaceflinger
    fi
}
