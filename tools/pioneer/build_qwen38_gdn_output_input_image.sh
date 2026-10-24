#!/bin/sh
# OFFLINE full distinct peer image;no native/device/install selector.
set -eu
python3 -c 'import os,sys;sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
env PIONEER_Q38_FFN_PERSISTENT=1 sh "$directory/build_qwen38_ffn_persistent_image.sh" "$1" "$2" --gdn-output-input
awk -F '\t' 'NF==3 && $2>2048 {print "STOP oversized new input frame: " $0; bad=1} END {exit bad}' "$2/activation.su"
