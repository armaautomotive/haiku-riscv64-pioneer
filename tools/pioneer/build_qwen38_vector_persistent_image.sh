#!/bin/sh
# OFFLINE ONLY vector persistent peer;no driver install/selection/card IO.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_PERSISTENT_IMAGE=1 exec sh "$directory/build_qwen38_persistent_rows_image.sh" "$@"
