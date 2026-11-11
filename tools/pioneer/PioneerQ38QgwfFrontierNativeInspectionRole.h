/* SPDX-License-Identifier: MIT */
// Private fake-tag inspection only: no device, client, or Work authority.
#include "PioneerQ38QgwfNativeInspectionRole.h"
#if !defined(PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION) || PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWF_NATIVE_INSPECTION) || PIONEER_Q38_QGWF_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWF_FRONTIER_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWF_FRONTIER_PROGRAMS_INSPECTION != 1 \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_FULL_OFFLOAD_METRICS)
#error QGWF frontier reader requires its distinct non-loadable inspection profile
#endif
