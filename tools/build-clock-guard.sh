#!/system/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${ANDROID_JAR:?Set ANDROID_JAR to an Android SDK android.jar (API 35+)}"
: "${D8_JAR:?Set D8_JAR to SDK build-tools lib/d8.jar}"
mkdir -p build/clock-guard-classes build/clock-guard-dex module/clock
javac -source 8 -target 8 -Xlint:-options -classpath "$ANDROID_JAR" -d build/clock-guard-classes module/source/clock-aod/ClockAodGuard.java
java -cp "$D8_JAR" com.android.tools.r8.D8 --lib "$ANDROID_JAR" --min-api 34 --output build/clock-guard-dex build/clock-guard-classes/dev/a57/liveblur/*.class
cp build/clock-guard-dex/classes.dex module/clock/clock-aod.dex
