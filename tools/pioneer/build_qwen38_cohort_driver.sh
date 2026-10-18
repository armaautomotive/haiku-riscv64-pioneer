#!/bin/sh
# OFFLINE separate51 shared-input cohort+default43control. NEVER install.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_DRIVER=6 exec sh "$directory/build_qwen38_banks_driver.sh" "$@"
