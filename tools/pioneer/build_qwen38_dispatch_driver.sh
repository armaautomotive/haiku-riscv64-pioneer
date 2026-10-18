#!/bin/sh
# Separate OFFLINE54 persistent root+119 peers;NEVER install/live-upgrade.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=9 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
