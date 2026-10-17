#!/bin/sh
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_local_image.sh" activation "$@"
