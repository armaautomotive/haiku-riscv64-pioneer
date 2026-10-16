#!/bin/sh
# Offline only: the assembled kernel runs in a strict host interpreter.
set -eu
[ "$#" -ge 1 ] || { echo 'Usage: run_pioneer_attention_image_tests.sh IMAGE_OUTPUT [PUBLIC_FIXTURES...]' >&2; exit 2; }
attention_image_test_root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
attention_image_test_dir=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-attention-image-test.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror \
 -fsanitize=address,undefined "$attention_image_test_root/pioneer_attention_image_test.cpp" \
 -o "$attention_image_test_dir/test"
UBSAN_OPTIONS=halt_on_error=1 "$attention_image_test_dir/test" "$@"
printf 'Offline assembled-image test retained at %s; no device accessed\n' "$attention_image_test_dir"
