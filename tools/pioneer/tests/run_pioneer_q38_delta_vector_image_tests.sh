#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-delta-vector-image.XXXXXXXX")
actual=$(shasum -a 256 "$1/code.bin"); [ "${actual%% *}" = 05bef31999bb3a8fa0280744af52645fc92e31a441207fe6e842fe8b53464a52 ]
boot=$(shasum -a 256 "$1/boot.bin"); [ "${boot%% *}" = d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6 ]
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off \
 "$directory/pioneer_q38_delta_vector_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$1"
printf 'Offline emitted vector GDN evidence retained at %s\n' "$temporary"
