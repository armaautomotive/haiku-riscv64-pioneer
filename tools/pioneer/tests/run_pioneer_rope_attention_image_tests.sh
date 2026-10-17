#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-rope-attention.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_rope_attention_image_test.cpp" -o "$temporary/test"
PIONEER_COORDINATED_PULL_INPUT=1 PIONEER_ATTENTION_PARALLEL_ROPE=1 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline parallel RMS/RoPE attention test retained at %s\n' "$temporary"
