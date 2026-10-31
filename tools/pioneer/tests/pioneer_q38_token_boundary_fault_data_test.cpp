/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT 1
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38TokenBoundaryFaultData.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 namespace P=PioneerQ38TokenControl;
 auto p=std::make_unique<P::Packet>();p->request.magic=P::Magic;p->request.version=1;p->request.operation=P::Observe;
 auto& r=p->reply;r.version=1;r.operation=P::Observe;r.started=r.unsafe=1;r.stage=9;r.status=-15;
 r.detail[0]=r.detail[1]=1;r.detail[2]=9;r.detail[5]=1;r.detail[14]=0x51465431;r.detail[15]=0x642c0;
 const unsigned op=TT_PCI_PROBE_Q38_TOKEN_HIDDEN_CONTROL;
 unsigned accepted=0,refused=0;
 auto check=[&](const void* data,size_t bytes,unsigned code,bool okay){
  PioneerQ38TokenBoundaryFaultWords out={0x12345678,0x87654321};
  const auto snapshot=std::make_unique<P::Packet>(*p);
  assert(PioneerQ38TokenBoundaryCopyFaultData(code,data,bytes,out)==okay);
  if(okay){assert(out.error==r.detail[14] && out.savedLink==r.detail[15]);accepted++;}
  else{assert(out.error==0x12345678 && out.savedLink==0x87654321);refused++;}
  assert(!std::memcmp(p.get(),snapshot.get(),sizeof(*p)));
 };
 for(uint32_t link:{0U,2U,0x642c0U,0x60004U,0xffffffffU}){r.detail[15]=link;check(p.get(),sizeof(*p),op,true);}
 for(int32_t status:{-1,-15,INT32_MIN}){r.status=status;check(p.get(),sizeof(*p),op,true);}
 for(unsigned index=0;index<16;index++){
  auto* words=reinterpret_cast<uint32_t*>(&p->request);words[index]^=1;
  check(p.get(),sizeof(*p),op,false);words[index]^=1;
 }
 for(unsigned index=0;index<32;index++)if(index!=8 && index!=31){
  auto* words=reinterpret_cast<uint32_t*>(&p->reply);words[index]^=1;
  check(p.get(),sizeof(*p),op,false);words[index]^=1;
 }
 r.status=0;check(p.get(),sizeof(*p),op,false);r.status=-15;
 for(size_t bytes:{size_t(0),sizeof(*p)-1,sizeof(*p)+1})check(p.get(),bytes,op,false);
 for(unsigned code:{0U,op-1,op+1,UINT32_MAX})check(p.get(),sizeof(*p),code,false);
 check(nullptr,sizeof(*p),op,false);
 std::vector<unsigned char> unaligned(sizeof(*p)+1);std::memcpy(unaligned.data()+1,p.get(),sizeof(*p));
 check(unaligned.data()+1,sizeof(*p),op,true);
 // Irrelevant model payload is NEVER interpreted or made into completion.
 p->prompt[8191]=0xffffffff;p->records[4095]=0x7fc01234;check(p.get(),sizeof(*p),op,true);
 printf("PASS boundary fault reply DATA/%u link+status+unaligned+opaque-payload cases/%u header+reserved+opcode+size+null refusals/input+failed-output unchanged;DATA ONLY,NOT authenticated user-copy,retired PC,STOP,cleanup or retry authority\n",accepted,refused);
}
