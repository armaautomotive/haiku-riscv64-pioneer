#!/bin/sh
# OFFLINE separate50 batched fullK candidate+43 control;NEVER install/reload.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=5 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
