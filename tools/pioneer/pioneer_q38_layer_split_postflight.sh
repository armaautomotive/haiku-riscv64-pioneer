#!/bin/bash
# Read-only, exact closing verification for tag213 generation3. Run only
# after the detached parent exits; refuse even metadata open if a team lives.
set -euo pipefail
stage=/boot/home/pioneer-q38-layer-split-candidate-213
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
exact(){ test -f "$2" && test ! -L "$2" && printf '%s  %s\n' "$1" "$2" | sha256sum -c -; }
test "$(uname -s)" = Haiku && test "$(id -u)" = 0
test -d "$stage" && test ! -L "$stage" && test "$(stat -c '%u:%a' "$stage")" = 0:700
cd "$stage"
for f in state rate.exit parent.exit rate.stdout rate.stderr parent.stdout parent.stderr run-once.sh launch.pid; do
	test -f "$f" && test ! -L "$f"
done
printf '0\n' | cmp -s rate.exit -
printf '0\n' | cmp -s parent.exit -
printf 'STABLE041_GEN4_RESTORED\n' | cmp -s state -
printf '621\n' | cmp -s launch.pid -
test ! -s rate.stderr && test ! -s parent.stderr
for f in /boot/home/.pioneer-q38-head-csr-exclusive candidate.incoming restore.incoming; do
	test ! -e "$f" && test ! -L "$f"
done
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk -v self="$$" '
$1=="Id" && $2=="Team" && NF==2 {header++;next}
$1~/^[0-9]+$/ && NF>=2 {
 teams++; if($1==self)next
 if($1==621 || $0~/(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|pioneer-q|pioneer-native-|pioneer_q38)/)bad++
 next
}
NF{bad++} END{if(header!=1 || !teams || bad)exit 1}'
exact b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f "$admin"
exact d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 "$guard"
exact 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 "$installed"
exact 4b6f208da0ed2960d7d3e9a6617f3e354737b9ec4b7b7c4cf0c62d25a3fbd9ec tenstorrent_pci_probe-native
exact 5c650f81142f5fc38c8f6ac24cd62ef9c4136b65144a750c47a8c8a01af148f6 HaikuAssistantQwen27FullOffload
exact ec90ba4575b78457c64d72a43846346b64923dc16e7aee1e1b6a808361a37889 pioneer-q38-full-offload-rate-smoke
exact 3e3f76f2b54a22d1ab3fbfda0799680cd554b09f4fa44c2b6023d29383e804b2 manifest.json
exact 1e7cdd1238593c7b317ea725e722574ed600dea10dc0b4dc9ee19d931f82e01c run-once.sh
status=$("$admin" --status)
printf '%s\n' "$status"
test "$status" = 'P100A_RELOAD driver55 tag2026102100010012 generation4 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
sysinfo | awk '/^64 .*running at/{cpus++;print} /used\/max [0-9]+ \/ 137034936320\)/{ram++;print} END{if(cpus!=1||ram!=1)exit 1}'
test "$(stat -c '%u:%h:%a:%s' /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf)" = 0:1:600:18973870528
sha256sum rate.stdout rate.stderr rate.exit parent.stdout parent.stderr parent.exit state run-once.sh launch.pid
