#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
actual=$(shasum -a 256 "$1/text.bin"); [ "${actual%% *}" = 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b ]
actual=$(shasum -a 256 "$1/boot.bin"); [ "${actual%% *}" = e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b ]
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-state-zero-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_state_zero_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline actual state-zero evidence retained at %s;NO hardware accessed\n' "$temporary"
