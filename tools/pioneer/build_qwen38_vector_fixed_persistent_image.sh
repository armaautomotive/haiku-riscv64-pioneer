#!/bin/sh
# OFFLINE ONLY fixed-format cached vector peer,NO install/selection/card IO.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_PERSISTENT_IMAGE=3 exec sh "$directory/build_qwen38_persistent_rows_image.sh" "$@"
