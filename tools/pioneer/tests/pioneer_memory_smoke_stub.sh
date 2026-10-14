#!/bin/sh
set -eu
test "$*" = '--mib=64 --expect-cpus=64'
test ! -e "$MEMORY_BOOT_TEST_BASE/armed-full-v1"
test -f "$MEMORY_BOOT_TEST_BASE/consumed-full-v1"
exit "${MEMORY_BOOT_TEST_RESULT:-0}"
