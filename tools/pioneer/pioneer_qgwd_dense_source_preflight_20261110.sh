#!/bin/sh
# CPU-only source authentication; does not access the P100A or driver.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
stage=/boot/home/pioneer-qgwd-real-dense-band-20261110-a
binary=$stage/source-preflight-build-1/pioneer-qgwd-dense-source-preflight
test "$(uname -s)" = Haiku && test "$(id -u)" = 0 && test -f "$binary" && test ! -L "$binary"
sum=$(sha256sum "$binary");test "${sum%% *}" = 5729dec8db5aa20d1dc9022a678809060a73f923bcca854f76ed66b270ed85cf
test ! -e "$stage/source-preflight-exit" && test ! -L "$stage/source-preflight-exit"
set +e
"$binary"
status=$?
set -e
printf '%s\n' "$status" > "$stage/source-preflight-exit"
exit "$status"
