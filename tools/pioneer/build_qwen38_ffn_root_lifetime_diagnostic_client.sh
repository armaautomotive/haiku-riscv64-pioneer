#!/bin/sh
# CPU-reference DEBUG client for exact tag34/namespace7 only,NO deployment.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_FFN_ROOT_LIFETIME_REFERENCE_CLIENT=1 PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT=1 \
 PIONEER_Q38_SENTINEL_REFERENCE_CLIENT=1 PIONEER_Q38_PANEL_CLIENT=4 PIONEER_Q38_VECTOR_CLIENT=1 \
 PIONEER_Q38_WORK_CLIENT=1 PIONEER_Q38_ROOT_CYCLE_CLIENT=1 \
 sh "$directory/build_qwen38_banks_client.sh" "$@"
