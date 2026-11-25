/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRIVER_RELOAD_STATE_H
#define PIONEER_DRIVER_RELOAD_STATE_H
#include <stdint.h>

enum : uint32_t {
	PIONEER_RECOVERY_PROOF_ASIC_RESET_COMPLETE = 1u << 0,
	PIONEER_RECOVERY_PROOF_POST_RESET_FW_READY = 1u << 1,
	PIONEER_RECOVERY_PROOF_PCI_STATE_RESTORED = 1u << 2,
	PIONEER_RECOVERY_PROOF_MAPPINGS_REBUILT = 1u << 3,
	PIONEER_RECOVERY_PROOF_ALL_CORES_HELD = 1u << 4,
	PIONEER_RECOVERY_PROOF_NOC_IDLE = 1u << 5,
	PIONEER_RECOVERY_PROOF_HOST_DMA_IDLE = 1u << 6,
	PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED = 1u << 7,
	PIONEER_RECOVERY_PROOF_REQUIRED = (1u << 8) - 1
};

// This proof is produced inside the kernel after a full device reset. It is
// never copied from userspace. Keep it fixed-width so the resident guard can
// validate the same contract independently of the candidate driver.
struct PioneerDriverRecoveryProof {
	uint32_t version=1;
	uint32_t flags=0;
	uint32_t core_count=0;
	uint32_t reserved=0;
	uint32_t noc_busy_mask=0;
	uint32_t noc_outstanding_mask=0;
	uint32_t host_dma_active=0;
	uint32_t retained_owners=0;
	uint64_t reset_generation=0;
	uint64_t firmware_generation=0;
	bool ReadyToRetireOneOwner() const {
		return version==1 && flags==(PIONEER_RECOVERY_PROOF_REQUIRED
			& ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED)
			&& core_count==120 && !reserved && !noc_busy_mask
			&& !noc_outstanding_mask && !host_dma_active && retained_owners==1
			&& reset_generation && firmware_generation;
	}
	bool Complete() const {
		return version==1 && flags==PIONEER_RECOVERY_PROOF_REQUIRED
			&& core_count==120 && !reserved && !noc_busy_mask
			&& !noc_outstanding_mask && !host_dma_active && !retained_owners
			&& reset_generation && firmware_generation;
	}
};

static_assert(sizeof(PioneerDriverRecoveryProof)==48,
	"fixed-width in-kernel recovery proof");

#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
// Deliberately distinct from the ASIC-reset proof: a Tensix fabric reset
// cannot assert that the PCIe interface, ARC, or the whole ASIC reset.
// This experimental contract is not evidence that any flag is true.
enum : uint32_t {
	PIONEER_FABRIC_PROOF_RESET_SEQUENCE = 1u << 0,
	PIONEER_FABRIC_PROOF_FIRMWARE_READY = 1u << 1,
	PIONEER_FABRIC_PROOF_PCIE_HEALTHY = 1u << 2,
	PIONEER_FABRIC_PROOF_OLD_MAPPINGS_DISPOSED = 1u << 3,
	PIONEER_FABRIC_PROOF_ALL_CORES_HELD = 1u << 4,
	PIONEER_FABRIC_PROOF_NOC_IDLE = 1u << 5,
	PIONEER_FABRIC_PROOF_HOST_DMA_IDLE = 1u << 6,
	PIONEER_FABRIC_PROOF_OWNERS_RELEASED = 1u << 7,
	PIONEER_FABRIC_PROOF_CLOCK_RESTORED = 1u << 8,
	PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED = 1u << 9,
	PIONEER_FABRIC_PROOF_REQUIRED = (1u << 10) - 1
};
struct PioneerDriverFabricRecoveryProof {
	static constexpr uint32_t Version=0x51463301U;
	uint32_t version=Version,flags=0,core_count=0,reserved=0;
	uint32_t noc_busy_mask=0,noc_outstanding_mask=0,host_dma_active=0,retained_owners=0;
	uint64_t fabric_generation=0,firmware_generation=0;
	bool ReadyToRetireOneOwner() const {
		return version==Version && flags==(PIONEER_FABRIC_PROOF_REQUIRED
			& ~(PIONEER_FABRIC_PROOF_OWNERS_RELEASED | PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED))
			&& core_count==120 && !reserved && !noc_busy_mask && !noc_outstanding_mask
			&& !host_dma_active && retained_owners==1 && fabric_generation && firmware_generation;
	}
	bool Complete() const {
		return version==Version && flags==PIONEER_FABRIC_PROOF_REQUIRED
			&& core_count==120 && !reserved && !noc_busy_mask && !noc_outstanding_mask
			&& !host_dma_active && !retained_owners && fabric_generation && firmware_generation;
	}
};
static_assert(sizeof(PioneerDriverFabricRecoveryProof)==48,
	"fixed-width experimental fabric recovery proof");
