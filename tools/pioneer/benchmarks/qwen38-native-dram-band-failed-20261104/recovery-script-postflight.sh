#!/bin/sh
set -eu
script=/boot/home/pioneer-native-dram-model-band-20261104-a/pioneer_native_dram_model_band_recovery_restore.sh
test -f "$script" && test ! -L "$script" && [ "$(stat -c '%u:%h' "$script")" = 0:1 ]
row=$(sha256sum "$script");printf '%s\n' "$row"
[ "${row%% *}" = 17f26da89df54a9fa1db5ec1bebc624e382858aeac321bc3ccce2f1d1fa0e9d6 ]
chmod 700 "$script"
echo FRESH_EPOCH_RESTORE_SCRIPT_PINNED_ONLY_NO_EXECUTION_YET
