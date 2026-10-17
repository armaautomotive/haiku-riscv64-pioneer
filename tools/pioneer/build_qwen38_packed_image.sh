#!/bin/sh
# OFFLINE ONLY:no driver build/install/device access.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_local_image.sh" packed "$1" "$2"
