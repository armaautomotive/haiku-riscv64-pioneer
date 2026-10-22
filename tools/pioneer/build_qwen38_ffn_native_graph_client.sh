#!/bin/sh
# Source-bound full-SHA native qualification client ONLY;NO execution.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_ffn_prime.sh" "$1" "$2" --native-graph
