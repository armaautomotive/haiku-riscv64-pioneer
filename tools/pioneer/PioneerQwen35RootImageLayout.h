/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ROOT_IMAGE_LAYOUT_H
#define PIONEER_QWEN35_ROOT_IMAGE_LAYOUT_H

#include "PioneerQwen35DeviceLayout.h"

namespace PioneerQwen35 {
enum : unsigned {
	RootBootMagic=0x31573551, RootProgressMagic=0x31503542,
	RootDone=0x31443551, RootFault=0x31463551,
	RootDispatchRejectTag=0x54f00000, RootDispatchRejectMask=0xffff0000,
	RootBootAddress=0x6000, RootTextAddress=0x40000, RootTextLimit=0x60000,
	RootMatrixJobAddress=0x74f60, RootDispatchStateAddress=0x7fc00,
	RootCheckpointAddress=0x17ffb0,
	RootProjectionResultAddress=0x9e000, RootControlAddress=0xe7000, RootProgressAddress=0xe7040,
	RootScaleEvidenceAddress=0xe7080,
	RootDispatchStateMagic=0x31533551,
	RootScratchAddress=0xb0000, RootScratchEnd=0xe2000,
	RootBounceAddress=RootScratchEnd, RootBounceBytes=16384,
	RootLayoutAddress=0xea000, RootPromptAddress=0x100000,
	RootOutputAddress=RootPromptAddress+MaxPositions*4, RootEnd=RootOutputAddress+MaxPositions*4,
	RootCompletionAddress=0x17ffc0, RootCompletionWords=2,
	EndOfText=248044, EndOfTurn=248046
};
struct DispatchState {
	unsigned magic, rootNonce, peerSequence[119];
};
struct RootControl {
	unsigned magic, promptCount, outputLimit, context, eos, endTurn;
	Range rope;
	unsigned requestId, reserved[6];
};
struct RequestProgress {
	unsigned magic, committed, generated, prefilled;
	unsigned promptCyclesLow, promptCyclesHigh, decodeCyclesLow, decodeCyclesHigh;
	unsigned terminal, requestId, reserved[6];
};
struct RootScaleEvidence {
	unsigned beforeClear, afterClear, afterStageInput, stageBandEntry;
	unsigned afterInitialValidation, afterFragments, afterStateValidation;
	unsigned beforeBatchObject, afterBatchObject, afterBatchBuild, afterBatchFields;
	unsigned afterRootJob, afterWorkerTable, beforeDispatch, beforeValidation;
};
enum : unsigned {
	ProgressReadyFailureSlot=0, ProgressFullCoreWavesSlot=4, ProgressProjectionWavesSlot=5,
	ProgressActivationPackTicksSlot=1, ProgressProjectionSetupTicksSlot=2,
	ProgressMatrixDispatchTicksSlot=3,
	RootScaleEvidenceWords=15, WorkTensixCoreCount=120, WorkTensixPeerCount=WorkTensixCoreCount-1,
	RootWorkerResultAddress=0x80000,
	RootWorkerResultBytes=WorkTensixPeerCount*256*sizeof(unsigned),
	RootWorkerResultEnd=RootWorkerResultAddress+RootWorkerResultBytes,
	RootScaleAddress=RootWorkerResultEnd, RootScaleMaxWords=80
};
static_assert(sizeof(DispatchState)==484 && RootDispatchStateAddress+sizeof(DispatchState)<=0x80000,
	"projection fleet state has an isolated ROOT L1 owner");
static_assert(sizeof(RootControl)==64 && RootControlAddress+sizeof(RootControl)<=RootProgressAddress
	&& RootProgressAddress+sizeof(RequestProgress)<=RootLayoutAddress,
	"control and progress have dedicated cache lines");
static_assert(sizeof(RequestProgress)==64, "card status cache line");
static_assert(sizeof(RootScaleEvidence)==60
	&& RootProgressAddress+sizeof(RequestProgress)==RootScaleEvidenceAddress
	&& RootScaleEvidenceAddress+sizeof(RootScaleEvidence)<=RootLayoutAddress,
	"failure-only scale evidence occupies its own reserved ROOT L1 range");
static_assert(RootWorkerResultEnd==0x9dc00 && RootScaleAddress==0x9dc00
	&& RootScaleAddress+RootScaleMaxWords*sizeof(unsigned)<=RootProjectionResultAddress,
	"Qwen3.5 scales must be disjoint from all 119 peer output buffers and ROOT results");
static_assert(RootBounceAddress+RootBounceBytes<=RootLayoutAddress
	&& RootLayoutAddress+sizeof(DeviceLayout)<=RootPromptAddress && RootEnd<RootCompletionAddress
	&& RootEnd<RootCheckpointAddress && RootCheckpointAddress+16==RootCompletionAddress
	&& RootProjectionResultAddress+256*4<=0xa0000,
	"dedicated local image regions are disjoint");
}
#endif
