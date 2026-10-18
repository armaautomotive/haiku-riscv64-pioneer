#!/bin/sh
# OFFLINE ONLY separate55 vector-peer driver;no install/reload/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=10 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
