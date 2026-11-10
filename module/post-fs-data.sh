#!/system/bin/sh
umask 077
MODDIR=${0%/*}
BB=/data/adb/ksu/bin/busybox
. "$MODDIR/renderer.conf" || exit 1
status() { echo "$*" >> "$MODDIR/renderer-status.log"; }
[ -e "$MODDIR/renderer-disabled" ] && { status 'Renderer disabled; JNI bridge remains enabled'; exit 0; }
[ "$(getprop ro.build.fingerprint)" = "$EXPECTED_BUILD" ] || { status 'Skipped: firmware mismatch'; exit 0; }
# Match the GLES/ANGLE backend that worked on this target. Apply it before
# any donor bind so cold boot cannot choose the unsupported direct Vulkan path.
if ! setprop debug.renderengine.backend skiaglthreaded || [ "$(getprop debug.renderengine.backend)" != skiaglthreaded ]; then
    status 'Skipped: unable to select Skia GLES backend'
    exit 0
fi
status 'Requested RenderEngine backend: skiaglthreaded'
boot_id=$(cat /proc/sys/kernel/random/boot_id) || exit 1
if [ -e "$MODDIR/trial-pending" ] && [ "$(cat "$MODDIR/trial-pending")" != "$boot_id" ]; then
    status 'Previous boot trial did not complete; keeping stock renderer'
    touch "$MODDIR/renderer-disabled"
    exit 0
fi
actual=$(sha256sum "$MODDIR/renderer/surfaceflinger")
[ "${actual%% *}" = "$DONOR_SHA256" ] || { status 'Skipped: donor checksum mismatch'; exit 0; }
chcon u:object_r:surfaceflinger_exec:s0 "$MODDIR/renderer/surfaceflinger" || { status 'Skipped: executable label failed'; exit 0; }
mounted_hash() {
    mounted=$("$BB" nsenter -t 1 -m /system/bin/sha256sum /system/bin/surfaceflinger) || return 1
    printf '%s\n' "${mounted%% *}"
}
bind_renderer() {
    if ! "$BB" nsenter -t 1 -m /system/bin/mount -o bind "$MODDIR/renderer/surfaceflinger" /system/bin/surfaceflinger; then
        status 'Skipped: renderer bind failed'
        return 1
    fi
    if ! "$BB" nsenter -t 1 -m "$BB" mount -o remount,bind,ro,suid,dev,exec "$MODDIR/renderer/surfaceflinger" /system/bin/surfaceflinger; then
        "$BB" nsenter -t 1 -m /system/bin/umount /system/bin/surfaceflinger
        status 'Skipped: executable mount flags failed'
        return 1
    fi
    status 'Flagship renderer mounted read-only'
}
actual=$(mounted_hash) || { status 'Skipped: cannot read mounted renderer'; exit 0; }
case "$actual" in
    "$STOCK_SHA256") bind_renderer || exit 0 ;;
    "$DONOR_SHA256")
        # An update can unlink the inode behind an existing bind without
        # changing its bytes. realpath(/proc/self/exe) then fails in the linker,
        # losing the system/sphal namespaces required by Samsung's GPU driver.
        # Compare file identities inside init's namespace, not only hashes.
        source_id=$("$BB" nsenter -t 1 -m /system/bin/stat -Lc '%d:%i' "$MODDIR/renderer/surfaceflinger") || exit 0
        mounted_id=$("$BB" nsenter -t 1 -m /system/bin/stat -Lc '%d:%i' /system/bin/surfaceflinger) || exit 0
        if [ -n "$source_id" ] && [ "$mounted_id" = "$source_id" ]; then
            status 'Verified flagship renderer already mounted from current file; reusing it without another bind'
        else
            status 'Stale flagship mount detected after update; refreshing executable inode'
            attempts=0
            while [ "$actual" = "$DONOR_SHA256" ] && [ "$attempts" -lt 3 ]; do
                "$BB" nsenter -t 1 -m /system/bin/umount /system/bin/surfaceflinger || { status 'Skipped: cannot remove stale renderer mount'; exit 0; }
                attempts=$((attempts+1))
                actual=$(mounted_hash) || exit 0
            done
            [ "$actual" = "$STOCK_SHA256" ] || { status "Skipped: unexpected renderer below stale mount $actual"; exit 0; }
            bind_renderer || exit 0
        fi
        ;;
    *)
        status "Skipped: unknown mounted renderer hash $actual"
        exit 0
        ;;
esac
if [ -f "$MODDIR/renderer-watch.pid" ]; then
    watch_pid=$(cat "$MODDIR/renderer-watch.pid")
    case "$watch_pid" in
        ''|*[!0-9]*) ;;
        *)
            if kill -0 "$watch_pid" 2>/dev/null; then
                watch_command=$(tr '\000' ' ' < "/proc/$watch_pid/cmdline" 2>/dev/null)
                case "$watch_command" in
                    *"$MODDIR/renderer-watch.sh"*) status 'Boot trial is already being monitored'; exit 0 ;;
                esac
            fi
            ;;
    esac
fi
printf '%s\n' "$boot_id" > "$MODDIR/trial-pending"
"$BB" nsenter -F -t 1 -m /system/bin/sh "$MODDIR/renderer-watch.sh" "$MODDIR" >> "$MODDIR/renderer-status.log" 2>&1 < /dev/null &
printf '%s\n' "$!" > "$MODDIR/renderer-watch.pid"
status 'Boot trial started; running process must use the flagship executable'
