#!/bin/sh
# OFFLINE ONLY. No driver build/install, device operation or route authority.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_local_image.sh" delta "$1" "$2"
