#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-native-q8-model-arguments.XXXXXXXX")
awk '/PIONEER_NATIVE_Q8_MODEL_ARGUMENT_GUARD_BEGIN/ {copying=1;next} /PIONEER_NATIVE_Q8_MODEL_ARGUMENT_GUARD_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$directory/../pioneer_q38_inference.cpp" > "$temporary/native_q8_model_argument_guard.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_native_q8_model_arguments_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline actual native-Q8 public-model argument evidence retained at %s\n' "$temporary"
