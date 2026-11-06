#!/bin/sh
set -eu
umask 077
stage=/boot/home/pioneer-q38-bf16-deferred-110-a
parent=$stage/pioneer_q38_bf16_deferred_full_offload_physical.sh
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -f "$parent" && test ! -L "$parent" && [ "$(stat -c '%u:%h:%a' "$parent")" = 0:1:700 ] || exit 1
[ "$(sha256sum "$parent")" = "b1a7c8ac9d830fe78bc3b1174e48bd694499fc49d16dcd61aa118f2b8459e1dc  $parent" ] || exit 1
for name in .direct-launch-once .trial-once direct-parent.stdout direct-parent.stderr direct-parent.exit direct-launcher.pid parent.pid rate.stdout rate.stderr rate.exit state; do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.direct-launch-once"
printf '%s\n' "$$" > "$stage/direct-launcher.pid"
if /bin/sh "$parent" --rate-32 > "$stage/direct-parent.stdout" 2> "$stage/direct-parent.stderr"; then result=0; else result=$?; fi
printf '%s\n' "$result" > "$stage/direct-parent.exit"
printf 'DIRECT_DEFERRED_FULL_110_PARENT_ACTUAL_EXIT%s\n' "$result"
exit "$result"
