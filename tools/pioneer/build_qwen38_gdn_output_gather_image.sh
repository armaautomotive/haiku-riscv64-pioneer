#!/bin/sh
# OFFLINE QOG1 ROOT image;never a native registry or install selector.
set -eu
python3 -c 'import os,sys;sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
sh "$here/build_qwen38_local_image.sh" gdn_output_gather "$1" "$2"
awk -F '\t' 'NF==3 && $2>2048 {print "STOP oversized gather frame: " $0; bad=1} END {exit bad}' "$2/program.su"
