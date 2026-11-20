/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DEVICE_IO_H
#define PIONEER_QWEN35_DEVICE_IO_H

#include "PioneerQwen35TokenRoot.h"
#include "PioneerQwen35RootImageLayout.h"

namespace PioneerQwen35 {
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
	bool TransferActivationRows(unsigned slot,unsigned firstRow,unsigned rows,
		float* local,bool write);
	bool StageBand(const TensorSpace& tensor,float* output,
		float* best=nullptr,unsigned* chosen=nullptr);
	bool StageBands(const TensorSpace* const* tensors,unsigned tensorCount,
		float* const* outputs,float* best=nullptr,unsigned* chosen=nullptr);
public:
	DeviceIO(const DeviceLayout& layout,Range ropeTable,volatile RequestProgress& status,
		volatile uint32_t* generatedIds);
	void Trace(unsigned stage);
	bool Ready();
	bool Embed(unsigned token,float* hidden,unsigned values);
	bool Project(unsigned layer,unsigned role,const float* input,unsigned inputs,float* output,unsigned outputs);
	bool ProjectGroup(unsigned layer,const unsigned* roles,const float* input,unsigned inputs,
		float* const* outputs,const unsigned* outputSizes,unsigned roleCount);
	bool ProjectPair(unsigned layer,unsigned firstRole,unsigned secondRole,
		const float* input,unsigned inputs,float* firstOutput,float* secondOutput,unsigned outputs);
	bool StoreActivation(unsigned slot,const float* input,unsigned rows);
	bool ActivateFfn(unsigned gateSlot,unsigned upSlot,float* gate,float* up,unsigned rows);
	bool ProjectFromActivation(unsigned layer,unsigned role,unsigned slot,unsigned inputs,
		float* output,unsigned outputs,float* inputScratch);
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
