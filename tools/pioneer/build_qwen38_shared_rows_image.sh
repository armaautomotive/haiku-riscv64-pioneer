#!/bin/sh
# OFFLINE shared-root input follow-on. No driver install or card operations.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_STREAM_IMAGE=3 PIONEER_Q38_STREAM_FAST=1 PIONEER_Q38_FAST_SCALE=1 \
 exec sh "$directory/build_qwen38_block_image.sh" "$@"
