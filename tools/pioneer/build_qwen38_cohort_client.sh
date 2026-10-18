#!/bin/sh
# Offline standalone DEBUG cohort client;NEVER install or access a card.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_PANEL_CLIENT=3 exec sh "$directory/build_qwen38_banks_client.sh" "$@"
