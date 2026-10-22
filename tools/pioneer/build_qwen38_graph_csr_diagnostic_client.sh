#!/bin/sh
# Build-only fixed tag31 DEBUG client,CPU references ONLY in qualification.
# No upload,device open,new FFN selector or production Work promotion.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_GRAPH_CSR_REFERENCE_CLIENT=1 PIONEER_Q38_SENTINEL_REFERENCE_CLIENT=1 \
 PIONEER_Q38_ROOT_CYCLE_CLIENT=1 PIONEER_Q38_WORK_CLIENT=1 \
 exec sh "$directory/build_qwen38_vector_dispatch_client.sh" "$@"
