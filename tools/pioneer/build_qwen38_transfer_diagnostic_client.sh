#!/bin/sh
# OFFLINE ONLY,separate58 debug client,not model inference.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_CLIENT=4 exec sh "$directory/build_qwen38_dispatch_client.sh" "$@"
