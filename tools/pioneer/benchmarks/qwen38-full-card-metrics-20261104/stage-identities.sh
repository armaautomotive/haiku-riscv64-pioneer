#!/bin/sh
set -eu
stage=/boot/home/pioneer-q38-full-metrics-20261104-a
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
for name in .trial-once driver.incoming tenstorrent_pci_probe-full-metrics90 HaikuAssistantQwen27FullOffload-metrics;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name"
done
for name in tenstorrent_pci_probe-native HaikuAssistantQwen27FullOffload pioneer-q38-full-offload-rate-smoke pioneer_q38_full_offload_rate_physical.sh;do
 test -f "$stage/$name" && test ! -L "$stage/$name"
 [ "$(stat -c '%u:%h' "$stage/$name")" = 0:1 ]
done
mv "$stage/tenstorrent_pci_probe-native" "$stage/tenstorrent_pci_probe-full-metrics90"
mv "$stage/HaikuAssistantQwen27FullOffload" "$stage/HaikuAssistantQwen27FullOffload-metrics"
for name in tenstorrent_pci_probe-full-metrics90 HaikuAssistantQwen27FullOffload-metrics pioneer-q38-full-offload-rate-smoke pioneer_q38_full_offload_rate_physical.sh;do
 chmod 700 "$stage/$name"
 sha256sum "$stage/$name"
 stat -c '%u:%h:%a:%s' "$stage/$name"
done
/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status
printf '%s\n' STAGED_ONLY_NO_DRIVER_EXCHANGE_OR_MODEL_EXECUTION
