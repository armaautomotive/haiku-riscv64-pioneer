/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GROUP32_DEBUG_TAG_H
#define PIONEER_Q38_GROUP32_DEBUG_TAG_H
#ifndef PIONEER_Q38_GROUP32_REFERENCE_CLIENT
#error Tag binds ONLY the independent group32 qualification client
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// CPU metadata ONLY. Refuse the wrong arithmetic image BEFORE BANK_BEGIN.
static inline bool PioneerQ38Group32DebugTag(const tt_pioneer_driver_reload& r) {
	return r.version==1 && r.driver==55 && r.guard_version==1
		&& r.build_tag==UINT64_C(0x2026102100010013) && r.generation
		&& !r.quiesced && !r.cold && !r.active_owner && r.open_count==1;
}
#endif
