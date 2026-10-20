#!/bin/sh
# Frozen ABI/pinned libraries; no dirty llama build or card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_ASSISTANT_WORKER=1 PIONEER_Q38_DISPATCH_INFERENCE=1 PIONEER_Q38_VECTOR_INFERENCE=1 \
 PIONEER_Q38_EXECUTION_MODE=work PIONEER_Q38_SPAN_PRELOAD=1 PIONEER_Q38_SPAN_FAST=1 \
 exec sh "$directory/build_qwen38_inference.sh" "$@"
