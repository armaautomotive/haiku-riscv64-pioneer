#!/bin/sh
# OFFLINE ONLY separate56 debug client,NO device access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_CLIENT=2 exec sh "$directory/build_qwen38_dispatch_client.sh" "$@"
