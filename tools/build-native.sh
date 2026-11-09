#!/system/bin/sh
set -eu
cd "$(dirname "$0")/.."
CXX=${CXX:-clang++}
SYSTEM_LIBDIR=${SYSTEM_LIBDIR:-/system/lib64}
mkdir -p build module/zygisk
"$CXX" -std=c++17 -O2 -shared -fPIC -nostdlib++ -fno-exceptions -fno-rtti module/source/donor_guards.cpp -Wl,--no-as-needed module/donor/libblur_donor.so "$SYSTEM_LIBDIR/liblog.so" -Wl,-soname,libblur_loader.so,-rpath,'$ORIGIN',-z,defs,-z,max-page-size=16384 -o build/libblur_loader.so
node tools/clean_loader_paths.js build/libblur_loader.so
"$CXX" -std=c++17 -O2 -shared -fPIC -fvisibility=hidden -fno-exceptions -fno-rtti -fno-threadsafe-statics -nostdlib++ -Wall -Wextra -Werror -Imodule/source module/source/native_donor.cpp "$SYSTEM_LIBDIR/liblog.so" -Wl,-z,defs,-z,max-page-size=16384 -o build/arm64-v8a.so
node tools/clean_elf.js build/arm64-v8a.so
cp build/libblur_loader.so module/donor/libblur_loader.so
cp build/arm64-v8a.so module/zygisk/arm64-v8a.so
