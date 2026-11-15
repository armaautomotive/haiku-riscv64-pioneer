/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_L1_WORKER_FOOTPRINT_H
#define PIONEER_Q38_GDN_L1_WORKER_FOOTPRINT_H
#include "PioneerQ38DeltaVectorLayout.h"

// Complete fixed-address L1 live ranges for the combined recurrent worker.
// The pushed-image snapshot reserves most of these already, but its historical
// address map does not cover Query/Key/Value/Output at 0xb0000. A persistent
// state cache must not allocate pages over that active scratch page.
namespace PioneerQ38GdnL1WorkerFootprint {
struct Range { unsigned begin,bytes; };
enum { CodeBytes=0x8000, DescriptorBytes=0x200, StateBytes=0x8000,
       StackBytes=1024, QueryBytes=320*4, KeyBytes=128*4,
       ValueBytes=64*4, OutputBytes=64*4 };
static inline unsigned RangeCount() { return 11; }
static inline Range GetRange(unsigned index)
{
 switch(index){
		case 0:return {PioneerQ38DeltaVectorLayout::Code,CodeBytes};
  case 1:return {0x70000-StackBytes,StackBytes};
		case 2:return {PioneerQ38DeltaVectorLayout::Descriptor,DescriptorBytes};
		case 3:return {PioneerQ38DeltaVectorLayout::Prior,StateBytes};
		case 4:return {PioneerQ38DeltaVectorLayout::Next,StateBytes};
		case 5:return {PioneerQ38DeltaVectorLayout::Query,QueryBytes};
		case 6:return {PioneerQ38DeltaVectorLayout::Key,KeyBytes};
		case 7:return {PioneerQ38DeltaVectorLayout::Value,ValueBytes};
		case 8:return {PioneerQ38DeltaVectorLayout::Decay,4};
		case 9:return {PioneerQ38DeltaVectorLayout::Beta,4};
		default:return {PioneerQ38DeltaVectorLayout::Output,OutputBytes};
 }
}
static inline bool Overlaps(unsigned address,unsigned bytes)
{
 if(!bytes || address>0x180000 || bytes>0x180000-address)return true;
 for(unsigned i=0;i<RangeCount();i++){
  const Range range=GetRange(i);
  if(address<range.begin+range.bytes && range.begin<address+bytes)return true;
 }
 return false;
}
}
#endif
