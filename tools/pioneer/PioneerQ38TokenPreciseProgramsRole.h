/* SPDX-License-Identifier: MIT */
// Rechecked DATA selection only. No native or image ownership capability.
#if !defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || PIONEER_Q38_TOKEN_PRECISE_PROGRAMS != 1
#error Exact QTP2/QGW2 complete program DATA selection required
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)
#error QTP2 DATA cannot alias an image or the old genuine native context
#endif
