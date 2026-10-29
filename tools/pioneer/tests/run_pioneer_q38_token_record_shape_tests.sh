#!/bin/sh
set -eu
task_dir=$(mktemp -d /private/tmp/pioneer-token-record-shape.XXXXXXXX)
trap 'printf "Test artifacts: %s\n" "$task_dir"' EXIT
task_here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  "$task_here/pioneer_q38_token_record_shape_test.cpp" -o "$task_dir/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$task_dir/test"
