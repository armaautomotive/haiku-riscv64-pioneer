#!/bin/sh
# OFFLINE ONLY separate56 host,ONLY frozen clean ABI libs/headers.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_INFERENCE=2 exec sh "$directory/build_qwen38_dispatch_inference.sh" "$@"
