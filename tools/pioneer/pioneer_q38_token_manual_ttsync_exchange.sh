#!/bin/sh
# Exact stable041 <-> manual-completion c01 + USERaaff ONLY. Tag75 is NOT identity.
# No GPU job,force unload,reset,retry,cold-latch clearing or automatic rollback.
set -eu
[ "$#" = 1 ] || exit 2
case "$1" in --apply|--restore) mode=$1;; *) exit 2;; esac
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-p100a-token-manual-ttsync-20261103-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
candidate=$stage/tenstorrent_pci_probe-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY
client=$stage/pioneer-q38-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY
incoming=$stage/driver.incoming
backup=$stage/stable041.backup
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=c01f1cda7549ef3b191454ab6c21d697de143f3f11fcc5c9da48c81dd37c1736
client_hash=aaffcaa125a02d1629dc49055d45d26af28f3187049dda631a8b5a272a9c165a
guard_hash=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
admin_hash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 identity=$(stat -c '%u:%h:%a' "$1") || return 1
 case "$identity" in 0:1:755|0:1:644|0:1:555|0:1:700|0:1:600) ;; *) return 1;; esac
 value=$(sha256sum "$1") || return 1
 [ "${value%% *}" = "$2" ]
}
parents_idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
  $1=="Id" && $2=="Team" && NF==2 {header++;next}
  $1 ~ /^[0-9]+$/ && NF>=2 {
   teams++;if($1==self)next
   if($0 ~ /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|graph-csr-debug|source-check)/ ||
    $0 ~ /pioneer-p100a-batched-output-closure/ || $0 ~ /pioneer-p100a-wide-gdn/ || $0 ~ /pioneer-p100a-token-phase/ || $0 ~ /pioneer-p100a-token-constant-q4/ || $0 ~ /pioneer-p100a-token-matrix-phase/ || $0 ~ /pioneer-p100a-token-manual-ttsync/) owner++
   next
  }
  NF {invalid++}
  END {if(header!=1 || !teams || invalid || owner)exit 1}
 '
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
exact "$stable" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" \
 && exact "$guard" "$guard_hash" && exact "$admin" "$admin_hash" || exit 1
test ! -e "$incoming" && test ! -L "$incoming" || exit 1
mkdir "$stage/.exchange-lock" || exit 1
trap 'rmdir "$stage/.exchange-lock"' 0
if [ "$mode" = --apply ]; then
 source=$candidate;oldhash=$stable_hash;newhash=$candidate_hash
 oldtag=2026102100010012;newtag=2026103100010075
else
 source=$stable;oldhash=$candidate_hash;newhash=$stable_hash
 oldtag=2026103100010075;newtag=2026102100010012
fi
exact "$installed" "$oldhash" && parents_idle || exit 1
metadata=$("$admin" --status) || exit 1
printf '%s\n' "$metadata"
generation=$(printf '%s\n' "$metadata" | awk -v tag="tag$oldtag" '
 $1=="P100A_RELOAD" && $2=="driver55" && $3==tag && $5=="quiesced0" && $6=="cold0" &&
  $7=="active_owner0" && $8=="open_count1" && $9 ~ /^guard1;/ {
  value=$4;sub(/^generation/,"",value);if(value ~ /^[1-9][0-9]*$/ && NF==9){print value;good++;next}}
 NF {invalid++}
 END {if(good!=1 || invalid)exit 1}
') || exit 1
generationhex=$(printf '%016x' "$generation")
if test -e "$backup" || test -L "$backup"; then exact "$backup" "$stable_hash" || exit 1
else cp -p "$stable" "$backup";exact "$backup" "$stable_hash" || exit 1;fi
cp -p "$source" "$incoming";chmod 755 "$incoming"
exact "$incoming" "$newhash" && exact "$installed" "$oldhash" \
 && exact "$guard" "$guard_hash" && exact "$client" "$client_hash" && parents_idle || exit 1
"$admin" --quiesce "$oldtag" || exit 1
exact "$incoming" "$newhash" && exact "$installed" "$oldhash" && exact "$client" "$client_hash" || exit 1
# Same-filesystem atomic move from OUTSIDE the watched driver directory.
# Preserve any uncertain state; never undo a replacement automatically.
mv -f "$incoming" "$installed"
sync
exact "$installed" "$newhash" && exact "$guard" "$guard_hash" && exact "$client" "$client_hash" || exit 1
"$admin" --wait "$newtag" "$generationhex" || { echo STOP_UNVERIFIED_NO_AUTOMATIC_ROLLBACK_OR_RESET;exit 1; }
echo TOKEN_MANUAL_TTSYNC_EXCHANGE_COMPLETE_NO_GPU_EXECUTION
