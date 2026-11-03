#!/bin/sh
# Bounded derived-weight CPU/disk operation. No driver or GPU access.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
tool=/boot/home/pioneer-native-weight-cache-20261103-a/pioneer-q38-native-weight-cache
cache=/boot/home/models/.p100a-native-weights-v1
test -f "$tool" && test ! -L "$tool"
row=$(sha256sum "$tool");printf '%s\n' "$row"
[ "${row%% *}" = 264bc2842e74d38c3ca31cb549a9c8761a6e8fe9c7558e6073f188cb58604c7f ]
test ! -e "$cache" && test ! -L "$cache"
date -u
"$tool" --build /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf "$cache" --tensor blk.0.ssm_alpha.weight
date -u
echo HAIKU_REAL_WEIGHT_CACHE_CREATE_COMPLETED_CPU_ONLY
