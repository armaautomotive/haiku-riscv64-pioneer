#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/assistant-qwen27-test.XXXXXXXX")
flags='-std=c++11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined'
${CXX:-c++} $flags "$directory/assistant_qwen27_fake_worker.cpp" -o "$temporary/worker"
${CXX:-c++} $flags -DASSISTANT_QWEN27_WORKER="\"$temporary/worker\"" \
 "$directory/assistant_qwen27_test.cpp" "$directory/../../../src/apps/assistant/P100AQwen27.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline Assistant27 evidence retained at %s\n' "$temporary"
