#!/bin/sh
# CPU-only actual templated loader;never opens a device or modifies cache.
set -eu
[ "$#" = 0 ] || { [ "$#" = 1 ] && { [ "$1" = --alignment64-control ] || [ "$1" = --alignment64-math ]; }; } || exit 2
variant=
if [ "$#" = 1 ];then
 if [ "$1" = --alignment64-control ];then variant=-DPIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64=1
 else variant=-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1;fi
fi
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-dram-band-loader.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined $variant \
 "$directory/pioneer_native_dram_band_loader_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if printf '#include "%s/PioneerNativeDramBandLoader.h"\n' "$directory/.." | \
  ${CXX:-c++} -std=c++17 $variant -D"$selection" -x c++ -c - -o "$temporary/refused-$selection.o" 2> "$temporary/refused-$selection.stderr";then exit 1;fi
 rg -q 'Unqualified card-side DRAM band loader is not driver/Work/validation' "$temporary/refused-$selection.stderr"
done
printf 'PASS DRAM loader3 forbidden production roles;OFFBOARD evidence: %s\n' "$temporary"
