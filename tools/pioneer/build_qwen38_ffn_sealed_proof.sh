#!/bin/sh
# Build ONLY. Same qualified device images;omit duplicate CPU work-manifest
# post-read ONLY. Exact tag38 with measured phases;STOP remains unchanged.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_ffn_native_graph.sh" "$1" "$2" --sealed-proof
