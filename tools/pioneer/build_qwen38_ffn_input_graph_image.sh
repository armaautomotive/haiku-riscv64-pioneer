#!/bin/sh
# NEW QFG2 initial F32 copy/readback + complete FFN. NO native selection.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1 PIONEER_Q38_FFN_GRAPH_ROOT=1 \
 exec sh "$directory/build_qwen38_ffn_graph_image.sh" "$@"
