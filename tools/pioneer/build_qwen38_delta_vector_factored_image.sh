#!/bin/sh
# OFFLINE experimental arithmetic image only. No driver/card/native launch authority.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_local_image.sh" delta_vector_factored "$1" "$2"
