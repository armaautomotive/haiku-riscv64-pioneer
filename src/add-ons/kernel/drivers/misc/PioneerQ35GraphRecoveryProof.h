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
inline bool PioneerQ35ScratchRecoveryProofMatches(const PioneerDriverRecoveryProof& proof,uint64_t ticket)
{
	return ticket && proof.ReadyToRetireOneOwner()
		&& proof.reset_generation==ticket && proof.firmware_generation==ticket;
}
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
// A fabric proof is a separate contract; it never asserts an ASIC reset.
inline bool PioneerQ35GraphRecoveryProofMatches(const PioneerDriverFabricRecoveryProof& proof,
	uint64_t ticket,bool mappingClosed,bool ownerMappingsDisposed)
{
	return ticket && mappingClosed && ownerMappingsDisposed && proof.ReadyToRetireOneOwner()
		&& proof.fabric_generation==ticket && proof.firmware_generation==ticket;
}
inline bool PioneerQ35ScratchRecoveryProofMatches(const PioneerDriverFabricRecoveryProof& proof,uint64_t ticket)
{
	return ticket && proof.ReadyForScratchRestore()
		&& proof.fabric_generation==ticket && proof.firmware_generation==ticket;
}
#endif
#endif
