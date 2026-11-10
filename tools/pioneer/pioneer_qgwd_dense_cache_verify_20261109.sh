#!/bin/sh
# Read-only real-model cache verification; no P100A/driver access.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
stage=/boot/home/pioneer-qgwd-dense-prepare-20261109-a
cache=/HaikuData/pioneer-qgwd-dense-bfp8-20261109-a
checker=$stage/pioneer-qgwd-dense-cache-check-reproducible
model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test "$(uname -s)" = Haiku && test "$(id -u)" = 0
test -d "$stage" && test ! -L "$stage" && test -d "$cache" && test ! -L "$cache"
test -f "$stage/exit-status" && test "$(/bin/cat "$stage/exit-status")" = 0 && test ! -s "$stage/stderr"
grep -Fx 'PASS_DENSE_Q8_BFP8_PREPARATION 48 selected tensors, frozen all-BFP8 recipe; NO DEVICE OR SPEED' "$stage/stdout" >/dev/null
test -f "$checker" && test ! -L "$checker" && test -f "$model" && test ! -L "$model"
sum=$(sha256sum "$checker")
test "${sum%% *}" = 06b0631f5f5fc86ae128e0340629952ebad2a23b64d62c6db96b03af7a5c14d4
test ! -e "$stage/verify-exit-status" && test ! -L "$stage/verify-exit-status"
set +e
"$checker" --verify-dense-q8-bfp8 "$model" "$cache"
status=$?
set -e
printf '%s\n' "$status" > "$stage/verify-exit-status"
exit "$status"
