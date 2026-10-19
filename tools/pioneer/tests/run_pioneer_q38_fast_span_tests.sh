#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-fast-span.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_SPAN_FAST "$directory/pioneer_q38_fast_span_image_test.cpp" -o "$temporary/image"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/image" "$1"
printf 'Offline fast span evidence retained at %s\n' "$temporary"
