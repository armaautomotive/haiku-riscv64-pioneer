#!/bin/sh
# Actual old+new controllers in one canonical reservation and full CLOSE fault.
set -eu
[ "$#" = 1 ] || exit 2
[ "$(shasum -a 256 "$1/text.bin" | awk '{print $1}')" = 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-decoder-initialization.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DPIONEER_Q38_GDN_MUTABLE_STATE \
 "$directory/pioneer_q38_decoder_initialization_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline joint decoder initializer evidence:%s;NO device access\n' "$temporary"
