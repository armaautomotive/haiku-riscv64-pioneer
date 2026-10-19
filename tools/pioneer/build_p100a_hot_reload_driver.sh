#!/bin/sh
# Offline guarded55 variant. Does NOT stage,install,reload or touch hardware.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
export PIONEER_DRIVER_RELOAD=1 PIONEER_DRIVER_BUILD_TAG="$3"
PIONEER_Q38_PANEL_DRIVER=10 exec sh "$directory/build_qwen38_banks_driver.sh" "$1" "$2"
