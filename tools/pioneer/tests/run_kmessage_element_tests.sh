#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-kmessage-tests.XXXXXX")
trap 'rm -f "$temporary/element-test" "$temporary/kmessage_fields.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /^struct KMessage::FieldValueHeader/ { copying = 1 }
 copying && /#pragma mark - KMessage$/ { found = 1; exit }
 copying { print }
 END { if (!found) exit 1 }
' "$test_dir/../../../src/system/kernel/messaging/KMessage.cpp" > "$temporary/kmessage_fields.inc"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$test_dir/kmessage_element_test.cpp" -o "$temporary/element-test"
"$temporary/element-test"
