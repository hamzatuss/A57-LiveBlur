#!/system/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${ANDROID_JAR:?Set ANDROID_JAR to an Android SDK android.jar}"
: "${D8_JAR:?Set D8_JAR to SDK build-tools lib/d8.jar}"
: "${AOD_APK:?Set AOD_APK to the installed Samsung AOD 8.9.67.3 APK path}"
mkdir -p build/clock-color-probe/classes build/clock-color-probe/dex
javac -source 8 -target 8 -Xlint:-options -classpath "$ANDROID_JAR" -d build/clock-color-probe/classes tools/clock-color-probe/ClockColorProbe.java
java -cp "$D8_JAR" com.android.tools.r8.D8 --lib "$ANDROID_JAR" --min-api 34 --output build/clock-color-probe/dex build/clock-color-probe/classes/*.class
# Test object is constructor-free and unattached. No live clock is accessed.
su -c "CLASSPATH='$PWD/build/clock-color-probe/dex/classes.dex' /system/bin/app_process /system/bin ClockColorProbe '$AOD_APK'"
