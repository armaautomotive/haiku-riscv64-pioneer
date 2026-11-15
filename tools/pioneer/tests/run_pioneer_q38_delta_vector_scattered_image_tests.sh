#!/bin/sh
# OFFLINE emitted local-state prototype; no card, driver, native owner or TPS.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/private/tmp}/pioneer-q38-scattered-state.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
sh "$directory/build_qwen38_local_image.sh" delta_vector_scattered "$1" "$temporary/image"
actual=$(shasum -a 256 "$temporary/image/code.bin"); [ "${actual%% *}" = 4a71216023dc18374415d544b2dc5ae8ac3b49156e00a6999aefca2123716315 ]
boot=$(shasum -a 256 "$temporary/image/boot.bin"); [ "${boot%% *}" = d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6 ]
testDirectory="$directory/tests"
testOutput="$temporary/test"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
 -fno-omit-frame-pointer -ffp-contract=off -DPIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT=1 \
 "$testDirectory/pioneer_q38_delta_vector_image_test.cpp" -o "$testOutput"
"$testOutput" "$temporary/image"
sh "$directory/build_qwen38_local_image.sh" delta_vector "$1" "$temporary/control"
control=$(shasum -a 256 "$temporary/control/code.bin"); [ "${control%% *}" = 05bef31999bb3a8fa0280744af52645fc92e31a441207fe6e842fe8b53464a52 ]
sh "$testDirectory/run_pioneer_q38_delta_vector_image_tests.sh" "$temporary/control"
printf 'OFFLINE scattered-state emitted image and ASan/UBSan tests retained at %s; no native selection/device/TPS\n' "$temporary"
