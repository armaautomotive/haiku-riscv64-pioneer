#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-raw-cache.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_q38_raw_unpack_cache_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline raw-cache panel evidence %s;NO device access\n' "$temporary"
