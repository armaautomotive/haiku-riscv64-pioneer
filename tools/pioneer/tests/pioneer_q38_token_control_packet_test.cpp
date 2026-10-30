/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenControlPacket.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
int main(){
 namespace P=PioneerQ38TokenControl;auto p=std::make_unique<P::Packet>();unsigned accepted=0,refused=0;
 auto reset=[&](unsigned operation){memset(p.get(),0,sizeof(*p));p->request.magic=P::Magic;
  p->request.version=P::Version;p->request.operation=operation;};
 auto good=[&](){assert(P::RequestShape(*p));accepted++;};
 auto bad=[&](auto& field,auto value){const auto saved=field;field=value;assert(!P::RequestShape(*p));
  assert(field==value);field=saved;refused++;};
 reset(P::Start);auto& r=p->request;r.prompts=2;r.limit=3;r.eos0=248318;r.eos1=248317;
 p->prompt[0]=248319;p->prompt[1]=17;good();r.stopEos=1;good();r.stopEos=0;
 for(unsigned value:{0U,2U,UINT32_MAX})bad(r.version,value);
 for(unsigned value:{0U,7U,UINT32_MAX})bad(r.operation,value);
 bad(r.magic,0U);
 for(auto& word:r.reserved){bad(word,1U);bad(word,UINT32_MAX);}
 for(unsigned value:{0U,8193U,UINT32_MAX})bad(r.prompts,value);
 for(unsigned value:{0U,1U,8193U,UINT32_MAX})bad(r.limit,value);
 bad(r.stopEos,2U);bad(r.eos0,unsigned(P::Vocabulary));bad(r.eos1,unsigned(P::Vocabulary));
 for(unsigned value:{1U,64U,UINT32_MAX})bad(r.records,value);
 for(unsigned i=0;i<P::Context;i++)bad(p->prompt[i],i<2?unsigned(P::Vocabulary):1U);
 auto* output=reinterpret_cast<unsigned char*>(&p->reply);
 for(size_t i=0;i<sizeof(p->reply);i++)bad(output[i],static_cast<unsigned char>(1));
 for(auto& word:p->records)bad(word,1U);
 good();
 for(unsigned operation:{unsigned(P::Observe),unsigned(P::Cancel),unsigned(P::Hold),unsigned(P::Finish)}){
  reset(operation);good();bad(r.prompts,1U);bad(r.limit,1U);bad(r.stopEos,1U);
  bad(r.eos0,1U);bad(r.eos1,1U);bad(r.records,1U);bad(p->prompt[0],1U);bad(p->prompt[P::Context-1],1U);
 }
 reset(P::Read);
 for(unsigned count:{0U,1U,64U}){r.records=count;good();}r.records=0;
 bad(r.records,65U);bad(r.records,UINT32_MAX);bad(p->records[P::Batch*P::RecordWords-1],1U);
 reset(P::Start);r.prompts=r.limit=P::Context;good();
 bad(p->prompt[P::Context-1],unsigned(P::Vocabulary));
 auto snapshot=std::make_unique<P::Packet>(*p);assert(P::RequestShape(*p) && !memcmp(p.get(),snapshot.get(),sizeof(*p)));
 printf("PASS private token packet/%u shape positives/%u atomic malformed+unused-prompt+caller-reply+record refusals/fixed49344B HEAP message;DATA grammar ONLY,no native/source/launch/terminal/cleanup/Work/physical/TPS authority\n",accepted,refused);
}
