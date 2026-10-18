#!/bin/sh
# Standalone DEBUG batched fullK primitive client,NOT inference/Assistant.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=2 PIONEER_Q38_ROWS_CLIENT=1 PIONEER_Q38_STREAM_DIAGNOSTICS=1 \
 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
