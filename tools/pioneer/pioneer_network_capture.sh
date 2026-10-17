#!/bin/sh
# Temporary, read-only transfer diagnosis. No interface/boot/driver changes.
# Use an external 900-second timeout; stops after 60 samples or a failed probe.
# Output contains only the known wired interface and filtered network threads.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] || exit 2
q38_sample=0
q38_deadline=$(($(date +%s)+800))
while [ "$q38_sample" -lt 60 ]; do
 [ "$(date +%s)" -lt "$q38_deadline" ] || exit 1
 q38_sample=$((q38_sample+1))
 echo "PIONEER-NETWORK-SAMPLE=$q38_sample"
 date -u
 # This cannot block all later diagnostics if an interface ioctl wedges.
 q38_interface=0
 timeout -k 2 8 /bin/ifconfig /dev/net/rtl8125/1 || q38_interface=$?
 q38_probe=0
 timeout -k 2 5 /bin/ping -n -c 1 -t 2 -W 1000 10.0.0.211 || q38_probe=$?
 echo "PIONEER-NETWORK-STATUS ifconfig=$q38_interface ping_mac=$q38_probe"
 # ps -as supplies wait-semaphore names, not packet contents or personal logs.
 timeout -k 2 8 /bin/ps -as | awk '/^\/boot\/system\/bin\/awk / {next} /rtl8125|rge[0-9]|intr handler|net_server|fbsd|callout|fast taskq|ethernet/ {print}' || true
 if [ "$q38_interface" -ne 0 ] || [ "$q38_probe" -ne 0 ]; then
  echo PIONEER_NETWORK_FAILURE_SNAPSHOT_COMPLETE_NO_RESET
  exit 1
 fi
 sleep 5
done
echo PIONEER_NETWORK_BOUNDED_CAPTURE_COMPLETE_NO_RESET
