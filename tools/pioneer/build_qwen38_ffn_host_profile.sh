#!/bin/sh
# CPU phase timing ONLY of the qualified unchanged QFG2/QFP2 images.
# Isolated tag37;no installation/execution/Assistant promotion.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_ffn_native_graph.sh" "$1" "$2" --host-profile
