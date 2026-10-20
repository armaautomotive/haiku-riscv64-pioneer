#!/bin/sh
# Compile-only explicit LOCAL CPU-checked qualification;no device access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=6 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
