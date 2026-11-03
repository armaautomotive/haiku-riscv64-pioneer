#!/bin/sh
# Install ONE new CPU-only program. No driver,boot,Assistant or model change.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
source=/boot/home/pioneer-native-weight-cache-20261103-a/pioneer-q38-native-weight-cache
target=/boot/home/config/non-packaged/bin/pioneer-q38-native-weight-cache
for directory in /boot/home/config /boot/home/config/non-packaged /boot/home/config/non-packaged/bin; do
  test -d "$directory" && test ! -L "$directory"
  [ "$(stat -c '%u' "$directory")" = 0 ]
done
test -f "$source" && test ! -L "$source"
row=$(sha256sum "$source"); [ "${row%% *}" = 264bc2842e74d38c3ca31cb549a9c8761a6e8fe9c7558e6073f188cb58604c7f ]
test ! -e "$target" && test ! -L "$target"
umask 077
cp -n "$source" "$target"
test -f "$target" && test ! -L "$target"
row=$(sha256sum "$target"); printf '%s\n' "$row"
[ "${row%% *}" = 264bc2842e74d38c3ca31cb549a9c8761a6e8fe9c7558e6073f188cb58604c7f ]
chmod 755 "$target"
[ "$(stat -c '%u:%h:%a' "$target")" = 0:1:755 ]
stat -c '%F %s %u %h %a' /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
echo NEW_CPU_CACHE_PROGRAM_INSTALLED_ORIGINAL_GGUF_ASSISTANT_DRIVER_UNCHANGED
