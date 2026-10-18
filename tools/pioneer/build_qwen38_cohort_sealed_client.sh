#!/bin/sh
# OFFLINE DEBUG arithmetic-check client for52. Still NOT model inference.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=3 PIONEER_Q38_COHORT_SEALED=1 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
