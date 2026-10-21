#!/bin/sh
# Explicit COUNTER-FABRIC lifecycle test,NOT zero-data/image/native execution.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
actual=$(shasum -a 256 "$1/text.bin")
[ "${actual%% *}" = 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b ]
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-publication.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE "$directory/pioneer_q38_gdn_publication_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline initialization-publication evidence retained at %s\n' "$temporary"
