#!/bin/sh
# OFFLINE ONLY,NO driver/card operations.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_STREAM_IMAGE=8 PIONEER_Q38_STREAM_FAST=0 PIONEER_Q38_FAST_SCALE=0 \
 exec sh "$directory/build_qwen38_block_image.sh" "$@"
