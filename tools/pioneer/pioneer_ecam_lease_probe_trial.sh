#!/bin/bash
# Read-only PCI/FDT profile and fixed no-op lease: no config writes, MMIO, DMA or firmware.
set -euo pipefail
[[ $# == 3 && $1 == /* && $2 =~ ^[1-9][0-9]{0,8}$ && ( $3 == absent || $3 == present || $3 == contention-absent || $3 == contention ) ]] || exit 2
stage=$1 generation=$2 mode=$3
[[ $(uname -s):$(id -u) == Haiku:0 ]] || exit 2
driver=$stage/pioneer_ecam_lease_probe
client=$stage/ecam_lease_probe_client
contention_client=$stage/ecam_lease_contention_client
bin=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/pioneer_ecam_lease_probe
link=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/pioneer_ecam_lease_probe
device=/dev/misc/pioneer_ecam_lease_probe
lock=/boot/home/.pioneer-q38-head-csr-exclusive
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
driver_hash=709d0962a7d5da7c8aed6f3bc76fe1e5a503920da6713509f85140173f1a69a3
client_hash=94963c46d47529f8be34354bbf123337bf813bd25514214fcaa0e0e80c765ebf
contention_client_hash=25a04b825db7f6390026cc4dd34780e4b9c36ce4156de441bf9aee95f31e8de4
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
if [[ $mode == contention ]]; then hash_is "$contention_client" "$contention_client_hash"; fi
[[ ! -e $bin && ! -L $bin && ! -e $link && ! -L $link && ! -e $device && ! -e $lock && ! -L $lock ]]
if listimage 1 | grep -F pioneer_ecam_lease_probe >/dev/null; then echo 'probe image already loaded' >&2; exit 1; fi
mkdir "$lock"
retain=0
cleanup() {
 if [[ $retain == 1 ]]; then echo 'CONTENTION_PROBE_RETAINED: inspect live threads and driver before cleanup' >&2; return; fi
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
case "$mode" in
 absent) "$client" --expect-absent;;
 present) "$client";;
 contention-absent) "$client" --expect-contention-absent;;
 contention)
  retain=1
  result=0
  "$contention_client" || result=$?
  if [[ $result == 0 || $result == 1 ]]; then retain=0; fi
  [[ $result == 0 ]] || exit "$result"
  ;;
esac
cleanup
trap - EXIT HUP INT TERM
for ((i=0; i<15; i++)); do [[ ! -e $device ]] && break; sleep 1; done
[[ ! -e $bin && ! -L $link && ! -e $device && ! -e $lock ]]
if listimage 1 | grep -F pioneer_ecam_lease_probe >/dev/null; then echo 'probe image still loaded' >&2; exit 1; fi
idle
echo "PASS fixed ECAM lease $mode and cleanup; no PCI writes or accelerator execution"
