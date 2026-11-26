#!/bin/bash
# Read-only PCI/FDT profile and fixed no-op lease: no config writes, MMIO, DMA or firmware.
set -euo pipefail
[[ $# == 3 && $1 == /* && $2 =~ ^[1-9][0-9]{0,8}$ && ( $3 == absent || $3 == present ) ]] || exit 2
stage=$1 generation=$2 mode=$3
[[ $(uname -s):$(id -u) == Haiku:0 ]] || exit 2
driver=$stage/pioneer_ecam_lease_probe
client=$stage/ecam_lease_probe_client
bin=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/pioneer_ecam_lease_probe
link=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/pioneer_ecam_lease_probe
device=/dev/misc/pioneer_ecam_lease_probe
lock=/boot/home/.pioneer-q38-head-csr-exclusive
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
driver_hash=6741d1fbe4fad63944f2cc4c88d2c13918f7582338eb535a7482439e98381e7f
client_hash=02b86c8824f108767e6b9b2cc371ce0b30c06e24a49db685b99fcb0fbf38b733
admin_hash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
hash_is() { [[ -f $1 && ! -L $1 && $(sha256sum "$1" | cut -d ' ' -f1) == "$2" ]]; }
idle() {
 local status listing
 status=$("$admin" --status)
 printf '%s\n' "$status"
 [[ $status == "P100A_RELOAD driver55 tag2026102100010012 generation$generation quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]]
 listing=$(ps -o Id -o Team)
 printf '%s\n' "$listing" | awk -v self="$$" '
  $1=="Id" && $2=="Team" && NF==2 {header++; next}
  $1~/^[0-9]+$/ && NF>=2 {teams++; if($1!=self && $2~/(^|\/)(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram)/) bad++; next}
  NF {bad++}
  END {exit !(header==1 && teams && !bad)}'
}
hash_is "$driver" "$driver_hash" && hash_is "$client" "$client_hash" && hash_is "$admin" "$admin_hash"
[[ ! -e $bin && ! -L $bin && ! -e $link && ! -L $link && ! -e $device && ! -e $lock && ! -L $lock ]]
if listimage 1 | grep -F pioneer_ecam_lease_probe >/dev/null; then echo 'probe image already loaded' >&2; exit 1; fi
mkdir "$lock"
cleanup() {
 if [[ -L $link && $(readlink "$link") == ../../bin/pioneer_ecam_lease_probe ]]; then rm "$link"; fi
 if hash_is "$bin" "$driver_hash"; then rm "$bin"; fi
 rmdir "$lock"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
idle
cp "$driver" "$bin"
hash_is "$bin" "$driver_hash"
ln -s ../../bin/pioneer_ecam_lease_probe "$link"
for ((i=0; i<15; i++)); do
 [[ -e $device ]] && break
 sleep 1
done
[[ -e $device ]] || { echo 'probe device did not appear' >&2; exit 1; }
idle
if [[ $mode == absent ]]; then "$client" --expect-absent; else "$client"; fi
cleanup
trap - EXIT HUP INT TERM
for ((i=0; i<15; i++)); do [[ ! -e $device ]] && break; sleep 1; done
[[ ! -e $bin && ! -L $link && ! -e $device && ! -e $lock ]]
if listimage 1 | grep -F pioneer_ecam_lease_probe >/dev/null; then echo 'probe image still loaded' >&2; exit 1; fi
idle
echo "PASS fixed ECAM lease $mode and cleanup; no PCI writes or accelerator execution"
