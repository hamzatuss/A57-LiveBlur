# Architecture and pinned inputs

## Target

SM-A576B, A576BXXS6BZJ2_OXM6BZJ2; the exact fingerprint is in `module/source/expected_build.hpp` and `module/renderer.conf`.

## JNI donor

Source release: https://github.com/enderbk0/a36toflagship/releases/tag/1.0.6

Original donor runtime SHA-256: `94426b4178088372552ecbb35905bd4ebfe3ce726cbe751690b72c618e1c3125`.

The release does not establish the original donor device/build. Only the SONAME was renamed in the supplied donor; executable sections were checked unchanged. The SurfaceControl registrar is pinned at `0x1724d0`; the extended-blur routine is at `0x2276cc`. The loader verifies the registrar offset and routine entry bytes.

The module temporarily substitutes the current thread's JNI RegisterNatives callback while invoking the donor registrar to initialize caches. It suppresses 154 unrelated registrations and restores the original JNI table. Only the extended-blur method is hooked in selected app processes; system_server and ordinary apps are excluded.

Zygisk API header: https://github.com/topjohnwu/zygisk-module-sample at commit `7bb941ac8edfcffd1d23761e401c45ca95409dc1`, API version 4.

## Renderer

Official Samsung firmware input: SM-S942B / EUX / S942BXXU5BZJA.
Archive: `SM-S942B_2_20261016025731_0iaygqx1ga_fac.zip.enc4`.

The firmware's Android package was range-streamed and decrypted; its super image was decoded and only `system_a` reconstructed. LP metadata checksums and the full Android package ZIP CRC/length passed. The 8.29 GiB system image was mounted read-only for file extraction.

SurfaceFlinger SHA-256: `103c5493947529c46bd46ad7e164c581ed39095ab24aee973e3d87b8932842dc`.
Stock A57 renderer SHA-256: `5bee19df69e0f2b9cb32dce7fd02c37857c01cd7c328e2f5a9daab04402e1dad`.

The donor linked against the existing target libraries with no unavailable libraries or missing strong symbols in the 92-library dependency check. Layer-state constructor field layout and five default vectors matched. These checks support this build-specific port; they do not establish cross-device compatibility.

Boot scripts perform a guarded bind mount and remount the file read-only, executable, with service-domain transitions allowed. A separate namespace test checked those mount flags. Rollback unmount was exercised privately with the display restart request intercepted, so the live display was not restarted during that test.

## Combined packaging

Module ID remains `enable_live_blur`. The installer disables the old standalone `native_blur_renderer` on flash. The combined payload has no root `skip_mount` marker, allowing its floating-feature XML overlays to load. Renderer rollback uses a component marker rather than disabling the combined module.

The GitHub Release ZIP is built from the combined module payload. Standalone module documentation is excluded. Repacking refreshes the checksum manifest; native binaries remain identical unless explicitly rebuilt.
