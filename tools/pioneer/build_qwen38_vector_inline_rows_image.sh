#!/bin/sh
# OFFLINE ONLY:inline-issue vector follow-on,never a native driver selection.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_STREAM_IMAGE=5 PIONEER_Q38_STREAM_FAST=0 PIONEER_Q38_FAST_SCALE=0 \
 exec sh "$directory/build_qwen38_block_image.sh" "$@"
