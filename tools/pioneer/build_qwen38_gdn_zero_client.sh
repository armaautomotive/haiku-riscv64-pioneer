#!/bin/sh
# Compile-only fixed zero diagnostic,NO linked driver/device operation.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=8 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
