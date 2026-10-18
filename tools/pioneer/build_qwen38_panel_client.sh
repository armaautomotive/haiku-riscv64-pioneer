#!/bin/sh
# Standalone debug client only,no device access/install/dirty llama source.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=1 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
