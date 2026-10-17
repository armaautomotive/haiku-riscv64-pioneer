#!/bin/sh
# Bounded native NIC qualification,NOT a model/GPU test. Never reset the NIC.
# Public source --repair-test-fast serves three fixed leaves three times.
set -eu
[ "$#" = 1 ] || exit 2
case "$1" in ''|*[!0-9]*) exit 2;; esac
q38_port=$1
[ "$q38_port" -gt 0 ] && [ "$q38_port" -le 65535 ] || exit 2
[ "$(uname -s)" = Haiku ] || exit 1
cd /boot/home/pioneer-rtl-serialize-20261017-a
q38_driver=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/net/rtl8125
q38_receiver=/boot/home/models/pioneer_model_receive-198f33d7
[ "$(sha256sum "$q38_driver" | cut -d ' ' -f1)" = 0b83ceed0071f3dab3f9b175355bd83fe7364176676264ae66d751ddba2b7c9a ]
[ "$(sha256sum "$q38_receiver" | cut -d ' ' -f1)" = 198f33d7a53d0c33caadddccd4382b1e91cf249074ea883b145184d6652595fc ]
for q38_round in 1 2 3; do
 for q38_index in 0 3 4; do
  q38_file=fullrate-r$q38_round-leaf$q38_index-20261017-a
  [ ! -e "$q38_file" ] && [ ! -L "$q38_file" ]
 done
done
q38_deadline=$(($(date +%s)+800))
date -u
echo SERIALIZED_NIC_FULLRATE_START_9_FRESH_FILES_NO_RECOVERY_NO_GPU
/bin/ifconfig /dev/net/rtl8125/1
for q38_round in 1 2 3; do
 for q38_index in 0 3 4; do
  [ "$(date +%s)" -lt "$q38_deadline" ] || exit 1
  case "$q38_index" in
   0) q38_first=4656888560; q38_last=4723997423; q38_expected=b3bf92bd99d64b110113b07bb36edd0895e110c720147b1c4e7ee5084d3254c4;;
   3) q38_first=10361301118; q38_last=10428409981; q38_expected=883e6eaa3be7eacef0458886cbfe5e44c991357d1471d09c20af274e32bc477d;;
   4) q38_first=12419359320; q38_last=12486468183; q38_expected=8a2d90530aeef8f16a1c9ca1a019d6485f2b40c6abc94dee4d5ea772f00ec2e4;;
  esac
  q38_file=fullrate-r$q38_round-leaf$q38_index-20261017-a
  date -u
  echo "FULLRATE_ROUND=$q38_round LEAF=$q38_index;FRESH_ONLY"
  timeout -k 5 120 "$q38_receiver" "$q38_port" "$q38_first" "$q38_last" "$q38_file"
  [ -f "$q38_file" ] && [ ! -L "$q38_file" ]
  [ "$(wc -c < "$q38_file")" -eq 67108864 ]
  q38_hash=$(sha256sum "$q38_file"); q38_hash=${q38_hash%% *}
  echo "FULLRATE_SHA256=$q38_hash"
  [ "$q38_hash" = "$q38_expected" ] || { echo STOP_FULLRATE_SHA_NO_RETRY; exit 1; }
 done
done
sync
/bin/ifconfig /dev/net/rtl8125/1
date -u
echo SERIALIZED_NIC_FULLRATE_9_SHA_PASSES_603979776_BYTES_NO_RECOVERY_NO_GPU