#endif

// CPU metadata only. The resident guard serializes recovery-ticket changes;
// clearing cold requires a fresh ticket, a complete device proof, and an
// unchanged fault epoch.
struct PioneerDriverReloadState {
	uint64_t generation=0;
	bool attached=false,cold=false;
	uint64_t nextRecoveryTicket=0,pendingRecoveryTicket=0;
	uint64_t pendingRecoveryGeneration=0;
	uint32_t pendingFaultEpoch=0;
	bool Attach(uint64_t& result) {
		result=0;
		if (cold || attached || pendingRecoveryTicket || generation==UINT64_MAX) return false;
		attached=true; result=++generation; return true;
	}
	bool Detach(uint64_t owner,bool clean) {
		if (!attached || !owner || owner!=generation) { cold=true; return false; }
		attached=false; cold |= !clean; return !cold;
	}
	void Fault(uint64_t) { cold=true; } // stale owner is NOT permission to clear safety state
	bool BeginRecovery(uint64_t expectedGeneration,uint32_t faultEpoch,
		uint64_t& ticket) {
		ticket=0;
		if (!attached || !cold || pendingRecoveryTicket
			|| !expectedGeneration || expectedGeneration!=generation
			|| nextRecoveryTicket==UINT64_MAX) return false;
		pendingRecoveryTicket=++nextRecoveryTicket;
		pendingRecoveryGeneration=generation;
		pendingFaultEpoch=faultEpoch;
		ticket=pendingRecoveryTicket;
		return true;
	}
	bool AbortRecovery(uint64_t ticket) {
		if (!ticket || ticket!=pendingRecoveryTicket) return false;
		pendingRecoveryTicket=0;
		pendingRecoveryGeneration=0;
		pendingFaultEpoch=0;
		return true; // cold deliberately remains set
	}
	bool CanResumeRecovery(uint64_t ticket,uint32_t faultEpoch) const {
		return ticket && ticket==pendingRecoveryTicket && attached && cold
			&& generation==pendingRecoveryGeneration && generation!=UINT64_MAX
			&& (faultEpoch&1U) && faultEpoch!=UINT32_MAX && faultEpoch==pendingFaultEpoch;
	}
	bool CompleteRecovery(uint64_t ticket,uint32_t faultEpoch,
		const PioneerDriverRecoveryProof& proof,uint32_t& begunFaultEpoch,
		uint64_t& newGeneration) {
		begunFaultEpoch=0;
		newGeneration=0;
		if (!ticket || ticket!=pendingRecoveryTicket || !attached || !cold
			|| generation!=pendingRecoveryGeneration || generation==UINT64_MAX
			|| faultEpoch!=pendingFaultEpoch || !proof.Complete()
			|| proof.reset_generation!=ticket || proof.firmware_generation!=ticket) return false;
		begunFaultEpoch=pendingFaultEpoch;
		newGeneration=++generation;
		cold=false;
		pendingRecoveryTicket=0;
		pendingRecoveryGeneration=0;
		pendingFaultEpoch=0;
		return true;
	}
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
	bool CompleteFabricRecovery(uint64_t ticket,uint32_t faultEpoch,
		const PioneerDriverFabricRecoveryProof& proof,uint32_t& begunFaultEpoch,
		uint64_t& newGeneration) {
		begunFaultEpoch=0;newGeneration=0;
		if (!ticket || ticket!=pendingRecoveryTicket || !attached || !cold
			|| generation!=pendingRecoveryGeneration || generation==UINT64_MAX
			|| faultEpoch!=pendingFaultEpoch || !proof.Complete()
			|| proof.fabric_generation!=ticket || proof.firmware_generation!=ticket) return false;
		begunFaultEpoch=pendingFaultEpoch;newGeneration=++generation;cold=false;
		pendingRecoveryTicket=0;pendingRecoveryGeneration=0;pendingFaultEpoch=0;
		return true;
	}
#endif
};
#endif
