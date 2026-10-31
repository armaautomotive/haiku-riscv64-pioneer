/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CARD_WORK_REQUEST_DATA_H
#define PIONEER_Q38_CARD_WORK_REQUEST_DATA_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenControlPacket.h"

// Future Assistant-to-resident-parent DATA conversion ONLY. No ioctl number,
// driver tag,capability,source authentication,execution,completion or Work
// activation. In particular the existing tag74 diagnostic must NOT accept
// these requests. The caller owns the input/output for this entire call.
namespace PioneerQ38CardWorkRequestData {
namespace P=PioneerQ38TokenControl;
enum {MaxPrompts=96,MaxOutputs=32,MaxPositions=MaxPrompts+MaxOutputs-1};
static_assert(MaxPositions<128 && MaxPositions<P::Context,"bounded Assistant context");

static inline bool Header(const P::Request& r){
 if(r.magic!=P::Magic || r.version!=P::Version || r.operation!=P::Start
  || !r.prompts || r.prompts>MaxPrompts || r.limit<r.prompts || r.limit>MaxPositions
  || r.limit-r.prompts>=MaxOutputs || r.stopEos!=1 || r.eos0>=P::Vocabulary
  || r.eos1>=P::Vocabulary || r.records)return false;
 for(auto word:r.reserved)if(word)return false;
 return true;
}
static inline bool Shape(const P::Packet& p){return Header(p.request) && P::RequestShape(p);}

static inline bool Build(P::Packet& out,const uint32_t* input,unsigned prompts,
 unsigned outputs,unsigned eos0,unsigned eos1){
 if(!input || !prompts || prompts>MaxPrompts || !outputs || outputs>MaxOutputs
  || eos0>=P::Vocabulary || eos1>=P::Vocabulary)return false;
 const uintptr_t source=reinterpret_cast<uintptr_t>(input),target=reinterpret_cast<uintptr_t>(&out);
 const unsigned bytes=prompts*sizeof(uint32_t);
 if(source%alignof(uint32_t) || source>UINTPTR_MAX-bytes || target>UINTPTR_MAX-sizeof(out)
  || (source<target+sizeof(out) && target<source+bytes))return false;
 const auto* raw=reinterpret_cast<const unsigned char*>(&out);
 for(unsigned i=0;i<sizeof(out);i++)if(raw[i])return false;
 uint32_t staged[MaxPrompts];
 for(unsigned i=0;i<prompts;i++){
  staged[i]=input[i];if(staged[i]>=P::Vocabulary)return false;
 } // read each caller word ONCE;reject before changing any output byte
 auto& r=out.request;
 r.magic=P::Magic;r.version=P::Version;r.operation=P::Start;r.prompts=prompts;
 // The LAST prompt position predicts output0. N outputs need N-1 further
 // decode positions,not N. A one-output request has ZERO warm decode steps.
 r.limit=prompts+outputs-1;r.stopEos=1;r.eos0=eos0;r.eos1=eos1;
 for(unsigned i=0;i<prompts;i++)out.prompt[i]=staged[i];
 return true;
} // pristine HEAP packet supplied by caller;all refusals preserve it

static inline bool OutputOrdinalData(const P::Request& r,unsigned position,unsigned& ordinal){
 if(!Header(r) || position<r.prompts-1 || position>=r.limit)return false;
 ordinal=position-(r.prompts-1);return true;
} // position arithmetic ONLY,NOT proof that a record exists or can be delivered
}
#endif
