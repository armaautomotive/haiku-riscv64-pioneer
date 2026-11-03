#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/assistant-qwen27-load-budget.XXXXXXXX")
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined'
${CXX:-c++} $flags "$directory/assistant_qwen27_fake_worker.cpp" -o "$temporary/worker"
${CXX:-c++} $flags "-DASSISTANT_QWEN27_WORKER=\"$temporary/worker\"" \
 "$directory/assistant_qwen27_load_budget_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'CPU-only load-budget evidence: %s\n' "$temporary"
