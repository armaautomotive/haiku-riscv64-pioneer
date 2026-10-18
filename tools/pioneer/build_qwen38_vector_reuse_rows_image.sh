#!/bin/sh
# OFFLINE ONLY affine-block reuse;never a native driver/image selection.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_STREAM_IMAGE=7 PIONEER_Q38_STREAM_FAST=0 PIONEER_Q38_FAST_SCALE=0 \
 exec sh "$directory/build_qwen38_block_image.sh" "$@"
