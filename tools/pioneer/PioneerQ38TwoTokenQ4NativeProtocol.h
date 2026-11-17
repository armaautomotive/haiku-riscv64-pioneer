/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWO_TOKEN_Q4_NATIVE_PROTOCOL_H
#define PIONEER_Q38_TWO_TOKEN_Q4_NATIVE_PROTOCOL_H
#include <Drivers.h>
#include "PioneerQ38TwoTokenQ4Probe.h"

// Private diagnostic ABI, never an Assistant hardware/Work selector.
namespace PioneerQ38TwoTokenQ4Native {
namespace Q=PioneerQ38TwoTokenQ4Probe;
constexpr uint64_t Tag=UINT64_C(0x2026111700010215);
enum : unsigned { Version=1, CompareOperation=B_DEVICE_OP_CODES_END+0x8280,
	Separate0=0, Separate1=1, Paired=2, Arms=3 };
struct Request {
	uint64_t tag,generation;
	uint32_t version,reserved;
	Q::Job job;
	uint64_t micros[Arms];
	tt_q38_panel_result arms[Arms];
	tt_resident_session_result cleanup;
	uint32_t compared,matched,cold,padding;
};
static_assert(sizeof(Request)<72*1024,"request is heap-only; no pointers or external address fields");
static inline bool Closed(const tt_resident_session_result& r)
{
	return r.version==1 && r.state==TT_RESIDENT_SESSION_CLOSED && !r.active && !r.cold
		&& r.pci_restored && r.host_released && !r.capabilities;
}
}
#endif
