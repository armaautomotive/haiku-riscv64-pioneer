#!/bin/sh
# CPU-only real-model cache preparation. Does not access the P100A or driver.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
stage=/boot/home/pioneer-qgwd-dense-prepare-20261109-a
cache=/HaikuData/pioneer-qgwd-dense-bfp8-20261109-a
binary=$stage/pioneer-q38-hybrid-bfp8-prepare
model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test "$(uname -s)" = Haiku && test "$(id -u)" = 0
test -d "$stage" && test ! -L "$stage" && test ! -e "$cache" && test ! -L "$cache"
test -f "$binary" && test ! -L "$binary" && test -f "$model" && test ! -L "$model"
test "$(stat -c '%u:%h:%a:%s' "$model")" = 0:1:600:18973870528
sum=$(sha256sum "$binary")
test "${sum%% *}" = 6b8082f8429c82485a41de53d4b7af8150101af6475c3c2e56171b570d296935
test ! -e "$stage/exit-status" && test ! -L "$stage/exit-status"
set +e
"$binary" --build "$model" "$cache"
status=$?
set -e
printf '%s\n' "$status" > "$stage/exit-status"
exit "$status"
