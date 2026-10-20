/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_STREAM_PROTOCOL_H
#define PIONEER_Q38_STREAM_PROTOCOL_H
#include "PioneerQ38PanelProbe.h"
// Separate future fullK probe protocol. No installed48 handler accepts it.
// Logical bank+page offset ONLY;not a physical pointer/coordinate/NoC table.
#define TT_PCI_PROBE_Q38_STREAM_SUPPORT (B_DEVICE_OP_CODES_END + 0x54ee)
#define TT_PCI_PROBE_Q38_STREAM_RUN (B_DEVICE_OP_CODES_END + 0x54ef)
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
enum { TT_Q38_STREAM_DRIVER=50,TT_Q38_STREAM_ABI=2,TT_Q38_STREAM_OUTPUT_ROWS=256 };
#define PioneerQ38StreamRows(r) ((r).groups*16U)
#define PioneerQ38StreamGroups(r) ((r).groups)
#else
enum { TT_Q38_STREAM_DRIVER=49,TT_Q38_STREAM_ABI=1,TT_Q38_STREAM_OUTPUT_ROWS=16 };
#define PioneerQ38StreamRows(r) 16U
#define PioneerQ38StreamGroups(r) 1U
#endif
enum { TT_Q38_STREAM_INPUT_WORDS=8704,
	TT_Q38_STREAM_SCALE_WORDS=544 };
struct tt_q38_stream_support {
	uint32_t driver,version,local_rows,max_inputs,banks,model_inference;
};
struct tt_q38_stream_probe {
	uint32_t version,type,inputs,bank,reserved[2];
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
	uint32_t groups,padding; // bounded1..16,not an address/worker selector
#endif
	uint64_t offset;
	uint32_t activation[TT_Q38_STREAM_INPUT_WORDS],scales[TT_Q38_STREAM_SCALE_WORDS];
	tt_q38_panel_result result;
	uint32_t output[TT_Q38_STREAM_OUTPUT_ROWS];
};
static inline unsigned PioneerQ38StreamBlockInputs(unsigned type) { return type==8?32:256; }
static inline unsigned PioneerQ38StreamBytes(const tt_q38_stream_probe& r)
{
	return PioneerQ38StreamGroups(r)*(r.inputs/PioneerQ38StreamBlockInputs(r.type))*PioneerQ38PanelProbeBytes(r.type);
}
static inline bool PioneerQ38StreamValid(const tt_q38_stream_probe& r)
{
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
	if (!r.groups || r.groups>16 || r.padding) return false;
#endif
	if (r.version!=TT_Q38_STREAM_ABI || r.reserved[0] || r.reserved[1] || !PioneerQ38PanelProbeBytes(r.type)
		|| (r.inputs!=5120 && r.inputs!=6144 && r.inputs!=17408) || r.bank>=7 || (r.offset&4095)
		|| r.offset>=PioneerResidentBanks::Capacity || PioneerQ38StreamBytes(r)>PioneerResidentBanks::Capacity-r.offset) return false;
	unsigned words=r.inputs/2,blocks=r.inputs/PioneerQ38StreamBlockInputs(r.type);
	for (unsigned w=0;w<words;w++) if (!PioneerInt8OperandValid(uint16_t(r.activation[w]))
		|| !PioneerInt8OperandValid(uint16_t(r.activation[w]>>16))) return false;
	for (unsigned w=words;w<TT_Q38_STREAM_INPUT_WORDS;w++) if (r.activation[w]) return false;
	for (unsigned b=0;b<blocks;b++) {
		uint32_t magnitude=r.scales[b]&0x7fffffff;
		if (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return false;
	}
	for (unsigned b=blocks;b<TT_Q38_STREAM_SCALE_WORDS;b++) if (r.scales[b]) return false;
	return true;
}
// KERNEL-PRIVATE derived authority. Constructed only after complete validated
// input AND private same-owner page bitmap AND immutable seven-route proof.
// A zero/nonzero user field can NEVER stand in for initialized pages.
struct PioneerQ38StreamExtent {
	uint32_t address=0,route=0,index=0,pages=0,bytes=0,blocks=0,flits=0;
};
static inline bool PioneerQ38StreamAuthorize(const PioneerResidentBanks& banks,const tt_dram_samples_result& dram,
	const uint8_t* initialized,uint32_t initializedPages,const tt_q38_stream_probe& request,PioneerQ38StreamExtent& result)
{
	if (!initialized || initializedPages!=PioneerResidentBanks::Pages || !PioneerQ38StreamValid(request)) return false;
	PioneerQ38StreamExtent stage;
	stage.bytes=PioneerQ38StreamBytes(request); stage.pages=(stage.bytes+4095)/4096;
	stage.blocks=PioneerQ38StreamGroups(request)*(request.inputs/PioneerQ38StreamBlockInputs(request.type));
	stage.flits=stage.bytes/64;
	if (!stage.bytes || (stage.bytes&63) || !banks.Page(dram,request.bank,request.offset,stage.address,stage.route,stage.index)
		|| stage.index>=initializedPages || stage.pages>initializedPages-stage.index) return false;
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	if (!banks.ImmutableRange(dram,request.bank,request.offset,stage.bytes)) return false;
#endif
	for (unsigned page=0;page<stage.pages;page++) if (initialized[stage.index+page]!=1) return false;
	result=stage; return true;
}
#endif
