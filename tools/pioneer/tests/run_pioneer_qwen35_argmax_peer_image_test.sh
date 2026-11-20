#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-qwen35-argmax-peer.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
sh "$directory/../build_qwen35_work_peer_image.sh" "$1" "$temporary/peer"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_argmax_peer_image_test.cpp" \
	-o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/peer"
