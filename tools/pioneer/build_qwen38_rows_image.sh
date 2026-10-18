#!/bin/sh
# OFFLINE multiple16-row fullK candidate;NEVER selected by a native driver.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_STREAM_IMAGE=2 PIONEER_Q38_STREAM_FAST=1 exec sh "$directory/build_qwen38_block_image.sh" "$@"
