/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION) || PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION != 1
#error Recurrent L1 state placement is inspection-only
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
 || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE)
#error Inspection-only L1 state placement cannot select Work,validation,or native execution
#endif
#ifndef PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION_H
#define PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION_H
#include <cstdint>

// Static layout investigation only: no IO, model allocation, launch, or
// assertion that live kernels can coexist with this proposed reservation.
struct PioneerQ38GdnL1CachePlanInspection {
	enum : uint32_t {
		L1Bytes=0x180000, Workers=96, RecurrentLayers=48, HeadsPerLayer=48,
		HalvesPerLayer=HeadsPerLayer*2, HalfStateBytes=32768,
		TotalHalves=RecurrentLayers*HalvesPerLayer,
		TotalStateBytes=TotalHalves*HalfStateBytes,
		BaseSlots=TotalHalves/Workers, ExtraSlots=TotalHalves%Workers,
		MaxSlots=BaseSlots+(ExtraSlots!=0), MaxStateBytes=MaxSlots*HalfStateBytes,
		MinimumScratch=L1Bytes-MaxStateBytes
	};
	struct Slot { uint32_t worker,ordinal,index,offset,bytes; };

	static bool Map(unsigned layerOrdinal,unsigned head,unsigned half,Slot& out)
	{
		if(layerOrdinal>=RecurrentLayers || head>=HeadsPerLayer || half>=2)return false;
		const uint32_t ordinal=layerOrdinal*HalvesPerLayer+head*2+half;
		const uint32_t worker=ordinal%Workers,index=ordinal/Workers;
		const uint32_t offset=index*HalfStateBytes;
		if(index>=MaxSlots || offset>L1Bytes || HalfStateBytes>L1Bytes-offset)return false;
		out={worker,ordinal,index,offset,HalfStateBytes};return true;
	}
	static unsigned SlotsForWorker(unsigned worker)
	{
		if(worker>=Workers)return 0;
		return BaseSlots+(worker<ExtraSlots?1:0);
	}
	static unsigned ScratchBytes(unsigned worker)
	{
		const unsigned slots=SlotsForWorker(worker);
		return slots?L1Bytes-slots*HalfStateBytes:0;
	}
	static unsigned ActualLayer(unsigned layerOrdinal)
	{
		return layerOrdinal<RecurrentLayers?layerOrdinal+layerOrdinal/3:64;
	}
	static bool IsRecurrentLayer(unsigned layer)
	{
		return layer<64 && layer%4!=3;
	}
	static bool LayerWorkersUnique(unsigned layerOrdinal)
	{
		if(layerOrdinal>=RecurrentLayers)return false;
		bool seen[Workers]={};
		for(unsigned h=0;h<HalvesPerLayer;h++){
			Slot slot={};if(!Map(layerOrdinal,h/2,h%2,slot)||seen[slot.worker])return false;
			seen[slot.worker]=true;
		}
		return true;
	}
};
#endif
