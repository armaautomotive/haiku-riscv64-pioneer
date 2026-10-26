#!/bin/sh
# Full original byte endpoints only,NO native/card/CPU-product inputs.
set -eu
[ "$#" -le 1 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-layer-learned-bytes.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_layer_learned_bytes_test.cpp" "$here/pioneer_q38_decoder_original_source.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$@"
