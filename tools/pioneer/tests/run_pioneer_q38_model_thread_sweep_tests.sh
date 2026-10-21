#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-model-thread.XXXXXXXX")
for region in SWITCH ARGUMENT_GUARD;do
case "$region" in SWITCH) name=model_thread_switch;;*) name=model_thread_arguments;;esac
awk -v begin="PIONEER_MODEL_THREAD_${region}_BEGIN" -v end="PIONEER_MODEL_THREAD_${region}_END" '
 index($0,begin){copying=1;next} index($0,end){found=1;exit} copying{print} END{if(!found)exit 1}' "$directory/../pioneer_q38_inference.cpp" > "$temporary/$name.inc"
done
awk '/PIONEER_NATIVE_Q8_WORK_SUPPORT_BEGIN/{copying=1;next} /PIONEER_NATIVE_Q8_WORK_SUPPORT_END/{found=1;exit} copying{print} END{if(!found)exit 1}' "$directory/../PioneerQ38WorkSession.h" > "$temporary/model_thread_support.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$temporary" "$directory/pioneer_q38_model_thread_sweep_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
python3 "$directory/pioneer_q38_model_thread_sweep_build_gate_test.py"
for section in STOP REPEAT_LOOP;do
case "$section" in STOP) name=stop;;*) name=repeat_loop;;esac
awk -v begin="PIONEER_PUBLIC_${section}_BEGIN" -v end="PIONEER_PUBLIC_${section}_END" '
 index($0,begin){copying=1;next} index($0,end)&&copying{found=1;exit} copying{print} END{if(!found)exit 1}' "$directory/../pioneer_q38_inference.cpp" > "$temporary/pioneer_public_$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined -DPIONEER_Q38_TEST_THREAD_SWEEP -I"$temporary" "$directory/pioneer_q38_public_repeat_loop_test.cpp" -o "$temporary/loop"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/loop"
printf 'Offline actual thread-switch/support/argument evidence retained at %s;NO native access\n' "$temporary"
