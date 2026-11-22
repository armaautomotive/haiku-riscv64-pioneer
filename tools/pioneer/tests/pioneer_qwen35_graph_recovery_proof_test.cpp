/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ35GraphRecoveryProof.h"
#include <cassert>
#include <cstdio>

static PioneerDriverRecoveryProof Proof() {
	PioneerDriverRecoveryProof p{};
	p.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
	p.core_count=120;p.retained_owners=1;p.reset_generation=9;p.firmware_generation=9;
	return p;
}
int main() {
	const auto proof=Proof();
	assert(PioneerQ35GraphRecoveryProofMatches(proof,9,true,true));
	assert(!PioneerQ35GraphRecoveryProofMatches(proof,0,true,true));
	assert(!PioneerQ35GraphRecoveryProofMatches(proof,8,true,true));
	assert(!PioneerQ35GraphRecoveryProofMatches(proof,9,false,true));
	assert(!PioneerQ35GraphRecoveryProofMatches(proof,9,true,false));
	auto bad=proof;bad.firmware_generation=10;
	assert(!PioneerQ35GraphRecoveryProofMatches(bad,9,true,true));
	bad=proof;bad.noc_outstanding_mask=1;
	assert(!PioneerQ35GraphRecoveryProofMatches(bad,9,true,true));
	bad=proof;bad.host_dma_active=1;
	assert(!PioneerQ35GraphRecoveryProofMatches(bad,9,true,true));
	bad=proof;bad.core_count=119;
	assert(!PioneerQ35GraphRecoveryProofMatches(bad,9,true,true));
	bad=proof;bad.retained_owners=0;
	assert(!PioneerQ35GraphRecoveryProofMatches(bad,9,true,true));
	puts("PASS graph recovery proof: ticket, full fleet, idle DMA/NoC and disposed mappings; SIMULATED ONLY");
	return 0;
}
