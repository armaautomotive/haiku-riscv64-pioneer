#!/bin/sh
# Detached one-shot launcher for the lengthy tag101 full-model rate trial.
set -eu
stage=/boot/home/pioneer-q38-full-spill-q4-sfpnop-20261115-d
parent=$stage/pioneer_q38_full_offload_spill_q4_sfpnop_rate_physical.sh
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -f "$parent" && test ! -L "$parent" && [ "$(stat -c '%u:%h:%a' "$parent")" = 0:1:700 ] || exit 1
digest=$(sha256sum "$parent")
[ "${digest%% *}" = 59cd346a40338ace08780e3626de3beef7e8ce2d8bb89f04afd48e848524cf7e ] || exit 1
for name in .direct-launch-once direct-parent.stdout direct-parent.stderr direct-parent.exit direct-launcher.pid parent.pid state rate.stdout rate.stderr rate.exit;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.direct-launch-once" # spent forever,including every failure
printf '%s\n' "$$" > "$stage/direct-launcher.pid"
if /bin/sh "$parent" --rate-32 > "$stage/direct-parent.stdout" 2> "$stage/direct-parent.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/direct-parent.exit"
printf 'DIRECT_TAG101_PARENT_ACTUAL_EXIT%s\n' "$result"
exit "$result"
