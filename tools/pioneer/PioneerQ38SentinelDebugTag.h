/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SENTINEL_DEBUG_TAG_H
#define PIONEER_Q38_SENTINEL_DEBUG_TAG_H
#ifndef PIONEER_Q38_SENTINEL_REFERENCE_CLIENT
#error Exact sentinel tag17 (or separately gated CPU-profile18) is independent qualification ONLY
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#if defined(PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT) && (defined(PIONEER_Q38_FRONTEND_REFERENCE_CLIENT) || defined(PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT) || defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT) || defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT))
#error Raw-Dst cache qualifier has its own exact driver tag22
#endif
#if defined(PIONEER_Q38_FRONTEND_REFERENCE_CLIENT) && (defined(PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT) || defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT) || defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT))
#error Frontend-barrier qualifier has its own exact driver tag21
#endif
#if defined(PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT) && (defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT) || defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT))
#error Direct batch input has its own exact driver tag20
#endif
#if defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT) && defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT)
#error Batch4 and CPU phase qualifiers have DISTINCT exact driver tags
#endif
static inline bool PioneerQ38SentinelDebugTag(const tt_pioneer_driver_reload& r) {
	return r.version==1 && r.driver==55 && r.guard_version==1
		&& r.build_tag==
#ifdef PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT
		UINT64_C(0x2026102100010022)
#elif defined(PIONEER_Q38_FRONTEND_REFERENCE_CLIENT)
		UINT64_C(0x2026102100010021)
#elif defined(PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT)
		UINT64_C(0x2026102100010020)
#elif defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT)
		UINT64_C(0x2026102100010019)
#elif defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT)
		UINT64_C(0x2026102100010018)
#else
		UINT64_C(0x2026102100010017)
#endif
		&& r.generation
		&& !r.quiesced && !r.cold && !r.active_owner && r.open_count==1;
}
#endif
