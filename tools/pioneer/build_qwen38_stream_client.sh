#!/bin/sh
# Standalone DEBUG fullK primitive client;not inference/Assistant/performance.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=2 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
