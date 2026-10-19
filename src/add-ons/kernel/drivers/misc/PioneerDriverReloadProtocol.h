/* SPDX-License-Identifier: MIT */
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
