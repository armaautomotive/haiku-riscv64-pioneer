#!/bin/sh
# Run ONLY after SAME physical parent has ACTUALLY terminated. No GPU calls.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-native-dram-model-band-20261104-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
for name in state parent.pid gddr.stdout gddr.stderr gddr.exit;do
 test -f "$stage/$name" && test ! -L "$stage/$name" && [ "$(stat -c '%u:%h' "$stage/$name")" = 0:1 ]
 stat -c '%n %u:%h:%a:%s' "$stage/$name"
 sha256sum "$stage/$name"
done
printf 'PHYSICAL_CHILD_EXIT ';cat "$stage/gddr.exit"
printf 'PHYSICAL_PARENT_STATE ';cat "$stage/state"
/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status
sha256sum /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 /boot/home/config/non-packaged/servers/HaikuAssistantService
sysinfo -cpu
sysinfo -mem
echo POSTTERMINAL_READONLY_RECORD_COLLECTION_NO_GPU_EXECUTION_OR_WRITES
