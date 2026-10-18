#!/bin/sh
# OFFLINE ONLY,NO driver/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_PERSISTENT_IMAGE=7 exec sh "$directory/build_qwen38_persistent_rows_image.sh" "$@"
