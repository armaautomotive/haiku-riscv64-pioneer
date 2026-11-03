#!/bin/sh
# CPU/userspace tests only;no P100A device access.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-native-weight-cache-20261103-a
test -d "$stage" && test ! -L "$stage"
row=$(sha256sum "$stage/pioneer-model-weight-cache-test-r3");printf '%s\n' "$row"
[ "${row%% *}" = 11ee356c1ca5e07069504a0c65c7ccfa50e677f59b0b70f0c9f67b36466f0b17 ]
row=$(sha256sum "$stage/pioneer-q38-native-weight-tiles-test");printf '%s\n' "$row"
[ "${row%% *}" = 9c6c2edc93213bf2b643dfef91d4ed83978c98f2f7f6eb0c1adb2a461acd8247 ]
test ! -e "$stage/cache-fixtures-r3" && test ! -e "$stage/tiles-fixtures-r3"
mkdir -m 700 "$stage/cache-fixtures-r3" "$stage/tiles-fixtures-r3"
"$stage/pioneer-model-weight-cache-test-r3" "$stage/cache-fixtures-r3"
"$stage/pioneer-q38-native-weight-tiles-test" "$stage/tiles-fixtures-r3"
echo HAIKU_CPU_ONLY_CACHE_AND_TILE_SELFTESTS_COMPLETED_NO_ACCELERATOR_ACCESS
