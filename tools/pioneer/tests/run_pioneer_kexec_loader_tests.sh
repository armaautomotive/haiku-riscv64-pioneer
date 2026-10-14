#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-kexec-loader-tests.XXXXXX")
trap 'rm -f "$temporary/loader-test"; rmdir "$temporary"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$test_dir/pioneer_kexec_loader_test.c" -o "$temporary/loader-test"
"$temporary/loader-test" "$@"
