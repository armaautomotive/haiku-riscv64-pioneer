/* SPDX-License-Identifier: MIT */
// Rechecked DATA selection only. No native or image ownership capability.
#ifdef PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION
#error Worker-sealed admission BUILD cannot select fixed QTP2 DATA
#endif
#if !defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || PIONEER_Q38_TOKEN_PRECISE_PROGRAMS != 1
#error Exact QTP2/QGW2 complete program DATA selection required
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE)
#error QTP2 DATA cannot alias a ROOT/worker image
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
// The real native parent checks its complete perimeter,not just a DATA flag.
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h"
#elif defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT)
#error QTP2 native context cannot be selected without its actual native parent
#endif
