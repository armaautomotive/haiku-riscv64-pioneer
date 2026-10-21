#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-wait-peers.XXXXXXXX")
awk '/bool WaitPeers\(/ {copying=1;sub(/^static .* bool WaitPeers/,"static bool WaitPeers")}
 copying && /^#endif/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../blackhole_qwen38_dispatch.cpp" > "$temporary/wait_peers.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_wait_peers_budget_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline actual WaitPeers budget evidence retained at %s;NO card access\n' "$temporary"
