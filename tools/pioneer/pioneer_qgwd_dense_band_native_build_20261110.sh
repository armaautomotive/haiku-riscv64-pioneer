#!/bin/sh
# Build-only real-Qwen fixed-band diagnostic ON the Pioneer. Never install,
# open the device, execute a driver, or move the private weights elsewhere.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
stage=/boot/home/pioneer-qgwd-real-dense-band-20261110-a
tree=$stage/build-tree
out=$stage/native-build-attempt-3
sdk=/boot/system/develop/headers
exact(){ test -f "$1" && test ! -L "$1"; sum=$(sha256sum "$1"); test "${sum%% *}" = "$2"; }
test "$(uname -s)" = Haiku && test "$(id -u)" = 0
test -d "$tree" && test ! -L "$tree" && test -d "$stage/private" && test ! -L "$stage/private"
test ! -e "$out" && test ! -L "$out"
test "$(/bin/cat "$stage/exit-status")" = 0 && test ! -s "$stage/stderr" && test ! -s "$stage/independent-stderr"
grep -Fx 'PASS_INDEPENDENT_DENSE_BAND samples=3 rows=96 phase_instructions=122880; NO DEVICE OR SPEED' "$stage/independent-stdout" >/dev/null
exact "$tree/pioneer-qgwd-source-stage-20261110.tar" cc5ef0d30fe6b11ad5aa7346c2735416e751a05ccef66d9fab93f914489372da
exact "$tree/PioneerNativeBfpBandGeneratedImage.h" 7234d47123244686c15b39666f8a6acf05085a80ccc162a3e31551a249d2d37b
exact "$tree/kernel.so" d3d7275b378faeef202e9eb9bf05729ef8af7841a4803885876ed9742fc9608a
exact "$tree/libshared.a" 4845e2fb611f79642a3f7289e45dda4bbb1fa622cb1be71c9291131a94055586
exact "$tree/src/add-ons/kernel/busses/pci/ecam/Sg2042RootPolicy.h" c79178e8fd666b1ea23a139cd26fb9d0618942d45f092343a411ab93db938d4e
exact "$stage/private/PioneerNativeBfpModelBandData.h" 3a7aced1e5cb5245105d14c2e8f74327cca708d1cfaf2d230fd5bf14fc165e25
exact "$stage/private/PioneerNativeBfpModelBandMetadata.h" 161c882ef8557e9b745ab7bb1d1871329e73019b81b9a17dc4885c4172c48575
test -d "$sdk" && test -f /boot/system/develop/lib/haiku_version_glue.o
mkdir "$out"
cd "$tree"
set -- -isystem "$tree/headers/private/kernel" -isystem "$tree/headers/private/kernel/arch/riscv64" \
 -isystem "$tree/headers/private/kernel/arch/generic" -isystem "$tree/headers/private" \
 -isystem "$tree/headers/private/drivers" -isystem "$tree/headers/private/system" \
 -isystem "$tree/headers/private/system/arch/riscv64" -isystem "$tree/headers/private/shared" \
 -isystem "$tree/build/config_headers" -I"$sdk" -idirafter "$sdk/posix" \
 -I"$tree" -I"$tree/tools/pioneer" -I"$stage/private"
for dir in "$sdk/os" "$sdk/os/"*; do if test -d "$dir"; then set -- "$@" -isystem "$dir"; fi; done
common='-std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fstack-usage -ffreestanding -finline -fno-semantic-interposition -fno-tree-vectorize -fno-use-cxa-atexit -fno-strict-aliasing -fno-delete-null-pointer-checks -mcmodel=medany -fpic -D_KERNEL_MODE'
native='-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_DRIVER_RELOAD -DPIONEER_DRIVER_BUILD_TAG=0x2026111000010140ULL -DPIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC=1 -DPIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC=1'
driver=$tree/src/add-ons/kernel/drivers/misc
g++ $common "$@" -c "$driver/PioneerPciProfile.cpp" -o "$out/profile.o"
g++ $common $native "$@" -c "$driver/pioneer_native_model_band_driver.cpp" -o "$out/native.o"
begin=$(g++ -print-file-name=crtbeginS.o); end=$(g++ -print-file-name=crtendS.o); gcc=$(g++ -print-file-name=libgcc.a)
test -f "$begin" && test -f "$end" && test -f "$gcc"
g++ -shared -nostdlib -Wl,--no-undefined,-soname=tenstorrent_pci_probe "$begin" \
 /boot/system/develop/lib/haiku_version_glue.o "$out/native.o" "$out/profile.o" "$tree/kernel.so" "$gcc" "$end" \
 -o "$out/tenstorrent_pci_probe-qgwd-dense-band140"
g++ -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror "$@" -I"$tree/headers/private/libroot" \
 -DPIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC=1 -DPIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE=1 \
 "$tree/tools/pioneer/pioneer_native_model_band_diagnostic.cpp" "$tree/tools/pioneer/PioneerModelWeightCache.cpp" \
 "$tree/libshared.a" -lbe -o "$out/pioneer-qgwd-dense-band-diagnostic140"
sha256sum "$out/tenstorrent_pci_probe-qgwd-dense-band140" "$out/pioneer-qgwd-dense-band-diagnostic140"
echo 'BUILD_ONLY_QGWD_DENSE_BAND140; NO INSTALL, DEVICE ACCESS OR SPEED CLAIM'
