#!/bin/sh
# Full original-model source,host memory only;NO accelerator IO.
set -eu
[ "$#" = 1 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-output-debug68-original-upload.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_output_original_upload_test.cpp" "$here/../pioneer_q38_original_source.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$1"
printf 'CPU-only original upload seam evidence retained at %s\n' "$build"
