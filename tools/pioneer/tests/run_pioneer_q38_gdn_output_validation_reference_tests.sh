#!/bin/sh
# CPU-only reference qualification;optional original authenticated GGUF.
set -eu
[ "$#" -le 1 ] || exit 2
if [ "$#" = 1 ];then [ -f "$1" ] || exit 2;fi
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-gdn-output-reference.XXXXXXXX)
python3 "$here/pioneer_q38_gdn_output_validation_reference_gates_test.py"
for name in reference reference_actual_model;do
 if [ "$name" = reference_actual_model ] && [ "$#" = 0 ];then continue;fi
 "${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  "$here/pioneer_q38_gdn_output_validation_${name}_test.cpp" -o "$build/$name"
 if [ "$name" = reference ];then
  ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/$name"
 else
  ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/$name" "$1"
 fi
done
printf 'CPU-only gated-output reference evidence retained at %s;NO device access\n' "$build"
