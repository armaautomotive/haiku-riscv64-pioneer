#!/bin/sh
# Transport wrapper: wait on the actual parent; never infer exit from logs.
set -eu
stage=/boot/home/pioneer-native-factored-q4-compare111-a
test -d "$stage" && test ! -L "$stage" || exit 1
test "$(sha256sum "$stage/pioneer_native_factored_q4_compare_physical.sh" | awk '{print $1}')" = f90b5288feb08039e65a8d6afd646c206bf7aa5bc56c1d088aa24cd3787c6bdf || exit 1
for name in .launcher-once launcher.pid parent.stdout parent.stderr parent.exit; do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
mkdir "$stage/.launcher-once"
printf '%s\n' "$$" > "$stage/launcher.pid"
if /bin/sh "$stage/pioneer_native_factored_q4_compare_physical.sh" > "$stage/parent.stdout" 2> "$stage/parent.stderr"; then result=0; else result=$?; fi
printf '%s\n' "$result" > "$stage/parent.exit"
cat "$stage/parent.stdout"
cat "$stage/parent.stderr"
printf 'NATIVE111_PARENT_ACTUAL_EXIT%s\n' "$result"
exit "$result"
