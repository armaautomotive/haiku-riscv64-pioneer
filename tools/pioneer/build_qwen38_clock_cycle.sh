#!/bin/sh
# Cross-build only;explicit bounded native qualification is separate.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=7 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
