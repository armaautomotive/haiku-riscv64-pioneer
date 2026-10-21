#!/bin/sh
# Compile-only explicit CPU-reference resident debug client,NO device access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=9 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
