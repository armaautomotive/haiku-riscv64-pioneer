#!/bin/sh
# Build ONLY:fixed-local arithmetic,not a native family/driver selector.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_local_image.sh" projection_input "$@"
