#!/bin/sh
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-qwen38-fixture.XXXXXXXX")
test "$(shasum -a 256 "$2" | cut -d ' ' -f1)" = eb62d234359d028a62e3dcf8ee1729c970cc2470e4999c6327ab3f01198a4f51
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen38_fixture_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
printf 'Offline public fixture evidence retained at %s\n' "$temporary"
