#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
root=$(CDPATH= cd -- "$directory/.." && pwd)
sdk=${1:?usage: run_pioneer_q38_sfpu_exp_worker_tests.sh ABSOLUTE_HAIKU_SDK}
case "$sdk" in /*) ;; *) echo 'SDK path must be absolute' >&2; exit 2;; esac
temporary=$(mktemp -d /private/tmp/pioneer-q38-sfpu-worker.XXXXXXXX)
python3 "$root/build_bfp4_worker_leaf.py" "$sdk" "$temporary/leaf"
python3 "$root/build_q38_hybrid_matrix_attention_worker.py" "$sdk" "$temporary/worker" --sfpu-exp-inspection
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 -DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1 \
 -DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1 -DPIONEER_Q38_TEST_SFPU_EXP=1 \
 "$directory/pioneer_q38_hybrid_matrix_attention_transition_test.cpp" -o "$temporary/transition"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 \
 "$temporary/transition" "$temporary/worker" "$temporary/leaf"
"$directory/run_pioneer_sfpu_exp_tests.sh"
printf 'SFPU exp linked-worker inspection retained at %s; NO card installation, silicon timing, Qwen model-quality or TPS claim\n' "$temporary"
