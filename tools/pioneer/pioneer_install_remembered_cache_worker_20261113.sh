#!/bin/sh
# One guarded worker replacement, only after the isolated trial closed cleanly.
# Keeps a verified copy of the previous worker; does not reload the driver.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-remembered-cache-20261113-a
installed=/boot/home/config/non-packaged/servers/HaikuAssistantQwen27
candidate=$stage/worker
backup=$stage/installed-worker-before
temporary=/boot/home/config/non-packaged/servers/.HaikuAssistantQwen27-remembered-20261113
driver=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistantQwen27|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/)owners++;
  next}
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
[ "$(cat "$stage/state")" = CARD_CLOSED_WORK_REQUEST_VERIFIED ] && [ "$(cat "$stage/card.exit")" = 0 ] \
 && [ "$(cat "$stage/host.exit")" = 0 ] && test ! -s "$stage/card.stderr" && test ! -s "$stage/host.stderr" || exit 1
test -f "$stage/card.stdout" && test ! -L "$stage/card.stdout" && [ "$(stat -c '%u:%h' "$stage/card.stdout")" = 0:1 ] || exit 1
output_sha=$(sha256sum "$stage/card.stdout")
[ "${output_sha%% *}" = 790682a994f81713af89b3ad3cc94a3a34ed8e2209cd80583db56e7af974b1d1 ] || exit 1
exact "$candidate" 93f1d09ba0b2e0d0ea73642ccc89bdc5d0bf0cbed4be0ce3bbd47ea94d24008f || exit 1
exact "$installed" 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact "$driver" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
[ "$("$admin" --status)" = 'P100A_RELOAD driver55 tag2026102100010012 generation42 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ] || exit 1
idle || exit 1
test ! -e "$backup" && test ! -L "$backup" && test ! -e "$temporary" && test ! -L "$temporary" || exit 1
mkdir "$stage/.install-once"
cp -p "$installed" "$backup"
exact "$backup" 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
cp "$candidate" "$temporary"
chmod 755 "$temporary"
exact "$temporary" 93f1d09ba0b2e0d0ea73642ccc89bdc5d0bf0cbed4be0ce3bbd47ea94d24008f || exit 1
idle && exact "$installed" 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
mv -f "$temporary" "$installed"
exact "$installed" 93f1d09ba0b2e0d0ea73642ccc89bdc5d0bf0cbed4be0ce3bbd47ea94d24008f || exit 1
exact "$driver" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 || exit 1
echo REMEMBERED_CACHE_WORKER_INSTALLED_BACKUP_VERIFIED_NO_DRIVER_CHANGE
