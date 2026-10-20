#!/bin/sh
# One checkpoint's guarded application-only deployment. NO driver/reboot/SD
# operations; no automatic rollback after any unknown accelerator ownership.
set -eu
[ "$#" = 1 ] && [ "$1" = --apply ] && [ "$(uname -s)" = Haiku ] || exit 2
umask 077
stage=/boot/home/pioneer-assistant-qwen27-20261020-a
base=/boot/home/config/non-packaged
hash() { actual=$(sha256sum "$1"); actual=${actual%% *}; [ "$actual" = "$2" ]; }
regular() { [ -f "$1" ] && [ ! -L "$1" ]; }
check() { regular "$stage/$1" && hash "$stage/$1" "$2"; }
check HaikuAssistant27-20261020-c 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933
check HaikuAssistant27-20261020-fService 8e3dc4a401f7c986c31ffa00e521fba01fbda40245278c7d429ce60030ca9b84
check HaikuAssistant27-20261020-cControl 88b4ee3ff9c66ab065ec5464f90d6f369ff00dfb4c6c67c4a1eb004eb225e138
check HaikuAssistant27-20261020-cHardware 997ffa12f9760ef91b7252d8c944a92716bf7b07bba3aa221bdccbd50be26f00
check HaikuAssistantQwen27-20261020-f d59413e5354fd3d2ba02851a28ed553407bc5f3cdf8ab37dc118574afac1e398
check HaikuAssistant27-20261020-fHardwareTest c53d212846de6c6ddf476c91d9ba88b607aabdc0d354973bcbf12ffd853ae9b6
check HaikuAssistant27-20261020-cModelStoreTest 8d0a856f261d8e7501458287493cdbe2b65054f549134f53190341d3efbce5c7
check Qwen27NativeFinalFakeWorker 50f51abdb5b6467885fad0147779fe3b42e5da2ca13ea2ff2e92bdc73dfa1133
check Qwen27NativeFinalTest 87a3ba64e8c16c3fee320101461bb81355ceabed1cad0b66c3c1dd83b0b303fe
check Qwen27NativeFinalCpuPoolTest fa0a75e243d062ae7f9bdd5d46eca63f983f37064c71693822e82e61c1df7438
hash "$base/add-ons/kernel/drivers/bin/tenstorrent_pci_probe" e903b8fcee619edf4e40457aed16e0387286a73c7606b4796e5b3961b275e498
hash "$base/add-ons/kernel/generic/pioneer_p100a_reload_guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
guard=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
hash "$guard" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
state=$("$guard" --status)
[ "$state" = 'P100A_RELOAD driver55 tag202610190006000d generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ] || exit 1
if /bin/ps | /bin/grep -E '[H]aikuAssistant|[p]ioneer_q38_inference'; then
 echo 'STOP: close Assistant/worker normally first; no process is killed.'; exit 1
fi
libraries=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
hash "$libraries/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af
hash "$libraries/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16
hash "$libraries/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389
hash "$libraries/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2
export LIBRARY_PATH="$libraries:/boot/system/lib"
"$stage/HaikuAssistant27-20261020-fHardwareTest"
"$stage/HaikuAssistant27-20261020-cModelStoreTest"
"$stage/Qwen27NativeFinalTest"
"$stage/Qwen27NativeFinalCpuPoolTest"
# Validate ALL targets before backing up or replacing ANY. No links or
# unexpected directories. The stopped application can be atomically replaced.
for target in apps/HaikuAssistant apps/HaikuAssistantHardware servers/HaikuAssistantService servers/HaikuAssistantQwen27 bin/assistantctl; do
 [ ! -L "$base/$target" ] || exit 1
 [ ! -e "$base/$target" ] || regular "$base/$target"
 [ ! -e "$base/$target.qwen27-new" ] && [ ! -L "$base/$target.qwen27-new" ] || exit 1
done
[ ! -e "$stage/before" ] && [ ! -L "$stage/before" ] || exit 1
mkdir -m 700 "$stage/before"
for target in apps/HaikuAssistant apps/HaikuAssistantHardware servers/HaikuAssistantService servers/HaikuAssistantQwen27 bin/assistantctl; do
 if [ -e "$base/$target" ]; then cp -p "$base/$target" "$stage/before/${target##*/}"; fi
done
settings=/boot/home/config/settings/HaikuAssistant_settings
if [ -e "$settings" ]; then regular "$settings"; cp -p "$settings" "$stage/before/HaikuAssistant_settings"; fi
# Worker first: it accepts the old parent's protocol subset. Never expose
# a new thread-command parent alongside an old worker lacking that command.
for pair in HaikuAssistantQwen27-20261020-f:servers/HaikuAssistantQwen27 HaikuAssistant27-20261020-c:apps/HaikuAssistant HaikuAssistant27-20261020-cHardware:apps/HaikuAssistantHardware HaikuAssistant27-20261020-fService:servers/HaikuAssistantService HaikuAssistant27-20261020-cControl:bin/assistantctl; do
 source=${pair%%:*}; target=${pair#*:}
 cp -p "$stage/$source" "$base/$target.qwen27-new"
 chmod 755 "$base/$target.qwen27-new"
 cmp "$stage/$source" "$base/$target.qwen27-new"
 mv "$base/$target.qwen27-new" "$base/$target"
done
echo 'Installed application-only Qwen27B option; backups retained. Driver/runtime/model/boot unchanged. Assistant may now be reopened.'
