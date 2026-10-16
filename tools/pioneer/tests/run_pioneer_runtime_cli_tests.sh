#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-runtime-cli.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/stdout" "$temporary/stderr"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++11 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/cli-stubs" "$directory/pioneer_runtime_cli_test.cpp" -o "$temporary/test"
if "$temporary/test" > "$temporary/stdout" 2> "$temporary/stderr"; then
 tail -n 1 "$temporary/stdout"
else
 tail -n 25 "$temporary/stdout"
 cat "$temporary/stderr"
 exit 1
fi
