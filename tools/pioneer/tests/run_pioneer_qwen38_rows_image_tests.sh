#!/bin/sh
set -eu
[ "$#" -ge 1 ] && [ "$#" -le 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-qwen38-rows-image.XXXXXXXX")
case "${PIONEER_Q38_BINARY_SCALE_IMAGE:-0}" in 0) extra=;; 1) extra=-DPIONEER_Q38_BINARY_SCALE_IMAGE;; *) exit 2;; esac
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 $extra "$directory/pioneer_qwen38_rows_image_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline multiple-row fullK instruction evidence retained at %s\n' "$temporary"
