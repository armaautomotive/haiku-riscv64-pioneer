#!/bin/sh
# OFFLINE ONLY,isolated58 transfer diagnostic;NO deployment/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=13 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
