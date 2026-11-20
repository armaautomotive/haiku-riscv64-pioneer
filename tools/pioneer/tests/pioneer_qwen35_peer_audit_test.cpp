/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35PeerAudit.h"
#include <cassert>
#include <cstdio>

int main()
{
	static_assert(PioneerQ35WorkJobAddress+PioneerQ35WorkJobBytes
		==PioneerQ35WorkPeerStatusAddress,"combined job must end before peer status");
	static_assert(PioneerQ35WorkJobSequenceAddress==PioneerQ35WorkJobAddress+15*4,
		"commit sequence must occupy the descriptor's trailing word15");
	static_assert((PioneerQ35WorkJobAddress&63)==0 && (PioneerQ35WorkJobBytes&63)==0,
		"combined command must be one aligned 64-byte transfer");
	assert(PioneerQ35PeerPhysicalAuditRequired(false,false));
	assert(PioneerQ35PeerPhysicalAuditRequired(false,true));
	assert(!PioneerQ35PeerPhysicalAuditRequired(true,false));
	assert(PioneerQ35PeerPhysicalAuditRequired(true,true));
	assert(PioneerQ35WorkSequenceValid(0,1));
	assert(PioneerQ35WorkSequenceValid(9,10));
	assert(!PioneerQ35WorkSequenceValid(9,9));
	assert(!PioneerQ35WorkSequenceValid(9,11));
	assert(!PioneerQ35WorkSequenceValid(0x7ffffffeU,0x7fffffffU));
	unsigned descriptor[16]={}; descriptor[12]=descriptor[15]=10;
	assert(PioneerQ35WorkDescriptorSequenceValid(9,10,descriptor));
	descriptor[12]=9;
	assert(!PioneerQ35WorkDescriptorSequenceValid(9,10,descriptor));
	descriptor[12]=10; descriptor[15]=9;
	assert(!PioneerQ35WorkDescriptorSequenceValid(9,10,descriptor));
	std::puts("PASS Qwen35 Work combined descriptor/sequence layout and peer audit policy");
	return 0;
}
