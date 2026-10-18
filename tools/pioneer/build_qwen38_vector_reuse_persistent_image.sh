#!/bin/sh
# OFFLINE ONLY vector peer+affine reuse;NO native install/card operations.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_PERSISTENT_IMAGE=2 exec sh "$directory/build_qwen38_persistent_rows_image.sh" "$@"
