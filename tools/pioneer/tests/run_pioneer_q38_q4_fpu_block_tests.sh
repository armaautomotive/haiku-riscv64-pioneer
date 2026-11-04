#!/bin/sh
# CPU-only data/emitted/compiled instruction tests,NEVER native/card access.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q4-fpu-block.XXXXXXXX")
for source in data pipeline image;do
 case "$source" in
  data) file=pioneer_q38_q4_fpu_block_data_test.cpp;;
  pipeline) file=pioneer_q38_q4_fpu_pipeline_test.cpp;;
  image) file=pioneer_q38_q4_fpu_block_image_test.cpp;;
 esac
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
  -fsanitize=address,undefined "$directory/$file" -o "$temporary/$source-test"
done
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/data-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pipeline-test"
python3 "$directory/../build_q38_q4_fpu_block.py" "$1" "$temporary/direct" --direct
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/image-test" "$temporary/direct" >"$temporary/image.log"
python3 "$directory/pioneer_q38_q4_fpu_pipeline_fraction_test.py" "$temporary/image.log"
tail -1 "$temporary/image.log"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if ${CXX:-c++} -std=c++17 -D"$role" -fsyntax-only "$directory/pioneer_q38_q4_fpu_pipeline_test.cpp" >"$temporary/$role.log" 2>&1;then
  printf 'ERROR:unqualified pipeline role compiled:%s\n' "$role" >&2
  exit 1
 fi
 rg -q 'Compressed Q4 FPU operand producer is not qualified' "$temporary/$role.log"
done
printf 'PASS Q4 pipeline compile gates:kernel/Work/validation refused;fixed8KiB/direct-only builder\n'
printf 'OFFBOARD Q4 pipeline build/tests retained: %s\n' "$temporary"
