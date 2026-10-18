#!/bin/sh
# OFFLINE ONLY,isolated59 peer-progress protocol;NO deployment/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=14 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
