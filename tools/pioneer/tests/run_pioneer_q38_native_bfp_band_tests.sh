#!/bin/sh
# Build+strict RV32I observer ONLY. NEVER installs/stages/launches on hardware.
set -eu
[ "$#" = 1 ] || exit 2
sdk=$1
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-bfp-band.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_native_bfp_band_image_test.cpp" -o "$temporary/test"
for width in 5120 6144 17408;do
 for phases in 1 4;do
  for issue in mmio direct;do
   output=$temporary/k$width-p$phases-$issue
   if [ "$issue" = direct ];then
    python3 "$directory/../build_native_bfp_band.py" "$1" "$output" "$width" "$phases" --direct
   else python3 "$directory/../build_native_bfp_band.py" "$1" "$output" "$width" "$phases";fi
   ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$output" "$width" "$phases" "$issue"
  done
 done
done
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if printf '#include "%s/PioneerQ38NativeBfpBand.h"\n' "$directory/.." | \
  ${CXX:-c++} -std=c++17 -D"$selection" -x c++ -c - -o "$temporary/forbidden-$selection.o" 2> "$temporary/forbidden-$selection.stderr";then exit 1;fi
 grep -q 'Native BFP band is an unqualified offboard prototype' "$temporary/forbidden-$selection.stderr"
done
if printf '#include "%s/PioneerQ38NativeBfpBand.h"\nstruct E{void Op(unsigned){} void Config(unsigned,unsigned){} void Drain(){}};void trial(){E e;PioneerQ38NativeBfpBand::Run<2>(e,5120);}\n' "$directory/.." | \
 ${CXX:-c++} -std=c++17 -x c++ -c - -o "$temporary/forbidden-phases.o" 2> "$temporary/forbidden-phases.stderr";then exit 1;fi
grep -q 'explicit separate precision variant required' "$temporary/forbidden-phases.stderr"
for dimensions in '6144 4' '5120 1';do
 set -- $dimensions
 rejected="$temporary/rejected-pair-work-$1-$2"
 if python3 "$directory/../build_native_bfp_band.py" "$sdk" "$rejected" "$1" "$2" --pair-overlap-work > /dev/null 2>&1;then exit 1;fi
 test ! -e "$rejected"
done
printf 'PASS native band build guards:3 driver/Work/validation selections and implicit2-phase variant refused\n'
printf 'OFFBOARD actual full-K native BFP band evidence: %s\n' "$temporary"
