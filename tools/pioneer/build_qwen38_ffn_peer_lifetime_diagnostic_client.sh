#!/bin/sh
# Exact-tag33 namespace6 debug qualifier;CPU references ONLY in this test.
# New workers boot+STOP;root/model execution remains absent. NO deployment.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT=1 PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT=1 \
 PIONEER_Q38_SENTINEL_REFERENCE_CLIENT=1 PIONEER_Q38_ROOT_CYCLE_CLIENT=1 PIONEER_Q38_WORK_CLIENT=1 \
 exec sh "$directory/build_qwen38_vector_dispatch_client.sh" "$@"
