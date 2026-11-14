#!/bin/sh
# Offline redistributed descriptor shape/route tests only;no card or owner authority.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-redistributed-shape.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_q38_gdn_redistributed_shape_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_Q38_RGDN_ROOT_QUALIFICATION \
	"$directory/pioneer_q38_gdn_redistributed_shape_test.cpp" -o "$temporary/root-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/root-test"
printf 'PASS offline redistributed GDN mapping/service proof/mixed dispatch routes;NO native/device/speed qualification\n'
