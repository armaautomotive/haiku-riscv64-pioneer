/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_COHORT_PROTOCOL_H
#define PIONEER_Q38_COHORT_PROTOCOL_H
#include "PioneerQ38PanelProbe.h"
// Separate51 debug/52 code-sealed/53 data-payload qualification,NOT inference.
// Native49/50 MUST NOT accept these ABIs;51/52/53 also reject each other.
// DATA only:logical bank+64B offset,never code,coordinates or NoC addresses.
#define TT_PCI_PROBE_Q38_COHORT_SUPPORT (B_DEVICE_OP_CODES_END + 0x54f0)
#define TT_PCI_PROBE_Q38_COHORT_RUN (B_DEVICE_OP_CODES_END + 0x54f1)
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD)
enum { TT_Q38_COHORT_DRIVER=53,TT_Q38_COHORT_ABI=3,TT_Q38_COHORT_PER_CALL_CODE=0 };
#elif defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED)
enum { TT_Q38_COHORT_DRIVER=52,TT_Q38_COHORT_ABI=2,TT_Q38_COHORT_PER_CALL_CODE=0 };
#else
enum { TT_Q38_COHORT_DRIVER=51,TT_Q38_COHORT_ABI=1,TT_Q38_COHORT_PER_CALL_CODE=1 };
#endif
enum { TT_Q38_COHORT_WORKERS=120,
	TT_Q38_COHORT_INPUT_WORDS=8704,TT_Q38_COHORT_SCALE_WORDS=544,TT_Q38_COHORT_ROWS=256 };
