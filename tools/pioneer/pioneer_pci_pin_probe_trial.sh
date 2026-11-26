#!/bin/bash
# Node metadata only: no PCI configuration, MMIO, DMA, firmware or model IO.
set -euo pipefail
[[ $# == 2 && $1 == /* && $2 =~ ^[1-9][0-9]{0,8}$ ]] || exit 2
stage=$1 generation=$2
[[ $(uname -s):$(id -u) == Haiku:0 ]] || exit 2
driver=$stage/pioneer_pci_pin_probe
client=$stage/pci_pin_probe_client
bin=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/pioneer_pci_pin_probe
link=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/pioneer_pci_pin_probe
device=/dev/misc/pioneer_pci_pin_probe
lock=/boot/home/.pioneer-q38-head-csr-exclusive
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
driver_hash=8e334df3216b2613d55586bea7ef572cfddf1fb7d88e22e08f900fe7c3e42623
client_hash=cce14e9b03b37289cbb90bf08afaf4719bef0aa6bcd2b4a7b4a19308606885b4
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
if listimage 1 | grep -F pioneer_pci_pin_probe >/dev/null; then echo 'probe image already loaded' >&2; exit 1; fi
mkdir "$lock"
cleanup() {
 if [[ -L $link && $(readlink "$link") == ../../bin/pioneer_pci_pin_probe ]]; then rm "$link"; fi
 if hash_is "$bin" "$driver_hash"; then rm "$bin"; fi
 rmdir "$lock"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
idle
cp "$driver" "$bin"
hash_is "$bin" "$driver_hash"
ln -s ../../bin/pioneer_pci_pin_probe "$link"
for ((i=0; i<15; i++)); do
 [[ -e $device ]] && break
 sleep 1
done
[[ -e $device ]] || { echo 'probe device did not appear' >&2; exit 1; }
idle
"$client"
cleanup
trap - EXIT HUP INT TERM
for ((i=0; i<15; i++)); do [[ ! -e $device ]] && break; sleep 1; done
[[ ! -e $bin && ! -L $link && ! -e $device && ! -e $lock ]]
if listimage 1 | grep -F pioneer_pci_pin_probe >/dev/null; then echo 'probe image still loaded' >&2; exit 1; fi
idle
echo 'PASS pinned card/root-port metadata and cleanup; no PCI config or accelerator IO'
