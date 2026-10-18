#!/bin/sh
# Separate experimental47 scaled-local-panel driver + default control.
# No install/reload/card/reset/unrelated build or sync.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=2 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
