/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_TASK_PLACEMENT_H
#define PIONEER_Q38_GDN_TASK_PLACEMENT_H
#include <stdint.h>

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
	StateHalfBytes=32768
};
struct Route {
	uint32_t layerOrdinal,actualLayer,logicalWorker,physicalWorker,peer,stateIndex;
};

static inline uint32_t ActualLayer(uint32_t ordinal)
{
	return ordinal<RecurrentLayers ? ordinal+ordinal/3 : 64;
}

static inline bool LayerOrdinal(uint32_t actualLayer,uint32_t& ordinal)
{
	if(actualLayer>=64 || actualLayer%4==3)return false;
	ordinal=actualLayer-actualLayer/4;
	return ordinal<RecurrentLayers && ActualLayer(ordinal)==actualLayer;
}

static inline uint32_t Peer(uint32_t physicalWorker)
{
	if(!physicalWorker || physicalWorker>PhysicalWorkers)return UINT32_MAX;
	const uint32_t x[]={1,2,3,4,5,6,7,10,11,12,13,14};
	uint32_t column=0,row=physicalWorker;
	while(row>=10){row-=10;column++;}
	if(column>=12)return UINT32_MAX;
	return x[column]|((row+2)<<6);
}

static inline bool Map(uint32_t layerOrdinal,uint32_t logicalWorker,Route& out)
{
	if(layerOrdinal>=RecurrentLayers || logicalWorker>=LogicalWorkers)return false;
	const uint32_t ordinal=layerOrdinal*LogicalWorkers+logicalWorker;
	const uint32_t physicalWorker=ordinal%PhysicalWorkers+1;
	const uint32_t peer=Peer(physicalWorker);
	if(peer==UINT32_MAX)return false;
	out={layerOrdinal,ActualLayer(layerOrdinal),logicalWorker,physicalWorker,peer,
		ordinal/PhysicalWorkers};
	return true;
}

static inline bool LogicalWorker(uint32_t actualLayer,uint32_t physicalWorker,uint32_t& logicalWorker)
{
	uint32_t layer=0;
	if(!LayerOrdinal(actualLayer,layer) || !physicalWorker || physicalWorker>PhysicalWorkers)return false;
	const uint32_t first=(layer*LogicalWorkers)%PhysicalWorkers;
	const uint32_t candidate=(physicalWorker-1+PhysicalWorkers-first)%PhysicalWorkers;
	if(candidate>=LogicalWorkers)return false;
	Route route={};if(!Map(layer,candidate,route) || route.physicalWorker!=physicalWorker)return false;
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
