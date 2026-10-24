#!/bin/sh
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d /private/tmp/pioneer-q38-gdn-rendezvous-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_gdn_rendezvous_image_test.cpp" -o "$output/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/test" "$1" "$2"