struct tt_q38_cohort_support { uint32_t driver,version,workers,rows,max_inputs,banks,offset_alignment,model_inference; };
struct tt_q38_cohort_part { uint32_t bank,groups; uint64_t offset; };
struct tt_q38_cohort_result {
	uint32_t version,state,workers,started,completed,stopped,recovery_required;
	tt_q38_panel_result core[TT_Q38_COHORT_WORKERS];
};
struct tt_q38_cohort_probe {
	uint32_t version,type,inputs,workers,reserved[4];
	tt_q38_cohort_part part[TT_Q38_COHORT_WORKERS];
	uint32_t activation[TT_Q38_COHORT_INPUT_WORDS],scales[TT_Q38_COHORT_SCALE_WORDS];
	tt_q38_cohort_result result;
	uint32_t output[TT_Q38_COHORT_WORKERS][TT_Q38_COHORT_ROWS];
};
static inline unsigned PioneerQ38CohortBlockInputs(unsigned type) { return type==8?32:256; }
static inline unsigned PioneerQ38CohortBytes(unsigned type,unsigned inputs,unsigned groups)
{ return groups*(inputs/PioneerQ38CohortBlockInputs(type))*PioneerQ38PanelProbeBytes(type); }
static inline bool PioneerQ38CohortPartValid(unsigned type,unsigned inputs,const tt_q38_cohort_part& p)
{
	return PioneerQ38PanelProbeBytes(type) && (inputs==5120 || inputs==6144 || inputs==17408)
		&& p.groups && p.groups<=16 && p.bank<7 && !(p.offset&63) && p.offset<PioneerResidentBanks::Capacity
		&& PioneerQ38CohortBytes(type,inputs,p.groups)<=PioneerResidentBanks::Capacity-p.offset;
}
static inline bool PioneerQ38CohortValid(const tt_q38_cohort_probe& r)
{
	if (r.version!=TT_Q38_COHORT_ABI || !r.workers || r.workers>TT_Q38_COHORT_WORKERS
		|| !PioneerQ38PanelProbeBytes(r.type) || (r.inputs!=5120 && r.inputs!=6144 && r.inputs!=17408)) return false;
	for (unsigned value:r.reserved) if (value) return false;
	for (unsigned w=0;w<TT_Q38_COHORT_WORKERS;w++) {
		const auto& p=r.part[w];
		if (w>=r.workers) { if (p.bank || p.groups || p.offset) return false; continue; }
		if (!PioneerQ38CohortPartValid(r.type,r.inputs,p)) return false;
		for (unsigned other=0;other<w;other++) {
			const auto& q=r.part[other];
			if (q.bank==p.bank && p.offset<q.offset+PioneerQ38CohortBytes(r.type,r.inputs,q.groups)
				&& q.offset<p.offset+PioneerQ38CohortBytes(r.type,r.inputs,p.groups)) return false;
		}
	}
	for (unsigned w=0;w<r.inputs/2;w++) if (!PioneerInt8OperandValid(uint16_t(r.activation[w]))
		|| !PioneerInt8OperandValid(uint16_t(r.activation[w]>>16))) return false;
	for (unsigned w=r.inputs/2;w<TT_Q38_COHORT_INPUT_WORDS;w++) if (r.activation[w]) return false;
	unsigned blocks=r.inputs/PioneerQ38CohortBlockInputs(r.type);
	for (unsigned b=0;b<blocks;b++) {
		uint32_t magnitude=r.scales[b]&0x7fffffff;
		if (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return false;
	}
	for (unsigned b=blocks;b<TT_Q38_COHORT_SCALE_WORDS;b++) if (r.scales[b]) return false;
	return true;
}
// PRIVATE derived authority. Validate the FULL covered page interval,
// including the 64B-aligned head and partial last page. Output is atomic.
struct PioneerQ38CohortExtent {
	uint32_t address=0,route=0,index=0,pages=0,bytes=0,blocks=0,reads=0,flits=0;
};
static inline bool PioneerQ38CohortAuthorizePart(const PioneerResidentBanks& banks,const tt_dram_samples_result& dram,
	const uint8_t* initialized,uint32_t initializedPages,unsigned type,unsigned inputs,const tt_q38_cohort_part& part,
	unsigned worker,PioneerQ38CohortExtent& output)
{
	if (!initialized || initializedPages!=PioneerResidentBanks::Pages || worker>=TT_Q38_COHORT_WORKERS
		|| !PioneerQ38CohortPartValid(type,inputs,part)) return false;
	PioneerQ38CohortExtent stage;
	unsigned head=unsigned(part.offset&4095);
	stage.bytes=PioneerQ38CohortBytes(type,inputs,part.groups); stage.pages=(head+stage.bytes+4095)/4096;
	stage.blocks=part.groups*(inputs/PioneerQ38CohortBlockInputs(type));
	stage.reads=stage.blocks; stage.flits=stage.bytes/64;
	if (worker) {
		unsigned inputBytes=inputs*2,scaleBytes=((inputs/PioneerQ38CohortBlockInputs(type))*4+63)&~63U;
		stage.reads+=(inputBytes+16383)/16384+1; stage.flits+=(inputBytes+scaleBytes)/64;
	}
	if (!banks.Page(dram,part.bank,part.offset-head,stage.address,stage.route,stage.index)
		|| stage.index>=initializedPages || stage.pages>initializedPages-stage.index) return false;
	stage.address+=head;
	for (unsigned p=0;p<stage.pages;p++) if (initialized[stage.index+p]!=1) return false;
	output=stage; return true;
}
static inline bool PioneerQ38CohortAuthorize(const PioneerResidentBanks& banks,const tt_dram_samples_result& dram,
	const uint8_t* initialized,uint32_t initializedPages,const tt_q38_cohort_probe& request,
	PioneerQ38CohortExtent* output)
{
	if (!PioneerQ38CohortValid(request)) return false;
	PioneerQ38CohortExtent one;
	// Caller holds the exclusive kernel owner lock. Request is a PRIVATE
	// copied snapshot;route authority+initialized bitmap are immutable under
	// that lock. Validate ALL before writing ANY output,then deterministically
	// derive again. Avoid120-element stack copies on Haiku's16KiB kernel stack.
	for (unsigned w=0;w<request.workers;w++) if (!PioneerQ38CohortAuthorizePart(banks,dram,initialized,initializedPages,
		request.type,request.inputs,request.part[w],w,one)) return false;
	if (!output) return true; // validation-only handler preflight,NO capability
	for (unsigned w=0;w<request.workers;w++) {
		PioneerQ38CohortAuthorizePart(banks,dram,initialized,initializedPages,request.type,request.inputs,request.part[w],w,one);
		output[w]=one;
	}
	for (unsigned w=request.workers;w<TT_Q38_COHORT_WORKERS;w++) output[w]={};
	return true;
}
#endif
