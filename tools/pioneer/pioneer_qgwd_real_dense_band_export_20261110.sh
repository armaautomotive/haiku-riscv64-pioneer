#!/bin/sh
# CPU-only private Q8/BFP8 diagnostic samples; no accelerator access.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
stage=/boot/home/pioneer-qgwd-real-dense-band-20261110-a
evidence=/boot/home/pioneer-qgwd-dense-prepare-20261109-a
binary=$stage/pioneer-qgwd-dense-band-export
model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
cache=/HaikuData/pioneer-qgwd-dense-bfp8-20261109-a
test "$(uname -s)" = Haiku && test "$(id -u)" = 0
test -d "$stage/private" && test ! -L "$stage/private" && test "$(stat -c '%u:%a' "$stage/private")" = 0:700
test -f "$evidence/verify-exit-status" && test "$(/bin/cat "$evidence/verify-exit-status")" = 0 && test ! -s "$evidence/verify-stderr"
grep -Fx 'DENSE_Q8_CACHE_MATCH fragments=336 pages=304640 raw=1247805440 padded=1247805440; NO DEVICE OR SPEED' "$evidence/verify-stdout" >/dev/null
test -f "$binary" && test ! -L "$binary" && test -f "$model" && test ! -L "$model" && test -d "$cache" && test ! -L "$cache"
sum=$(sha256sum "$binary")
test "${sum%% *}" = f6028ada0e3eaad9d4e2d885b6a87841468e98eb0363af1f9422f1e425340703
test ! -e "$stage/exit-status" && test ! -L "$stage/exit-status"
set +e
"$binary" "$model" "$cache" "$stage/private"
status=$?
set -e
printf '%s\n' "$status" > "$stage/exit-status"
exit "$status"
