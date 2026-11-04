#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-native-dram-model-band-20261104-a
test ! -e "$stage" && test ! -L "$stage"
umask 077
mkdir "$stage"
[ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
echo PRIVATE_NEW_STAGE_CREATED_NO_DRIVER_CHANGE_OR_GPU
