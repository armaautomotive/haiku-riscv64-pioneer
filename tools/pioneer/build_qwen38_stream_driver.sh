#!/bin/sh
# OFFLINE separate49 fullK candidate+default43 control;no install/card/reset.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=4 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
