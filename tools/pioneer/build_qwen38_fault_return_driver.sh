#!/bin/sh
# OFFLINE ONLY,isolated57 diagnostic driver;NO deployment/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=12 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
