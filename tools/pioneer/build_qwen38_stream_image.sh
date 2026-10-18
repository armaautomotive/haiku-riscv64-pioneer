#!/bin/sh
# OFFLINE fullK candidate;no installed driver selects this image.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_STREAM_IMAGE=1 exec sh "$directory/build_qwen38_block_image.sh" "$@"
