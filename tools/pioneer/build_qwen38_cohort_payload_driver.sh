#!/bin/sh
# Separate OFFLINE53 data-payload cohort;NEVER install/live-upgrade.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=8 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
