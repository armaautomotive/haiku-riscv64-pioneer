#!/bin/sh
# Separate process,full SAME-OPEN original authentication again,CPU only.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
tool=/boot/home/pioneer-native-weight-cache-20261103-a/pioneer-q38-native-weight-cache
cache=/boot/home/models/.p100a-native-weights-v1
entry=$cache/p100a-weight-v1-443b4e6f1f5068b78e1b27f789994d399a567d9edb9db088025fe263a69337c4.cache
test -f "$tool" && test ! -L "$tool" && test -f "$entry" && test ! -L "$entry"
row=$(sha256sum "$tool");printf '%s\n' "$row"
[ "${row%% *}" = 264bc2842e74d38c3ca31cb549a9c8761a6e8fe9c7558e6073f188cb58604c7f ]
row=$(sha256sum "$entry");printf '%s\n' "$row"
[ "${row%% *}" = 89878c153dd0f9ff7f43546fe8c7210d1a7a85794f4841fe743fa63af690bb60 ]
[ "$(stat -c '%s:%u:%h:%a' "$entry")" = 348608:0:1:400 ]
date -u
"$tool" --build /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf "$cache" --tensor blk.0.ssm_alpha.weight
date -u
echo HAIKU_REAL_WEIGHT_CACHE_SEPARATE_REUSE_COMPLETED_CPU_ONLY
