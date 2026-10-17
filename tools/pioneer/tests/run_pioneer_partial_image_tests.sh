#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-partial-test.XXXXXXXX")
sh "$directory/../build_partial_image.sh" "$1" "$temporary/image"
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_partial_image_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/image/code.bin"
printf 'Offline partial image test retained at %s\n' "$temporary"
