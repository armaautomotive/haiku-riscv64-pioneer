#!/bin/sh
set -eu

TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SOURCE=${1:-$TEST_DIR/../../../src/add-ons/kernel/busses/usb/xhci.cpp}
TEST_OUTPUT=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-xhci-cleanup.XXXXXX")
# Leave generated test artifacts available for inspecting a failed run.

awk '
	/^XHCI::~XHCI\(\)$/ { copying = 1 }
	copying { print }
	copying && /^}$/ { found = 1; exit }
	END { if (!found) exit 1 }
' "$SOURCE" > "$TEST_OUTPUT/xhci_destructor.inc"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
	-fsanitize=address,undefined -fno-omit-frame-pointer \
	-I "$TEST_OUTPUT" "$TEST_DIR/xhci_cleanup_test.cpp" \
	-o "$TEST_OUTPUT/xhci_cleanup_test"
"$TEST_OUTPUT/xhci_cleanup_test"
