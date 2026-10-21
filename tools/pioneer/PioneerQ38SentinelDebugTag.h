/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SENTINEL_DEBUG_TAG_H
#define PIONEER_Q38_SENTINEL_DEBUG_TAG_H
#ifndef PIONEER_Q38_SENTINEL_REFERENCE_CLIENT
#error Exact sentinel tag17 (or separately gated CPU-profile18) is independent qualification ONLY
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
static inline bool PioneerQ38SentinelDebugTag(const tt_pioneer_driver_reload& r) {
	return r.version==1 && r.driver==55 && r.guard_version==1
		&& r.build_tag==
#ifdef PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT
		UINT64_C(0x2026102100010018)
#else
		UINT64_C(0x2026102100010017)
#endif
		&& r.generation
		&& !r.quiesced && !r.cold && !r.active_owner && r.open_count==1;
}
#endif
