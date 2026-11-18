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
enum : unsigned { WorkVersion=1, WorkDriver=60 };
enum WorkStatus : unsigned { WorkRunning=1, WorkComplete, WorkQuarantined };

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
	unsigned generatedIds[MaxPositions];
};
struct WorkStop {
	unsigned version, restored, reserved[2];
};

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
		|| !(status.decodeCyclesLow|status.decodeCyclesHigh))
		return false;
	for (unsigned word:status.reserved)
		if (word) return false;
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
