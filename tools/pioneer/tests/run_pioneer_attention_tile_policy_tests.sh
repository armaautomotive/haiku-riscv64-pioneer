#!/bin/sh
set -eu
attention_policy_root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
attention_policy_dir=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-attention-policy.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$attention_policy_root/pioneer_attention_tile_policy_test.cpp" -o "$attention_policy_dir/test"
UBSAN_OPTIONS=halt_on_error=1 "$attention_policy_dir/test"
printf 'Offline attention policy test retained at %s\n' "$attention_policy_dir"
