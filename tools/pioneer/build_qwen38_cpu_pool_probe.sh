#!/bin/sh
# Frozen CPU-library API/lifecycle only,no model/device/deployment.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_CPU_POOL_PROBE=1 exec sh "$directory/build_qwen38_inference.sh" "$@"
