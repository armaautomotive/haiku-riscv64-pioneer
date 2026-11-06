#!/bin/sh
# Full local instruction-chain/compiled-function tests ONLY;no model/device.
set -eu
[ "$#" = 2 ] || exit 2
sdk=$1
output=$2
case "$sdk:$output" in /*:/*) ;; *) exit 2;; esac
test -d "$sdk" && test ! -e "$output" && test ! -L "$output" || exit 2
mkdir -m 700 "$output"
tests=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
compiler=${CXX:-c++}
"$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$tests/pioneer_q38_q4_factored_tile_test.cpp" -o "$output/tile-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/tile-test"
python3 "$tests/../build_q38_q4_factored_bfp8.py" "$sdk" "$output/compiled" --inline --tile
"$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$tests/pioneer_q38_q4_factored_tile_code_test.cpp" -o "$output/code-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 \
 "$output/code-test" "$output/compiled/text.bin" "$output/compiled/symbols.txt"
python3 "$tests/pioneer_q38_q4_factored_tile_roles_test.py" "$sdk"
# Changed shared model hook defaults to the previous raw-publication policy.
"$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$tests/pioneer_q38_q4_fpu_pipeline_test.cpp" -o "$output/default-pipeline"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/default-pipeline"
# The added --tile builder selector must leave the old arithmetic probe exact.
python3 "$tests/../build_q38_q4_factored_bfp8.py" "$sdk" "$output/default-arithmetic" --inline
test "$(shasum -a 256 "$output/default-arithmetic/text.bin" | cut -d ' ' -f1)" = 455c60867c618023d894f10ad2e9a74d8a88d0f512dcd9ce9b2eae80ef46cef5
