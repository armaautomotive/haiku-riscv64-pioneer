#!/bin/sh
# Build-only CPU source preflight ON Pioneer; never open P100A.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
stage=/boot/home/pioneer-qgwd-real-dense-band-20261110-a
tree=$stage/build-tree
out=$stage/source-preflight-build-1
sdk=/boot/system/develop/headers
exact(){ test -f "$1" && test ! -L "$1"; sum=$(sha256sum "$1"); test "${sum%% *}" = "$2"; }
test "$(uname -s)" = Haiku && test "$(id -u)" = 0 && test -d "$sdk"
test ! -e "$out" && test ! -L "$out"
exact "$tree/tools/pioneer/pioneer_qgwd_dense_source_preflight.cpp" 6a7569c23d4bab06341e8a14065dcc81d2f243930952ea938f21cb66fd85c2f2
exact "$stage/private/PioneerNativeBfpModelBandMetadata.h" 161c882ef8557e9b745ab7bb1d1871329e73019b81b9a17dc4885c4172c48575
exact "$tree/libshared.a" 4845e2fb611f79642a3f7289e45dda4bbb1fa622cb1be71c9291131a94055586
set -- -I"$sdk" -idirafter "$sdk/posix" -I"$tree/headers/private/shared" -I"$tree/headers/private/libroot" -I"$stage/private"
for dir in "$sdk/os" "$sdk/os/"*; do if test -d "$dir"; then set -- "$@" -isystem "$dir"; fi; done
mkdir "$out"
g++ -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror "$@" -DPIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE=1 \
 "$tree/tools/pioneer/pioneer_qgwd_dense_source_preflight.cpp" "$tree/tools/pioneer/PioneerModelWeightCache.cpp" \
 "$tree/libshared.a" -lbe -o "$out/pioneer-qgwd-dense-source-preflight"
sha256sum "$out/pioneer-qgwd-dense-source-preflight"
echo 'BUILD_ONLY_QGWD_SOURCE_PREFLIGHT; NO MODEL, DEVICE OR SPEED CLAIM'
