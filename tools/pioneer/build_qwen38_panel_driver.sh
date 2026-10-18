#!/bin/sh
# Compile/link separate experimental46 local panel probe + default control.
# NEVER installs/reloads/starts/resets hardware or syncs other sources.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=1 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
