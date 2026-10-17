#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-qwen38-activation0-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen38_activation0_image_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline Q8_0 activation instruction test retained at %s\n' "$temporary"
