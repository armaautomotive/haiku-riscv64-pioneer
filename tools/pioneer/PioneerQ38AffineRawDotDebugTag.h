/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_AFFINE_RAWDOT_DEBUG_TAG_H
#define PIONEER_Q38_AFFINE_RAWDOT_DEBUG_TAG_H
#ifndef PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT
#error Exact tag15 binds ONLY independent original-TC affine/raw-dot qualification
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
static inline bool PioneerQ38AffineRawDotDebugTag(const tt_pioneer_driver_reload& r) {
	return r.version==1 && r.driver==55 && r.guard_version==1
		&& r.build_tag==UINT64_C(0x2026102100010015) && r.generation
		&& !r.quiesced && !r.cold && !r.active_owner && r.open_count==1;
}
#endif
