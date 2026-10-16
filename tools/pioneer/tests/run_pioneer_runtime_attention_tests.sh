#!/bin/sh
set -eu
attention_shared_root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
attention_shared_dir=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-attention-shared.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$attention_shared_root/pioneer_runtime_attention_test.cpp" -o "$attention_shared_dir/test"
UBSAN_OPTIONS=halt_on_error=1 "$attention_shared_dir/test"
printf 'Offline shared attention entry test retained at %s\n' "$attention_shared_dir"
