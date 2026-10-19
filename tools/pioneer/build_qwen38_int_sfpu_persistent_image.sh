#!/bin/sh
# OFFLINE ONLY. No driver/card access or native speed qualification.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_PERSISTENT_IMAGE=9 PIONEER_Q38_Q4_TWO_PHASE=1 exec sh "$directory/build_qwen38_persistent_rows_image.sh" "$@"
