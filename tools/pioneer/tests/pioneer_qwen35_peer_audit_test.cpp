/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35PeerAudit.h"
#include <cassert>
#include <cstdio>

int main()
{
	assert(PioneerQ35PeerPhysicalAuditRequired(false,false));
	assert(PioneerQ35PeerPhysicalAuditRequired(false,true));
	assert(!PioneerQ35PeerPhysicalAuditRequired(true,false));
	assert(PioneerQ35PeerPhysicalAuditRequired(true,true));
	std::puts("PASS Qwen35 Work physical PC/reset audit policy: every close and all checked dispatches are audited");
	return 0;
}
