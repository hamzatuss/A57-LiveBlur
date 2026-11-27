#!/system/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/aod-overlay module/system/product/overlay
key=build/aod-overlay/signing.jks
if [ ! -f "$key" ]; then
    keytool -genkeypair -keystore "$key" -alias aod-overlay -keyalg RSA -keysize 2048 -validity 10000 -dname 'CN=A57 LiveBlur Resource Overlay' -storepass a57-overlay-build -keypass a57-overlay-build -noprompt
fi
aapt2 compile --dir overlay/aod-light-reveal/res -o build/aod-overlay/resources.zip
aapt2 link -I /system/framework/framework-res.apk --manifest overlay/aod-light-reveal/AndroidManifest.xml --auto-add-overlay -o build/aod-overlay/unsigned.apk build/aod-overlay/resources.zip
apksigner sign --ks "$key" --ks-key-alias aod-overlay --ks-pass pass:a57-overlay-build --key-pass pass:a57-overlay-build --out module/system/product/overlay/A57AodLightReveal.apk build/aod-overlay/unsigned.apk
apksigner verify module/system/product/overlay/A57AodLightReveal.apk
# v4 signatures are not needed for a system resource overlay.
rm -f module/system/product/overlay/A57AodLightReveal.apk.idsig
