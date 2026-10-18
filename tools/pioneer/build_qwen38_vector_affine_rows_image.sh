#!/bin/sh
# OFFLINE ONLY separate vector FP32 epilogue;no native driver/image update.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_STREAM_IMAGE=6 PIONEER_Q38_STREAM_FAST=0 PIONEER_Q38_FAST_SCALE=0 \
 exec sh "$directory/build_qwen38_block_image.sh" "$@"
