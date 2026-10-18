#!/bin/sh
# Separate47 debug fixture client. Fixed public DATA only,verified MEMORY
# SHA before device open. Model mode retains whole same-FD authentication.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=1 PIONEER_Q38_AFFINE_CLIENT=1 PIONEER_Q38_FIXTURE_CLIENT=1 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
