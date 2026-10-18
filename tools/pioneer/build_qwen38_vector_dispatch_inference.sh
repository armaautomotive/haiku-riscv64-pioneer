#!/bin/sh
# OFFLINE55 host graph,ONLY frozen clean ABI libs,no dirty llama build.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_INFERENCE=1 exec sh "$directory/build_qwen38_dispatch_inference.sh" "$@"
