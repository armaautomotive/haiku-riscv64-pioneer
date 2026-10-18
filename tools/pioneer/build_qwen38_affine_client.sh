#!/bin/sh
# Separate experimental47 local affine debug client,not inference/backend.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=1 PIONEER_Q38_AFFINE_CLIENT=1 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
