#!/bin/sh
# CPU-only sanitizer/reference test, optionally reading public Qwen fixtures.
set -eu
attention_test_root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
attention_test_dir=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-attention-math.XXXXXXXX")
${CXX:-c++} -std=c++11 -O1 -g -ffp-contract=off -Wall -Wextra -Werror \
 -fsanitize=address,undefined "$attention_test_root/pioneer_attention_math_test.cpp" \
 -o "$attention_test_dir/test"
UBSAN_OPTIONS=halt_on_error=1 "$attention_test_dir/test" "$@"
printf 'CPU-only attention test retained at %s; no device was accessed\n' "$attention_test_dir"
