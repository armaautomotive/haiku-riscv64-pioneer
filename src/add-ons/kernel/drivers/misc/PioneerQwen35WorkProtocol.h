/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_WORK_PROTOCOL_H
#define PIONEER_QWEN35_WORK_PROTOCOL_H

#include "../../../../../tools/pioneer/PioneerQwen35RootImageLayout.h"

// The private Work driver admits token IDs and one checked data manifest.
// Executable words, core selections, resets and NoC routes are driver-owned.
#define TT_PCI_PROBE_Q35_WORK_SUPPORT (B_DEVICE_OP_CODES_END + 0x64d0)
#define TT_PCI_PROBE_Q35_WORK_PREPARE (B_DEVICE_OP_CODES_END + 0x64d1)
#define TT_PCI_PROBE_Q35_WORK_START (B_DEVICE_OP_CODES_END + 0x64d2)
#define TT_PCI_PROBE_Q35_WORK_POLL (B_DEVICE_OP_CODES_END + 0x64d3)
#define TT_PCI_PROBE_Q35_WORK_STOP (B_DEVICE_OP_CODES_END + 0x64d4)

namespace PioneerQwen35 {
enum : unsigned { WorkVersion=3, WorkDriver=60 };
enum WorkStatus : unsigned {
	WorkRunning=1, WorkComplete, WorkQuarantined, WorkRecoverableFault, WorkRecoveryPending
};

// The Work peer saves RA and fences before publishing Q8PF. Those two
// instructions move its terminal self-loop eight bytes past the generic
// persistent peer's fault park. The Work image places a second local jump at
// the following word so speculative PC snapshots stay inside the fault park.
enum : unsigned {
	WorkPeerFaultEntry=0x72080, WorkPeerFaultPark=0x720a4,
	WorkPeerFaultParkFetch=0x720a8
};

inline bool WorkPeerFaultHandlerPc(unsigned pc)
{
	return pc>=WorkPeerFaultEntry && pc<=WorkPeerFaultParkFetch && !(pc&3);
}

inline bool WorkPeerFaultParkPc(unsigned pc)
{
	return pc==WorkPeerFaultPark || pc==WorkPeerFaultParkFetch;
}

struct WorkSupport {
	unsigned version, driver, maxContext, vocabulary;
	unsigned tensixProjections, hostInference, explicitEnd, reserved;
};
struct WorkPrepare {
	unsigned version, reserved;
	Range rope;
	DeviceLayout manifest;
};
struct WorkStart {
	unsigned version, requestId, promptCount, maxOutput;
	unsigned prompt[MaxPositions];
};
struct WorkPoll {
	unsigned version, requestId;
	WorkStatus state;
	unsigned pc;
	RequestProgress progress;
	unsigned long long startMicros, prefillObservedMicros, completeObservedMicros;
	unsigned recoveryProofReason, recoveryProofDetail;
	unsigned faultNocValid, faultNocCommand[10], faultNocCounters[8], faultNocCounterDelta[8];
	unsigned faultPeer119Valid, faultPeer119Pc[2], faultPeer119Status[16];
	unsigned generatedIds[MaxPositions];
};
struct WorkStop {
	unsigned version, restored, recoveryProofReason, recoveryProofDetail, reserved[2];
};

inline bool WorkPeerStatusSafe(unsigned recorded,unsigned requested,unsigned finished,
	unsigned prior,unsigned root)
{
	if (recorded>=0x7ffffffe || requested>=0x7ffffffe || finished>=0x7ffffffe
		|| prior>=0x7ffffffe || root>=0x7ffffffe || recorded>root
		|| requested<prior || finished<prior || requested>root+1 || finished>root+1
		|| finished>requested || requested-finished>1) return false;
	const bool requestKnown=requested==recorded || requested==recorded+1;
	const bool finishKnown=finished==recorded || finished==recorded+1;
	return requestKnown && finishKnown;
}

inline unsigned WorkPeerRecoveryStatusFailure(const unsigned* status,unsigned recorded,
	unsigned prior,unsigned root,unsigned peerMarker,unsigned faultMarker,
	unsigned imageBegin,unsigned imageBytes,bool faultHandler,unsigned* detail=NULL)
{
	if (detail) *detail=0;
	if (!status) return 1;
	if (!WorkPeerStatusSafe(recorded,status[0],status[1],prior,root)) {
		if (detail) *detail=((status[0]&0xfff)<<12)|(status[1]&0xfff);
		return 2;
	}
	if (status[3]!=peerMarker) { if (detail) *detail=status[3]; return 3; }
	if (status[4]>1) { if (detail) *detail=status[4]; return 4; }
	if (status[2]==0) {
		if (!faultHandler) {
			for (unsigned i=5;i<16;i++) if (status[i]) {
				if (detail) *detail=((i&15)<<20)|(status[i]&0xfffff);
				return 5;
			}
			return 0;
		}
		if (status[5] && ((status[5]&3) || status[5]<imageBegin
			|| status[5]>=imageBegin+imageBytes)) {
			if (detail) *detail=status[5];
			return 6;
		}
		for (unsigned i=9;i<16;i++) if (status[i]) {
			if (detail) *detail=((i&15)<<20)|(status[i]&0xfffff);
			return 7;
		}
		if (status[6]==0x50 && status[7]==status[8]) {
			if (detail) *detail=((status[7]&0xfff)<<12)|(status[8]&0xfff);
			return 8;
		}
		return 0;
	}
	if (!faultHandler) { if (detail) *detail=status[5]; return 9; }
	if (status[2]!=faultMarker) { if (detail) *detail=status[2]; return 10; }
	if ((status[5]&3) || status[5]<imageBegin || status[5]>=imageBegin+imageBytes) {
		if (detail) *detail=status[5];
		return 11;
	}
	for (unsigned i=9;i<16;i++) if (status[i]) {
		if (detail) *detail=((i&15)<<20)|(status[i]&0xfffff);
		return 12;
	}
	if (status[6]==0x50 && status[7]==status[8]) {
		if (detail) *detail=((status[7]&0xfff)<<12)|(status[8]&0xfff);
		return 13;
	}
	return 0;
}

inline bool WorkPeerRecoveryStatusSafe(const unsigned* status,unsigned recorded,
	unsigned prior,unsigned root,unsigned peerMarker,unsigned faultMarker,
	unsigned imageBegin,unsigned imageBytes,bool faultHandler)
{
	return WorkPeerRecoveryStatusFailure(status,recorded,prior,root,peerMarker,
		faultMarker,imageBegin,imageBytes,faultHandler)==0;
}

inline bool WorkPeerFreshStatus(const unsigned* status,unsigned peerMarker)
{
	if (!status) return false;
	for (unsigned i=0;i<16;i++)
		if (status[i]!=(i==3?peerMarker:0)) return false;
	return true;
}

inline bool ValidWorkStart(const WorkStart& start, unsigned context, unsigned nextId)
{
	if (start.version!=WorkVersion || !nextId || start.requestId!=nextId
		|| !context || context>MaxPositions || !start.promptCount || start.promptCount>context
		|| !start.maxOutput || start.maxOutput-1>context-start.promptCount)
		return false;
	for (unsigned i=0; i<start.promptCount; i++)
		if (start.prompt[i]>=Vocabulary) return false;
	return true;
}

inline bool ValidWorkResult(const WorkPoll& result, unsigned promptCount, unsigned maxOutput,
	unsigned context, unsigned terminalCount)
{
	const auto& status=result.progress;
	if (result.version!=WorkVersion || !result.requestId || status.requestId!=result.requestId
		|| status.magic!=RootProgressMagic || status.terminal!=1 || status.prefilled!=promptCount
		|| !promptCount || promptCount>context || context>MaxPositions
		|| !status.generated || status.generated>maxOutput || status.generated!=terminalCount
		|| status.generated-1>context-promptCount
		|| status.committed!=promptCount+status.generated-1
		|| !(status.promptCyclesLow|status.promptCyclesHigh)
		|| !(status.decodeCyclesLow|status.decodeCyclesHigh)
		|| !status.reserved[ProgressFullCoreWavesSlot]
		|| status.reserved[ProgressProjectionWavesSlot]<status.reserved[ProgressFullCoreWavesSlot])
		return false;
	for (unsigned word=0;word<ProgressFullCoreWavesSlot;word++)
		if (status.reserved[word]) return false;
	for (unsigned i=0; i<status.generated; i++) {
		const unsigned token=result.generatedIds[i];
		const bool stop=token==EndOfTurn || token==EndOfText;
		if (token>=Vocabulary || (stop && i+1!=status.generated)
			|| (!stop && i+1==status.generated && status.generated!=maxOutput))
			return false;
	}
	return true;
}
}
#endif
