#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-plan-metadata-cost.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror "$directory/pioneer_q38_plan_metadata_cost_test.cpp" -o "$temporary/test"
"$temporary/test" "$@"
printf 'Offline plan metadata-cost evidence retained at %s\n' "$temporary"
