/* SPDX-License-Identifier: MIT */
/* Separate routed-token candidate L1 journal geometry; DATA bounds only. */
#ifndef PIONEER_Q38_TOKEN_REDISTRIBUTED_FOOTPRINT_H
#define PIONEER_Q38_TOKEN_REDISTRIBUTED_FOOTPRINT_H
#include <stdint.h>
namespace PioneerQ38TokenRedistributedL1 {
struct Span {uint32_t address,words;};
enum {Family=0x31575251,Workers=120,RootSavedWords=316939,PeerSavedWords=94217,
	SavedWords=RootSavedWords+119*PeerSavedWords,SavedBytes=SavedWords*4};
static inline unsigned SnapshotSpans(unsigned worker){return worker>=Workers?0:7;}
static inline Span SnapshotSpan(unsigned worker,unsigned span){
	if(worker>=Workers || span>=SnapshotSpans(worker))return {UINT32_MAX,0};
	if(!span)return {0x6000,1};
	if(!worker){
		switch(span){
			case 1:return {0x40000,32768};
			case 2:return {0x60000,2};
			case 3:return {0x64000,8192};
			case 4:return {0x6e000,2048};
			case 5:return {0x74000,273920};
			default:return {0x17ffc0,8};
		}
	}
	switch(span){
		case 1:return {0x20000,2048};
		case 2:return {0x40000,4096};
		case 3:return {0x60000,16384};
		case 4:return {0x70000,61440};
		case 5:return {0xb8000,10240}; // candidate GDN text ends at 0xc1d40
		default:return {0x17ffc0,8};
	}
}
static inline unsigned SnapshotWords(unsigned worker){return worker>=Workers?0:worker?PeerSavedWords:RootSavedWords;}
static inline unsigned SnapshotBase(unsigned worker){
	return worker>=Workers?UINT32_MAX:worker?RootSavedWords+(worker-1)*PeerSavedWords:0;
}
static inline uint32_t SnapshotAddress(unsigned worker,unsigned ordinal){
	if(worker>=Workers || ordinal>=SnapshotWords(worker))return UINT32_MAX;
	for(unsigned i=0;i<SnapshotSpans(worker);i++){
		const auto span=SnapshotSpan(worker,i);
		if(ordinal<span.words)return span.address+ordinal*4;
		ordinal-=span.words;
	}
	return UINT32_MAX;
}
static inline bool SnapshotContains(unsigned worker,uint32_t address){
	if(worker>=Workers || (address&3))return false;
	for(unsigned i=0;i<SnapshotSpans(worker);i++){
		const auto span=SnapshotSpan(worker,i);
		if(address>=span.address && address-span.address<span.words*4)return true;
	}
	return false;
}
static_assert(1+32768+2+8192+2048+273920+8==RootSavedWords,"complete root originals");
static_assert(1+2048+4096+16384+61440+10240+8==PeerSavedWords,"complete routed worker originals");
static_assert(SavedWords==11528762 && SavedBytes==46115048,"120-worker routed image journal size");
}
#endif
