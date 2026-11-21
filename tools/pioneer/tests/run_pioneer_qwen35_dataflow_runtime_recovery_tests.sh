#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-runtime-recovery.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g \
	-fsanitize=address,undefined -fno-omit-frame-pointer \
	"$directory/pioneer_qwen35_dataflow_runtime_recovery_test.cpp" \
	-o "$temporary/test"
"$temporary/test"
