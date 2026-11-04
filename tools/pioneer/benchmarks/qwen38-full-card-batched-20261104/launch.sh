#!/bin/sh
set -eu
stage=/boot/home/pioneer-q38-full-batched-20261104-a
helper=$stage/pioneer_q38_full_offload_batched_physical.sh
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
test -f "$helper" && test ! -L "$helper" && [ "$(stat -c '%u:%h:%a' "$helper")" = 0:1:700 ]
digest=$(sha256sum "$helper")
[ "${digest%% *}" = 90001ae65aab8eac168406186728a8c20ba9e22cfe96993ef87fedd25359e86d ]
test ! -e "$stage/.trial-once" && test ! -e "$stage/parent.pid" && test ! -e "$stage/state"
# This exact one-shot helper repeats all artifact/runtime/idle/health and
# generation checks BEFORE any exchange or model/accelerator execution.
exec sh "$helper" --hello-four
