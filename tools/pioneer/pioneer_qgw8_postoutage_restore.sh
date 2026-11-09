#!/bin/sh
# Explicit, once-only stable-driver recovery after a verified fresh power epoch.
# The backup hash argument is an OPERATOR ATTESTATION: this script cannot check
# the Mac backup itself. Never retry after an uncertain quiesce or exchange.
set -eu
PATH=/boot/system/bin:/bin
export PATH
[ "$#" = 4 ] && { [ "$1" = --check-only ] || [ "$1" = --restore ]; } && \
 [ "$2" = --attest-mac-backup ] && \
 printf '%s\n' "$3" | grep -Eq '^[0-9a-f]{64}$' && [ "$4" = --attest-fresh-power-cycle ] && \
 [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
action=$1
umask 077
stage=/boot/home/pioneer-qgw8-hidden-trial-20261108-a
work=/boot/home/pioneer-qgw8-postoutage-restore-20261108-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
stable=$stage/stable.backup
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush
old=0b61c939d9b461a6c60e0d7f68a2c3bccac588ea052dc634d85b9a35d32b929f
new=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555|0:1:600) ;; *) return 1;; esac
 value=$(sha256sum "$1") || return 1; [ "${value%% *}" = "$2" ]
}
records(){
 exact "$stage/state" 75e078802fd23010cf3d8e76e2fe9f72410fd9ec1779a9b9afa9f2824b7a1660 && \
 exact "$stage/parent.stdout" b09b7f8e1b55da9bbd282ca13fa99edebe836b0ac6a3e51b75280a5adeae4ba2 && \
 exact "$stage/parent.stderr" e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 && \
 exact "$stage/trial.stdout" 628efcfc3f4a11a3213f9f9286a95488c82faf9206c9e6abb8ee257bcee7b0ab && \
 exact "$stage/trial.stderr" e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 && \
 test ! -e "$stage/trial.exit" && test ! -L "$stage/trial.exit" && \
 test ! -e "$stage/hidden.bin" && test ! -L "$stage/hidden.bin"
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/ || $0 ~ /(upload-test|preflight-test|full-test)/)owners++
  next}
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1; printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -d "$stage/.trial-once" && test ! -L "$stage/.trial-once" || exit 1
test ! -e "$work" && test ! -L "$work" || exit 1
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
records && exact "$installed" "$old" && exact "$stage/tenstorrent_pci_probe-qgw8-hidden-129" "$old" && exact "$stable" "$new" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f && \
 exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 && \
 exact "$flush" 242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d || exit 1
health=$(sysinfo) || exit 1
printf '%s\n' "$health" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}' || exit 1
idle && metadata 2026110700010129 1 || exit 1
if [ "$action" = --check-only ]; then
 echo QGW8_POSTOUTAGE_CHECK_ONLY_BACKUP_ATTESTATION_NOT_VERIFIED
 exit 0
fi
mkdir "$work"
trap 'code=$?; if [ "$code" -ne 0 ]; then echo "RESTORE_FAILED_${code}_NO_RETRY_RESET_OR_ROLLBACK" >&2; fi' EXIT
cp -p "$installed" "$work/previous129"
cp -p "$stable" "$work/incoming041"
chmod 755 "$work/incoming041"
exact "$work/previous129" "$old" && exact "$work/incoming041" "$new" && exact "$installed" "$old" && records && idle || exit 1
"$flush" --flush-only /dev/disk/scsi/0/3/0/0 || exit 1
"$admin" --quiesce 2026110700010129 || exit 1
exact "$work/incoming041" "$new" && exact "$installed" "$old" || exit 1
mv -f "$work/incoming041" "$installed"
exact "$installed" "$new" && exact "$work/previous129" "$old" || exit 1
"$flush" --flush-only /dev/disk/scsi/0/3/0/0 || exit 1
"$admin" --wait 2026102100010012 0000000000000001 || exit 1
metadata 2026102100010012 2 && idle && records || exit 1
echo QGW8_POSTOUTAGE_STABLE041_RESTORED_NO_ACCELERATOR_EXECUTION
