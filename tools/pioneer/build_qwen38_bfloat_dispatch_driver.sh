#!/bin/sh
# OFFLINE ONLY separate56,NO installation/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=11 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
