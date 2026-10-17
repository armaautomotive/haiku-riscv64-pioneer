#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-host-stage.XXXXXXXX")
${CXX:-c++} -std=c++11 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_runtime_host_stage_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'Host-stage test retained at %s\n' "$temporary"
