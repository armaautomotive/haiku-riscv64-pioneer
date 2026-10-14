#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-kexec-trace-tests.XXXXXX")
trap 'rm -f "$temporary/trace-test"; rmdir "$temporary"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$test_dir/kexec_trace_test.c" "$test_dir/../kexec_trace/trace.c" \
	-o "$temporary/trace-test"
"$temporary/trace-test"
