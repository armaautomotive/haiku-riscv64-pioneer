#!/bin/sh
# Fixed PUBLIC 1MiB pre-cohort transport test;build only,NO deployment/IO.
set -eu
PIONEER_Q38_PANEL_CLIENT=5 exec sh "$(dirname "$0")/build_qwen38_banks_client.sh" "$@"
