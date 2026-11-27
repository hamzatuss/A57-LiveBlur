#!/system/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${ANDROID_JAR:?Set ANDROID_JAR to an Android SDK android.jar (API 35 or newer)}"
: "${D8_JAR:?Set D8_JAR to the SDK build-tools lib/d8.jar}"
mkdir -p build/aod-refresh-classes build/aod-refresh-dex module/refresh
javac -source 8 -target 8 -Xlint:-options -classpath "$ANDROID_JAR" -d build/aod-refresh-classes module/source/aod-refresh/AodRefresh.java
java -cp "$D8_JAR" com.android.tools.r8.D8 --lib "$ANDROID_JAR" --min-api 34 --output build/aod-refresh-dex build/aod-refresh-classes/dev/a57/liveblur/*.class
cp build/aod-refresh-dex/classes.dex module/refresh/aod-refresh.dex
