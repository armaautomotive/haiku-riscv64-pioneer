/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_RUNTIME_PLAN_H
#define PIONEER_Q38_TOKEN_RUNTIME_PLAN_H
#include "PioneerQ38TokenStartupPlan.h"
#include "PioneerQ38TokenRope.h"
// Exact immutable startup write recipe,NOT initialized cache/model products,
// an uploaded/authenticated model,source/clock/native ownership or RUN grant.
// KV prefix is EMPTY:do NOT fill stale cache rows or mark them readable.
class PioneerQ38TokenRuntimePlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38TokenStartupPlan* startup=nullptr;
 PioneerQ38GdnIdentity identity={};unsigned bank=7,workspace=0;bool attempted=false,ready=false;
 bool Output(const void* output,unsigned bytes)const{
  const uintptr_t o=reinterpret_cast<uintptr_t>(output),s=reinterpret_cast<uintptr_t>(this),r=reinterpret_cast<uintptr_t>(PioneerQ38TokenRope::Data);
  return output && bytes && o<=UINTPTR_MAX-bytes && !(o<s+sizeof(*this) && s<o+bytes)
   && !(o<r+PioneerQ38TokenRope::Bytes && r<o+bytes);
 }
public:
 enum {PackageChunks=2048,RopeChunks=128,OutputChunks=128,Chunks=2308,PayloadBytes=37782528};
 struct Chunk {unsigned kind,layer,offset,bank,address,bytes;};
 PioneerQ38TokenRuntimePlan()=default;
 PioneerQ38TokenRuntimePlan(const PioneerQ38TokenRuntimePlan&)=delete;
 PioneerQ38TokenRuntimePlan& operator=(const PioneerQ38TokenRuntimePlan&)=delete;
 bool Bind(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& data,const PioneerQ38GdnIdentity& id,const uint32_t* source){
  if(attempted)return false;
  attempted=true;
  if(!data.MatchesBinding(c,data.Bank(),id,source) || !data.MatchesPublishedBinding(c,data.Bank(),id,source))return false;
  canonical=&c;startup=&data;identity=id;bank=data.Bank();workspace=data.Workspace();ready=true;return true;
 }
 bool MatchesBinding(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& data,const PioneerQ38GdnIdentity& id,const uint32_t* source)const{
  return ready && canonical==&c && startup==&data && identity==id && bank==data.Bank() && workspace==data.Workspace()
   && data.MatchesPublishedBinding(c,bank,id,source);
 }
 bool Describe(unsigned ordinal,Chunk& output)const{
  namespace C=PioneerQ38GlobalChainLayout;namespace T=PioneerQ38TokenLayout;
  if(!ready || !canonical || !canonical->Ready() || !startup || ordinal>=Chunks)return false;
  Chunk d={0,64,0,bank,0,16384};unsigned n=ordinal;
  if(n<PackageChunks){d.kind=1;d.layer=n/32;d.offset=(n%32)*16384;
   if(!startup->PackageAddress(d.layer,d.address))return false;
   d.address+=d.offset;
  }else if((n-=PackageChunks)<RopeChunks){d.kind=2;d.offset=n*16384;d.address=workspace+C::D::RopeOffset+d.offset;
  }else if((n-=RopeChunks)==0){d.kind=3;d.bytes=512;d.address=workspace+C::HeadOffset;
  }else if(--n==0){d.kind=4;d.bytes=512;d.address=workspace+T::SessionOffset;
  }else if(--n<2){d.kind=5;d.offset=n*16384;d.address=workspace+T::PromptOffset+d.offset;
  }else{n-=2;d.kind=6;d.offset=n*16384;d.address=workspace+T::OutputOffset+d.offset;}
  if(d.bank>=7 || !d.bytes || (d.address|d.bytes)&63 || d.address<workspace
   || d.address>workspace+C::WorkspaceBytes-d.bytes)return false;
  output=d;return true;
 }
 bool Read(unsigned ordinal,uint8_t* output,unsigned bytes)const{
  Chunk d;if(!Output(output,bytes) || !Describe(ordinal,d) || bytes!=d.bytes)return false;
  if(d.kind==1)return startup->ReadPublishedPackage(d.layer,d.offset,output,bytes);
  if(d.kind==3)return startup->ReadHead(0,output,bytes);
  if(d.kind==4)return startup->ReadSession(0,output,bytes);
  if(d.kind==5)return startup->ReadPrompt(d.offset,output,bytes);
  // SAME complete startup/package/canonical alias refusal for raw/zero DATA.
  if(!startup->PublishedOutput(output,bytes))return false;
  for(unsigned i=0;i<bytes;i++)output[i]=d.kind==2?
   uint8_t(PioneerQ38TokenRope::Data[(d.offset+i)/4]>>(((d.offset+i)&3)*8)):0;
  return true;
 }
 unsigned Bank()const{return ready?bank:7;}
 unsigned Workspace()const{return ready?workspace:0;}
};
static_assert(PioneerQ38TokenRuntimePlan::PayloadBytes==64*524288+2097152+512+512+32768+2097152,"complete startup payload");
#endif
