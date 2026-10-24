#!/bin/sh
# Separate QFG3:completed GDDR normalized+residual -> FULL FFN+residual.
# OFFLINE ONLY;no native selector,host input,installation or device access.
set -eu
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT=1 PIONEER_Q38_FFN_GRAPH_ROOT=1 \
 exec sh "$directory/build_qwen38_ffn_graph_image.sh" "$@"
