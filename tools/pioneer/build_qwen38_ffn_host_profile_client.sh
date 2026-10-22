#!/bin/sh
# Source-bound CPU host-phase timing qualifier ONLY;no execution.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_ffn_prime.sh" "$1" "$2" --native-graph-host-profile
