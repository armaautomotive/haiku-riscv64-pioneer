#!/bin/sh
set -eu
stage=/boot/home/pioneer-q38-full-metrics-20261104-a
helper=$stage/pioneer_q38_full_offload_rate_physical.sh
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
test -f "$helper" && test ! -L "$helper" && [ "$(stat -c '%u:%h:%a' "$helper")" = 0:1:700 ]
digest=$(sha256sum "$helper")
[ "${digest%% *}" = 5df0d6ae9714e192b56c1de654c8edd25a76dac8c91853c31b1cef91cb65f91c ]
for name in .trial-once parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name"
done
# Exact qualified helper repeats all current admission before any exchange.
exec sh "$helper" --rate-32
