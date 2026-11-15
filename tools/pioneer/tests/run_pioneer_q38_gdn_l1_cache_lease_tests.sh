#!/bin/sh
# Metadata-only lease test; never accesses device or native inference.
set -eu
[ "$#" = 0 ] || exit 2
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith(("PIONEER_","TT_PIONEER_")) for k in os.environ) else 2)' || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
tmp=$(mktemp -d /private/tmp/pioneer-q38-gdn-cache-lease.XXXXXXXX)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_gdn_l1_cache_lease_inspection_test.cpp" -o "$tmp/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$tmp/test"
python3 "$directory/pioneer_q38_gdn_l1_cache_lease_role_test.py"
