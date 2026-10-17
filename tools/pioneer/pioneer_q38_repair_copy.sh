#!/bin/sh
# One-shot copy of THREE fixed public-model repair leaves. No GPU/boot work.
# Fresh files only; no append/resume. At most TWO temporary NIC recoveries,
# only AFTER the receiver has exited AND the authenticated source is unreachable.
set -eu
[ "$#" = 1 ] || exit 2
case "$1" in ''|*[!0-9]*) exit 2;; esac
q38_port=$1
[ "$q38_port" -gt 0 ] && [ "$q38_port" -le 65535 ] || exit 2
[ "$(uname -s)" = Haiku ] || exit 1
cd /boot/home/models/qwen38-parallel-20261017
q38_receiver=/boot/home/models/pioneer_model_receive-198f33d7
q38_hash=$(sha256sum "$q38_receiver"); q38_hash=${q38_hash%% *}
[ "$q38_hash" = 198f33d7a53d0c33caadddccd4382b1e91cf249074ea883b145184d6652595fc ]
[ -f "$q38_receiver" ] && [ ! -L "$q38_receiver" ]
q38_url=http://10.0.0.211:$q38_port/qwen38-c600de0300ae8a0e.gguf
q38_deadline=$(($(date +%s)+800))
q38_recoveries=0
for q38_index in 0 3 4; do
 q38_final=leaf$q38_index-verified-repair-20261017-b
 [ ! -e "$q38_final" ] && [ ! -L "$q38_final" ]
 for q38_attempt in 1 2 3; do
  q38_file=leaf$q38_index-fresh-repair-20261017-b$q38_attempt
  [ ! -e "$q38_file" ] && [ ! -L "$q38_file" ]
 done
done
for q38_index in 0 3 4; do
 case "$q38_index" in
  0) q38_first=4656888560; q38_last=4723997423; q38_expected=b3bf92bd99d64b110113b07bb36edd0895e110c720147b1c4e7ee5084d3254c4;;
  3) q38_first=10361301118; q38_last=10428409981; q38_expected=883e6eaa3be7eacef0458886cbfe5e44c991357d1471d09c20af274e32bc477d;;
  4) q38_first=12419359320; q38_last=12486468183; q38_expected=8a2d90530aeef8f16a1c9ca1a019d6485f2b40c6abc94dee4d5ea772f00ec2e4;;
 esac
 q38_complete=0
 for q38_attempt in 1 2 3; do
  [ "$(date +%s)" -lt "$q38_deadline" ] || exit 1
  q38_file=leaf$q38_index-fresh-repair-20261017-b$q38_attempt
  q38_final=leaf$q38_index-verified-repair-20261017-b
  date -u
  echo "FRESH_LEAF=$q38_index ATTEMPT=$q38_attempt;NO_APPEND_NO_GPU"
  if timeout -k 5 120 "$q38_receiver" "$q38_port" "$q38_first" "$q38_last" "$q38_file"; then
   [ -f "$q38_file" ] && [ ! -L "$q38_file" ]
   [ "$(wc -c < "$q38_file")" -eq 67108864 ]
   q38_hash=$(sha256sum "$q38_file"); q38_hash=${q38_hash%% *}
   echo "LEAF${q38_index}_SHA256=$q38_hash"
   [ "$q38_hash" = "$q38_expected" ] || { echo STOP_LEAF_SHA_NO_PATCH; exit 1; }
   [ ! -e "$q38_final" ] && [ ! -L "$q38_final" ]
   mv -n "$q38_file" "$q38_final"
   [ ! -e "$q38_file" ] && [ ! -L "$q38_file" ]
   q38_complete=1
   break
  fi
  echo INTERRUPTED_FRESH_LEAF_PRESERVED_NO_APPEND
  # Do not probe/reset during an active transfer: the source is single-request.
  if timeout -k 2 8 wget --inet4-only --spider --max-redirect=0 --timeout=5 --tries=1 --no-verbose "$q38_url"; then
   echo SOURCE_REACHABLE_NO_INTERFACE_RESET
  else
   [ "$q38_recoveries" -lt 2 ] || { echo STOP_WIRED_RECOVERY_BOUND; exit 1; }
   q38_recoveries=$((q38_recoveries+1))
   echo "BOUNDED_WIRED_RECOVERY=$q38_recoveries;NO_PERSISTENT_SETTINGS"
   printf 'QWEN38-LEAVES: bounded wired recovery %s\n' "$q38_recoveries" > /dev/dprintf
   timeout -k 2 15 /bin/ifconfig /dev/net/rtl8125/1 down
   sleep 2
   timeout -k 2 15 /bin/ifconfig /dev/net/rtl8125/1 up
   sleep 5
   timeout -k 2 8 wget --inet4-only --spider --max-redirect=0 --timeout=5 --tries=1 --no-verbose "$q38_url"
  fi
 done
 [ "$q38_complete" -eq 1 ] || { echo STOP_FRESH_ATTEMPT_BOUND; exit 1; }
done
sync
echo "ALL3_FRESH_LEAVES_SHA_VERIFIED_RECOVERIES=$q38_recoveries;NO_PATCH_NO_MODEL_EXECUTION"
