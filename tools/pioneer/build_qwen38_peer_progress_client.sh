#!/bin/sh
# OFFLINE ONLY,separate59 debug client,not model inference.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_CLIENT=5 exec sh "$directory/build_qwen38_dispatch_client.sh" "$@"
