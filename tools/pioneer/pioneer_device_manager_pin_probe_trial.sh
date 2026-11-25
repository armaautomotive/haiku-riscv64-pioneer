#!/bin/sh
# Haiku-only, card-independent temporary driver qualification. No PCI access.
set -eu
[ "$#" = 2 ] || { echo 'usage: probe_trial ABSOLUTE_STAGE absent|present' >&2; exit 2; }
stage=$1
mode=$2
case "$stage" in /*) ;; *) exit 2;; esac
case "$mode" in absent|present) ;; *) exit 2;; esac
[ "$(uname -s)" = Haiku ] || exit 2
driver=$stage/pioneer_device_manager_pin_probe
client=$stage/pin_probe_client
bin=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/pioneer_device_manager_pin_probe
link=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/pioneer_device_manager_pin_probe
device=/dev/misc/pioneer_dm_pin_probe
driver_hash=ef04ada6a47cfbf80b5255d06b0a96b69697e3500d2b4d8775a605735f7cf180
client_hash=b9538dc24c2ee076cbceb3418d73237f7b5824f5967cbdc97bf48b5f92645b9a
hash_is() { [ -f "$1" ] && [ ! -L "$1" ] && [ "$(sha256sum "$1" | cut -d ' ' -f1)" = "$2" ]; }
hash_is "$driver" "$driver_hash" && hash_is "$client" "$client_hash"
[ ! -e "$bin" ] && [ ! -L "$bin" ] && [ ! -e "$link" ] && [ ! -L "$link" ] && [ ! -e "$device" ]
if listimage 1 | grep -F pioneer_device_manager_pin_probe >/dev/null; then
 echo 'probe image already loaded' >&2; exit 1
fi
cleanup() {
 if [ -L "$link" ] && [ "$(readlink "$link")" = ../../bin/pioneer_device_manager_pin_probe ]; then rm "$link"; fi
 if hash_is "$bin" "$driver_hash"; then rm "$bin"; fi
}
trap cleanup EXIT HUP INT TERM
cp "$driver" "$bin"
hash_is "$bin" "$driver_hash"
ln -s ../../bin/pioneer_device_manager_pin_probe "$link"
i=0
while [ "$i" -lt 15 ] && [ ! -e "$device" ]; do sleep 1; i=$((i+1)); done
[ -e "$device" ] || { echo 'probe device did not appear' >&2; exit 1; }
if [ "$mode" = absent ]; then "$client" --expect-absent; else "$client"; fi
cleanup
trap - EXIT HUP INT TERM
i=0
while [ "$i" -lt 15 ] && [ -e "$device" ]; do sleep 1; i=$((i+1)); done
[ ! -e "$bin" ] && [ ! -L "$link" ] && [ ! -e "$device" ]
if listimage 1 | grep -F pioneer_device_manager_pin_probe >/dev/null; then
 echo 'probe image still loaded' >&2; exit 1
fi
echo "PASS probe $mode and cleanup; no accelerator access"
