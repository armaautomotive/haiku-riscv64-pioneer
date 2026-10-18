#!/bin/sh
# Separate guarded experimental48 local FULL-BLOCK probe + default control.
# No install/reload/card/reset/other source build or sync.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=3 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
