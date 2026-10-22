#!/bin/sh
# Actual zero+embedding ROM machines/controller,NO card access.
set -eu
[ "$#" = 2 ] || [ "$#" = 3 ] || exit 2
[ "$(shasum -a 256 "$1/text.bin" | awk '{print $1}')" = 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b ] || exit 2
[ "$(shasum -a 256 "$2/code.bin" | awk '{print $1}')" = c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-embedding-controller.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE= "$directory/pioneer_q38_embedding_controller_test.cpp" \
 "$directory/pioneer_q38_embedding_execution_bridge.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual embedding controller evidence:%s;NO native access\n' "$temporary"
