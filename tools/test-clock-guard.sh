#!/system/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${ANDROID_JAR:?Set ANDROID_JAR to an Android SDK android.jar (API 35+)}"
: "${D8_JAR:?Set D8_JAR to SDK build-tools lib/d8.jar}"
mkdir -p build/clock-guard-fixture-classes build/clock-guard-fixture-dex
javac -source 8 -target 8 -Xlint:-options -classpath "$ANDROID_JAR" -d build/clock-guard-fixture-classes module/source/clock-aod/ClockAodGuard.java tools/clock-guard-fixture/com/samsung/android/uniform/widget/clock/AbsClockView.java tools/clock-guard-fixture/dev/a57/liveblur/ClockGuardFixture.java
java -cp "$D8_JAR" com.android.tools.r8.D8 --lib "$ANDROID_JAR" --min-api 34 --output build/clock-guard-fixture-dex build/clock-guard-fixture-classes/dev/a57/liveblur/*.class build/clock-guard-fixture-classes/com/samsung/android/uniform/widget/clock/*.class
# Separate runtime; no activity is launched and no live SystemUI view is touched.
su -c "CLASSPATH='$PWD/build/clock-guard-fixture-dex/classes.dex' /system/bin/app_process /system/bin dev.a57.liveblur.ClockGuardFixture"
