#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-model-root-profile.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_model_root_profile_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
awk '/PIONEER_NATIVE_Q8_WORK_SUPPORT_BEGIN/ {copying=1;next} /PIONEER_NATIVE_Q8_WORK_SUPPORT_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$directory/../PioneerQ38WorkSession.h" > "$temporary/model_root_support.inc"
awk '/PIONEER_MODEL_ROOT_ARGUMENT_GUARD_BEGIN/ {copying=1;next} /PIONEER_MODEL_ROOT_ARGUMENT_GUARD_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$directory/../pioneer_q38_inference.cpp" > "$temporary/model_root_arguments.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$temporary" \
 "$directory/pioneer_q38_model_root_support_test.cpp" -o "$temporary/support"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/support"
python3 "$directory/pioneer_q38_model_root_build_gate_test.py"
printf 'Offline model root CPU profile evidence retained at %s;NO device access\n' "$temporary"
