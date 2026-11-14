#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-l1-plan.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
	"$directory/pioneer_q38_gdn_l1_cache_plan_inspection_test.cpp" -o "$temporary/test"
"$temporary/test"
