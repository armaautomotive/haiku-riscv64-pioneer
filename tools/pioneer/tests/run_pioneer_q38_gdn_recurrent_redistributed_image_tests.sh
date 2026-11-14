#!/bin/sh
# Offline ACTUAL emitted candidate worker + simulated NoC/GDDR only.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
root=$(CDPATH= cd -- "$directory/.." && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-q38-gdn-recurrent-redistributed-test.XXXXXXXX)
image="$temporary/image"
bash "$root/build_qwen38_gdn_recurrent_redistributed_image.sh" "$1" "$image"
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
	-fsanitize=address,undefined -DPIONEER_Q38_GDN_REDISTRIBUTED_IMAGE=1 \
	"$directory/pioneer_q38_gdn_recurrent_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$image"
