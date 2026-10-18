#!/bin/sh
# OFFLINE ONLY all-four-phase RWC candidate,NO install/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_PERSISTENT_IMAGE=4 exec sh "$directory/build_qwen38_persistent_rows_image.sh" "$@"
