#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-token-manual-ttsync-20261103-a
test ! -e "$stage" && test ! -L "$stage"
mkdir -m 700 "$stage"
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
echo NEW_EMPTY_ROOT700_MANUAL_STAGE_CREATED_NO_DRIVER_OR_GPU_CHANGE
