#!/bin/sh
# Build-only exact-tag32 NS5 debug qualifier;CPU references ONLY in test.
# NO install/upload/new FFN execution/production mode selection.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT=1 PIONEER_Q38_SENTINEL_REFERENCE_CLIENT=1 \
 PIONEER_Q38_ROOT_CYCLE_CLIENT=1 PIONEER_Q38_WORK_CLIENT=1 \
 exec sh "$directory/build_qwen38_vector_dispatch_client.sh" "$@"
