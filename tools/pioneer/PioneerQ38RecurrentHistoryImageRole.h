/* SPDX-License-Identifier: MIT */
// No guard: experimental HISTORY-STAGING image, never a native/Work role.
#if !defined(PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE) || PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE != 1
#error Exact separate recurrent history-staging image required
#endif
#include "PioneerQ38MixedRecurrentBlockRootImageRole.h"
#if defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK) || defined(PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST) || defined(PIONEER_Q38_GDN_MUTABLE_STATE)
#error History-staging instructions confer no native,state,validation or Work authority,including defined0
#endif
