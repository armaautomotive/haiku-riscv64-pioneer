#!/bin/sh
# Build and execute ACTUAL RV32 ONLY in a CPU observer. NEVER deploys.
set -eu
[ "$#" = 1 ] || { [ "$#" = 2 ] && [ "$2" = --alignment64-math ]; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-dram-band-image.XXXXXXXX")
flags=
option=
if [ "$#" = 2 ];then flags=-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1;option=--alignment64-math;fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 $flags "$directory/pioneer_native_dram_band_image_test.cpp" -o "$temporary/test"
for width in 5120 6144 17408;do
 python3 "$directory/../build_native_dram_band.py" "$1" "$temporary/k$width" "$width" $option
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/k$width" "$width"
done
printf 'OFFBOARD actual direct-GDDR native image evidence: %s\n' "$temporary"
