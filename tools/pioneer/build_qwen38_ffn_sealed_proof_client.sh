#!/bin/sh
# Build ONLY. Same source/input/prior-GPU oracle;exact measured tag38.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_ffn_prime.sh" "$1" "$2" --native-graph-sealed-proof
