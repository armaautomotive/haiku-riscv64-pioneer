#!/bin/sh
# OFFLINE ROOT gather only;no native role/registry/card access.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_local_image.sh" projection_gather "$@"
