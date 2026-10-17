#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
case "$1" in /*) model=$1;; *) model=$PWD/$1;; esac
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-qwen38-inspect.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/../pioneer_qwen38_inspect.cpp" -o "$temporary/test"
export UBSAN_OPTIONS=halt_on_error=1
"$temporary/test" "$model"
if "$temporary/test"; then exit 1; else result=$?; test "$result" = 2; fi
if "$temporary/test" "$model" extra; then exit 1; else result=$?; test "$result" = 2; fi
if "$temporary/test" "$temporary/missing"; then exit 1; else result=$?; test "$result" = 1; fi
if "$temporary/test" "$temporary"; then exit 1; else result=$?; test "$result" = 1; fi
empty=$(mktemp "$temporary/empty.XXXXXXXX")
if "$temporary/test" "$empty"; then exit 1; else result=$?; test "$result" = 1; fi
ln -s "$model" "$temporary/model-link"
if "$temporary/test" "$temporary/model-link"; then exit 1; else result=$?; test "$result" = 1; fi
printf 'PASS: read-only inspector real layout/first+last rows,argument/missing/directory/empty/symlink refusals; retained at %s\n' "$temporary"
