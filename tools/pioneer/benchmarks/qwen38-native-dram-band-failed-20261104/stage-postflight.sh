#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-native-dram-model-band-20261104-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ]
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact "$stage/tenstorrent_pci_probe-native-dram-model-band81" 0ab145ac3b938238fd3fd139fa8c1e899458932058acd2567f220bb4504cdf27
exact "$stage/pioneer-native-dram-model-band-diagnostic" 0091244ae265e26075e80fa98f5d6b8b5c6d37304938e3d140d9bab6584e307e
exact "$stage/pioneer_native_dram_model_band_physical.sh" 577aa3cc051b036f815a94e519b98489fbc6793a4c0e47ee78e7f8539cc4650a
exact "$stage/learned-reference.log" 35142021d16a51e50b38657418800f3dcb102ddadc3961ec680d16ebf693ce83
chmod 700 "$stage/tenstorrent_pci_probe-native-dram-model-band81" "$stage/pioneer-native-dram-model-band-diagnostic" "$stage/pioneer_native_dram_model_band_physical.sh"
chmod 600 "$stage/learned-reference.log"
for name in .trial-once stable041.backup driver.incoming gddr.stdout gddr.stderr gddr.exit parent.pid state cli-refusal.stdout cli-refusal.stderr;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name"
done
if "$stage/pioneer-native-dram-model-band-diagnostic" --no-device-cli-probe > "$stage/cli-refusal.stdout" 2> "$stage/cli-refusal.stderr";then result=0;else result=$?;fi
[ "$result" = 2 ] && test ! -s "$stage/cli-refusal.stdout"
[ "$(cat "$stage/cli-refusal.stderr")" = 'Use --fixed-gddr-learned-bands for three fixed DEBUG samples,NOT model/Work inference.' ]
echo EXPECTED_CLI_REFUSAL_EXIT2_NATIVE_CLIENT_LOADER_WORKS_NO_SOURCE_OR_DEVICE_OPEN
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
[ "$metadata" = 'P100A_RELOAD driver55 tag2026102100010012 generation10 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
echo PRIVATE_STAGING_KNOWN_CLEAN_NORMAL_DRIVER_UNCHANGED_NO_GPU_RUN
