#!/bin/sh
# OFFLINE ONLY. No driver/card/route/native launch authority.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_local_image.sh" delta_vector "$1" "$2"
