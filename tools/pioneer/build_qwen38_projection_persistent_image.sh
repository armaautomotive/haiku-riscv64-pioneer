#!/bin/sh
# OFFLINE complete new peer image;no native registry/coordinator access.
set -eu
[ "${PIONEER_Q38_CONV_NATIVE-0}:${PIONEER_Q38_CONV_CLIENT-0}:${PIONEER_Q38_CONV_IMAGE-0}" = 0:0:0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_VALIDATION_NATIVE-0}:${PIONEER_Q38_PROJECTION_VALIDATION_CLIENT-0}" = 0:0 ] && [ "${PIONEER_DRIVER_BUILD_TAG-}" != 2026102200010042 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_RESULT_STASH-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE-0}:${PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT-0}" = 0:0 ] && [ "${PIONEER_DRIVER_BUILD_TAG-}" != 2026102200010041 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_QUALIFICATION-0}:${PIONEER_Q38_EMBEDDING_CLIENT-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_LEARNED_RMS-0}:${PIONEER_Q38_LEARNED_RMS_NATIVE-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_EXECUTION_OWNER-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE-0}:${PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_NATIVE-0}" = 0 ] || exit 2
[ "$#" = 2 ] || exit 2
for key in $(env | sed -n 's/^\(PIONEER_[A-Z0-9_]*\)=.*/\1/p');do
 [ "$(printenv "$key" || :)" = 0 ] || exit 2
done
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec env PIONEER_Q38_FFN_PERSISTENT=1 sh "$directory/build_qwen38_ffn_persistent_image.sh" "$1" "$2" --projection-input
