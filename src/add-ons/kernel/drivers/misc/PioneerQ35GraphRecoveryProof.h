/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q35_GRAPH_RECOVERY_PROOF_H
#define PIONEER_Q35_GRAPH_RECOVERY_PROOF_H

#include "PioneerDriverReloadState.h"

// CPU-only release gate. The reset ticket and proof are produced inside the
// locked recovery operation; neither is accepted from an inference request.
inline bool PioneerQ35GraphRecoveryProofMatches(const PioneerDriverRecoveryProof& proof,
	uint64_t ticket,bool mappingClosed,bool ownerMappingsDisposed)
{
	return ticket && mappingClosed && ownerMappingsDisposed && proof.ReadyToRetireOneOwner()
		&& proof.reset_generation==ticket && proof.firmware_generation==ticket;
}
#endif
