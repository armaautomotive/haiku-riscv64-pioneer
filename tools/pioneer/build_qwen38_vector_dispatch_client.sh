#!/bin/sh
# OFFLINE55 DEBUG client;CPU references ONLY in debug,never inference.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_VECTOR_CLIENT=1 exec sh "$directory/build_qwen38_dispatch_client.sh" "$@"
