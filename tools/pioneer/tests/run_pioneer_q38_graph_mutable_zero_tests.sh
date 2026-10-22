#!/bin/sh
# Existing controllers+actual frozen zero instructions,simulated fabric.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is(){ actual=$(shasum -a 256 "$1");[ "${actual%% *}" = "$2" ]; }
hash_is "$1/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
hash_is "$1/text.bin" 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-graph-mutable-zero.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE "$directory/pioneer_q38_graph_mutable_zero_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline joint role zero evidence retained at %s;NO device access\n' "$temporary"
