#!/bin/sh
# OFFLINE ONLY,NO install/card operations.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_PERSISTENT_IMAGE=5 exec sh "$directory/build_qwen38_persistent_rows_image.sh" "$@"
