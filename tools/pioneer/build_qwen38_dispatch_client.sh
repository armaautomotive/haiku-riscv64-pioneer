#!/bin/sh
# Cross-build only;NO deployment/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=4 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
