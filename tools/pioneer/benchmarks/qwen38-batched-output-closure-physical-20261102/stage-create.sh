#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-batched-output-closure-20261102-a
old=/boot/home/pioneer-p100a-frontier-failure-link-20261102-a/pioneer-q38-FRONTIER34-FAILED-REPLY-DIAGNOSTIC-ONLY
test ! -e "$stage" && test ! -L "$stage"
test -f "$old" && test ! -L "$old" && [ "$(stat -c '%u:%h:%a' "$old")" = 0:1:755 ]
value=$(sha256sum "$old");[ "${value%% *}" = f8efe67a51d9685a9e7e3ce57267ed2ae4967600c2802dd21f5f6ee0c24a22c9 ]
umask 077
mkdir "$stage"
[ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
cp -p "$old" "$stage/pioneer-q38-BATCHED-OUTPUT-CLOSURE34-DIAGNOSTIC-ONLY"
value=$(sha256sum "$stage/pioneer-q38-BATCHED-OUTPUT-CLOSURE34-DIAGNOSTIC-ONLY")
printf '%s\n' "$value"
[ "${value%% *}" = f8efe67a51d9685a9e7e3ce57267ed2ae4967600c2802dd21f5f6ee0c24a22c9 ]
[ "$(stat -c '%u:%h:%a' "$stage/pioneer-q38-BATCHED-OUTPUT-CLOSURE34-DIAGNOSTIC-ONLY")" = 0:1:755 ]
echo PRIVATE_STAGE_CREATED_CLIENT_COPIED_NOT_EXECUTED
