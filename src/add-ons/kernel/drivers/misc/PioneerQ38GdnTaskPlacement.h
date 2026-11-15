/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_TASK_PLACEMENT_H
#define PIONEER_Q38_GDN_TASK_PLACEMENT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif

// Pure routing metadata for a possible layer-dependent GDN task layout.
// This does not select the layout, launch workers, or grant L1/device access.
namespace PioneerQ38GdnTaskPlacement {
enum : uint32_t {
	LogicalWorkers=96,
	PhysicalWorkers=119,
	RecurrentLayers=48,
	HeadsPerLayer=48,
	TasksPerLayer=LogicalWorkers,
	TotalTasks=RecurrentLayers*TasksPerLayer,
	SlotsPerPhysical=(TotalTasks+PhysicalWorkers-1)/PhysicalWorkers,
	StateHalfBytes=32768
};
// first physical worker for each layer's cyclic 96-task window. Keeping the
// small route table explicit avoids repeated modulo-by-119 operations on the
// on-card ROOT while preserving the exact Map() permutation.
static const unsigned char FirstPhysical[RecurrentLayers]={
	0,96,73,50,27,4,100,77,54,31,8,104,81,58,35,12,
	108,85,62,39,16,112,89,66,43,20,116,93,70,47,24,1,
	97,74,51,28,5,101,78,55,32,9,105,82,59,36,13,109
};
struct Route {
	uint32_t layerOrdinal,actualLayer,logicalWorker,physicalWorker,peer,stateIndex;
};

static inline uint32_t ActualLayer(uint32_t ordinal)
{
	if(ordinal>=RecurrentLayers)return 64;
	uint32_t groups=0,remainder=ordinal;
	while(remainder>=3){remainder-=3;groups++;}
	return ordinal+groups;
}

static inline bool LayerOrdinal(uint32_t actualLayer,uint32_t& ordinal)
{
	if(actualLayer>=64 || actualLayer%4==3)return false;
	ordinal=actualLayer-actualLayer/4;
	return ordinal<RecurrentLayers && ActualLayer(ordinal)==actualLayer;
}

static inline uint32_t Peer(uint32_t physicalWorker)
{
	if(!physicalWorker || physicalWorker>PhysicalWorkers)return ~uint32_t(0);
	const uint32_t x[]={1,2,3,4,5,6,7,10,11,12,13,14};
	uint32_t column=0,row=physicalWorker;
	while(row>=10){row-=10;column++;}
	if(column>=12)return ~uint32_t(0);
	return x[column]|((row+2)<<6);
}

static inline bool Map(uint32_t layerOrdinal,uint32_t logicalWorker,Route& out)
{
	if(layerOrdinal>=RecurrentLayers || logicalWorker>=LogicalWorkers)return false;
	const uint32_t ordinal=layerOrdinal*LogicalWorkers+logicalWorker;
	uint32_t physicalWorker=ordinal+1,stateIndex=0;
	while(physicalWorker>PhysicalWorkers){physicalWorker-=PhysicalWorkers;stateIndex++;}
	const uint32_t peer=Peer(physicalWorker);
	if(peer==~uint32_t(0))return false;
	out={layerOrdinal,ActualLayer(layerOrdinal),logicalWorker,physicalWorker,peer,
		stateIndex};
	return true;
}

static inline bool LogicalWorker(uint32_t actualLayer,uint32_t physicalWorker,uint32_t& logicalWorker)
{
	uint32_t layer=0;
	if(!LayerOrdinal(actualLayer,layer) || !physicalWorker || physicalWorker>PhysicalWorkers)return false;
	const uint32_t first=FirstPhysical[layer];
	const uint32_t physical=physicalWorker-1;
	const uint32_t candidate=physical>=first?physical-first:physical+PhysicalWorkers-first;
	if(candidate>=LogicalWorkers)return false;
	logicalWorker=candidate;return true;
}

static inline uint32_t StateSlots(uint32_t physicalWorker)
{
	if(!physicalWorker || physicalWorker>PhysicalWorkers)return 0;
	uint32_t count=0;
	for(uint32_t ordinal=physicalWorker-1;ordinal<TotalTasks;ordinal+=PhysicalWorkers)count++;
	return count;
}
}
#endif
