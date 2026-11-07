/* SPDX-License-Identifier: MIT */
// The fixed hybrid candidate must not share its identity with another driver.
// Recheck outside the include guard so a later role/tag change is rejected.
#if defined(TT_PIONEER_DRIVER_RELOAD) && defined(PIONEER_DRIVER_BUILD_TAG) \
 && PIONEER_DRIVER_BUILD_TAG == 0x2026110700010126ULL \
 && (!defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 || !defined(PIONEER_Q38_HYBRID_FIXED_126) \
  || PIONEER_Q38_HYBRID_FIXED_126 != 1)
#error Tag126 is reserved for the corrected tracked hybrid full-card candidate
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && defined(PIONEER_DRIVER_BUILD_TAG) \
 && PIONEER_DRIVER_BUILD_TAG == 0x2026110700010127ULL \
 && (!defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 \
  || !defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) || PIONEER_Q38_HYBRID_MATRIX_NATIVE_127 != 1)
#error Tag127 is reserved for the isolated QGW7 matrix-attention candidate
#endif
#ifndef PIONEER_DRIVER_RELOAD_PROTOCOL_H
#define PIONEER_DRIVER_RELOAD_PROTOCOL_H
#include <stdint.h>
#define TT_PCI_PROBE_RELOAD_STATUS (B_DEVICE_OP_CODES_END+0x61e0)
#define TT_PCI_PROBE_RELOAD_QUIESCE (B_DEVICE_OP_CODES_END+0x61e1)
// No pointers,physical addresses,arbitrary PCI commands or cold-latch reset.
struct tt_pioneer_driver_reload {
	uint32_t version,reserved[3];
	uint64_t expected_tag,expected_generation;
	uint32_t driver,quiesced,cold,active_owner,open_count,guard_version;
	uint64_t build_tag,generation;
};
static_assert(sizeof(tt_pioneer_driver_reload)==72,"fixed pointer-free reload protocol");
static inline bool PioneerDriverReloadRequest(const tt_pioneer_driver_reload& r)
{
	return r.version==1 && !r.reserved[0] && !r.reserved[1] && !r.reserved[2];
}
#endif
