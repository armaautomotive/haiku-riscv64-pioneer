/* SPDX-License-Identifier: MIT */
// No guard:the new consumer can never be selected by an old scope alias.
#include "PioneerQ38ResidentLayerNativeRole.h"
#if !defined(PIONEER_Q38_RESIDENT_LAYER_OWNER) || PIONEER_Q38_RESIDENT_LAYER_OWNER != 1 \
 || !defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) || PIONEER_Q38_RESIDENT_OUTPUT_OWNER != 1
#error Exact NEW full-layer owner and genuine typed QPS1 producer export are required
#endif
#if defined(PIONEER_Q38_RGDN_ROOT_QUALIFICATION) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_GDN_GATED_NORM_LOCAL)
#error Full-layer owner does not alias experimental GDN/layout contracts
#endif
