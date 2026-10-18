#!/bin/sh
# OFFLINE arithmetic-check client for53;NOT model inference/installation.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=3 PIONEER_Q38_COHORT_SEALED=1 PIONEER_Q38_COHORT_PAYLOAD=1 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
