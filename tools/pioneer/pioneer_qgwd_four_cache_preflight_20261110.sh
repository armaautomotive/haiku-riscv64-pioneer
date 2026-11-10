#!/bin/sh
# CPU-only four-cache preflight. Never opens P100A or installs a driver.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
stage=/boot/home/pioneer-qgwd-four-cache-preflight-20261110-a
binary=$stage/pioneer-q38-qgwd-cache-preflight
test "$(uname -s)" = Haiku && test "$(id -u)" = 0 && test -f "$binary" && test ! -L "$binary"
sum=$(sha256sum "$binary");test "${sum%% *}" = 0aee55451ae1d5e90e29060f119ad5a479a411841f6e93096f2456fe6bb668df
test ! -e "$stage/exit-status" && test ! -L "$stage/exit-status"
set +e
"$binary" --verify-qgwd /HaikuData/pioneer-qgw8-bfp4-native-20261107 \
 /boot/home/pioneer-hybrid-bfp8-20261107-cache /HaikuData/pioneer-qgw8-head-native-20261107 \
 /HaikuData/pioneer-qgwd-dense-bfp8-20261109-a
status=$?
set -e
printf '%s\n' "$status" > "$stage/exit-status"
exit "$status"
