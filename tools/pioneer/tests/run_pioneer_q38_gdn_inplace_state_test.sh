#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tmp=$(mktemp -d "${TMPDIR:-/private/tmp}/pioneer-q38-gdn-inplace.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra -Werror \
	-DPIONEER_Q38_GDN_INPLACE_LAYOUT_TEST=1 \
	"$here/pioneer_q38_gdn_inplace_state_test.cpp" -o "$tmp/gdn-inplace-test"
"$tmp/gdn-inplace-test"
