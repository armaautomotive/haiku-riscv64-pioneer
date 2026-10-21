#!/bin/sh
# Offline actual instructions+simulated NoC ONLY,never a native capability.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
actual=$(shasum -a 256 "$1/code.bin"); [ "${actual%% *}" = c3d150b4818c6abd12de957c1840237b60f46d52c143a894b53efb7492ee5a48 ]
actual=$(shasum -a 256 "$1/boot.bin"); [ "${actual%% *}" = e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b ]
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-resident-gdn-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_resident_gdn_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline resident GDN emitted-image evidence retained at %s\n' "$temporary"
