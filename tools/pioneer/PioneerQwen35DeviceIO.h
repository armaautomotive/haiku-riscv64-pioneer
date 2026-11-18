/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DEVICE_IO_H
#define PIONEER_QWEN35_DEVICE_IO_H

#include "PioneerQwen35TokenRoot.h"
#include "PioneerQwen35DeviceLayout.h"

namespace PioneerQwen35 {
struct RequestProgress {
	uint32_t magic, committed, generated, prefilled;
	uint32_t promptCyclesLow, promptCyclesHigh, decodeCyclesLow, decodeCyclesHigh;
	uint32_t terminal, reserved[7];
};
static_assert(sizeof(RequestProgress)==64, "card status cache line");

// Only the resident owner may initialize the manifest, RoPE table and
// arena. This transport performs all reads and projections on the ROOT.
class DeviceIO {
	const DeviceLayout& model;
	const Range rope;
	volatile RequestProgress& progress;
	volatile uint32_t* tokens;
	unsigned position=0;
	unsigned generated=0;
	__UINT64_TYPE__ started, decodeStarted=0;

	bool Transfer(Range range,void* local);
	bool Write(Range range,const void* local);
	bool TransferWords(Range range,void* local,bool write);
	bool StageInput(unsigned type,const float* input,unsigned inputs);
	bool StageBand(const TensorSpace& tensor,unsigned first,unsigned groups,float* output);
public:
	DeviceIO(const DeviceLayout& layout,Range ropeTable,volatile RequestProgress& status,
		volatile uint32_t* generatedIds);
	bool Ready() const;
	bool Embed(unsigned token,float* hidden,unsigned values);
	bool Project(unsigned layer,unsigned role,const float* input,unsigned inputs,float* output,unsigned outputs);
	bool TiedArgmax(const float* input,unsigned inputs,unsigned& token);
	bool ReadWeight(unsigned layer,unsigned role,unsigned offset,float* output,unsigned values);
	bool ReadRope(unsigned position,float* output,unsigned values);
	bool ReadConv(unsigned layer,unsigned slot,unsigned channel,float* output,unsigned values);
	bool WriteConv(unsigned layer,unsigned slot,unsigned channel,const float* input,unsigned values);
	bool ReadDelta(unsigned layer,unsigned slot,unsigned head,float* output,unsigned values);
	bool WriteDelta(unsigned layer,unsigned slot,unsigned head,const float* input,unsigned values);
	bool ReadKv(unsigned layer,unsigned head,unsigned position,uint32_t* key,uint32_t* value,unsigned words);
	bool WriteKv(unsigned layer,unsigned head,unsigned position,const uint32_t* key,const uint32_t* value,unsigned words);
	bool PrefillDone(unsigned positions);
	bool Commit(unsigned position,unsigned token,bool sampled,unsigned next);
	bool FinishRequest(unsigned positions,unsigned generated);
};
}
#endif
