#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
tmp=$(mktemp -d /private/tmp/q38-gdn-l1-journal.XXXXXX)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
c++ -std=c++11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$root/tools/pioneer/tests/pioneer_q38_gdn_l1_original_page_journal_test.cpp" -o "$tmp/test"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$tmp/test"
if c++ -std=c++11 -Wall -Wextra -Werror -DPIONEER_Q38_WORK_INFERENCE=1 \
	-fsyntax-only "$root/tools/pioneer/tests/pioneer_q38_gdn_l1_original_page_journal_test.cpp" \
	>"$tmp/role.stdout" 2>"$tmp/role.stderr"; then
	echo "FAIL: journal inspection source accepted Work mode" >&2
	exit 1
fi
rg -q "standalone offline inspection component" "$tmp/role.stderr"
echo "PASS GDN L1 original-page journal Work-role refusal"
