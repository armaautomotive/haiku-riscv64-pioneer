/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_POST_RESET_AUDIT_H
#define PIONEER_POST_RESET_AUDIT_H

#include <stdint.h>

enum : unsigned {
	PIONEER_POST_RESET_CORES=120,
	PIONEER_POST_RESET_NOCS=2,
	PIONEER_POST_RESET_INITIATORS=4,
	PIONEER_POST_RESET_IDS=32,
	PIONEER_POST_RESET_HELD=0x47800
};

enum PioneerPostResetAuditFailure {
	PioneerPostResetAuditNone=0,
	PioneerPostResetAuditSelect,
	PioneerPostResetAuditResetRead,
	PioneerPostResetAuditCoreNotHeld,
	PioneerPostResetAuditNoCRead,
	PioneerPostResetAuditNoCBusy,
	PioneerPostResetAuditOutstanding
};

struct PioneerPostResetAuditResult {
	unsigned failure=PioneerPostResetAuditNone;
	unsigned pass=0,core=PIONEER_POST_RESET_CORES;
	unsigned noc=PIONEER_POST_RESET_NOCS,index=0;
	uint32_t value=0;
	unsigned audited=0;
};

// IO must use fresh post-reset mappings, not pre-reset saved TLB leases.
// Read failures are distinct from register values; no all-ones sentinel can
// accidentally count as idle. Both complete passes must succeed, while the
// caller holds the exclusive device lock and prohibits any new work.
template<class IO>
bool PioneerPostResetAudit(IO& io,PioneerPostResetAuditResult& result)
{
	result={};
	for (unsigned pass=0;pass<2;pass++) {
		for (unsigned core=0;core<PIONEER_POST_RESET_CORES;core++) {
			result.pass=pass; result.core=core;
			result.noc=PIONEER_POST_RESET_NOCS; result.index=0;
			result.value=0;
			if (!io.SelectPostResetCore(core)) {
				result.failure=PioneerPostResetAuditSelect; return false;
			}
			uint32_t reset=0;
			if (!io.ReadPostResetState(reset)) {
				result.failure=PioneerPostResetAuditResetRead; return false;
			}
			if (reset!=PIONEER_POST_RESET_HELD) {
				result.failure=PioneerPostResetAuditCoreNotHeld;
				result.value=reset; return false;
			}
			for (unsigned noc=0;noc<PIONEER_POST_RESET_NOCS;noc++) {
				result.noc=noc;
				for (unsigned index=0;index<PIONEER_POST_RESET_INITIATORS
					+PIONEER_POST_RESET_IDS;index++) {
					result.index=index;
					const unsigned offset=index<PIONEER_POST_RESET_INITIATORS
						?index*0x800+0x40
						:0x240+(index-PIONEER_POST_RESET_INITIATORS)*4;
					uint32_t value=0;
					if (!io.ReadPostResetNoC(noc,offset,value)) {
						result.failure=PioneerPostResetAuditNoCRead; return false;
					}
					if (index>=PIONEER_POST_RESET_INITIATORS) value&=0xff;
					if (value) {
						result.failure=index<PIONEER_POST_RESET_INITIATORS
							?PioneerPostResetAuditNoCBusy:PioneerPostResetAuditOutstanding;
						result.value=value; return false;
					}
				}
			}
			result.audited++;
		}
		if (!pass) io.PostResetAuditPause();
	}
	result.pass=2; result.core=PIONEER_POST_RESET_CORES;
	result.noc=PIONEER_POST_RESET_NOCS; result.index=0;
	return true;
}

#endif
