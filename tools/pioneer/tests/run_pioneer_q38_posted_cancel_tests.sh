#!/bin/sh
set -eu
task_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_tmp=$(mktemp -d /private/tmp/pioneer-q38-posted-cancel-unit.XXXXXXXX)
trap 'rm -f -- "$task_tmp/test"; rmdir -- "$task_tmp"' EXIT
"${CXX:-c++}" -std=c++17 -O1 -g0 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$task_dir/pioneer_q38_posted_cancel_test.cpp" -o "$task_tmp/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$task_tmp/test"
