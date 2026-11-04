#!/bin/sh
# New owner plus actual fresh RV32 images. NO installation or GPU access.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-bfp-band-owner.XXXXXXXX")
for width in 5120 6144 17408;do
 python3 "$directory/../build_native_bfp_band.py" "$1" "$temporary/k$width" "$width" 4
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_native_bfp_band_owner_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/k5120" "$temporary/k6144" "$temporary/k17408"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if printf '#include "%s/PioneerNativeBfpBandLease.h"\n' "$directory/.." | \
  ${CXX:-c++} -std=c++17 -DB_DEVICE_OP_CODES_END=10000 -D"$selection" -x c++ -c - -o "$temporary/forbidden-$selection.o" 2> "$temporary/forbidden-$selection.stderr";then exit 1;fi
 rg -q 'Full-K band lease is an offboard owner prototype' "$temporary/forbidden-$selection.stderr"
done
printf 'PASS full-K owner build guards:driver/Work/validation refused;CPU-only evidence retained at %s\n' "$temporary"
