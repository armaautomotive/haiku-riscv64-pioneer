/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PACKED_PLAN_H
#define PIONEER_QWEN35_PACKED_PLAN_H
#include "PioneerQwen35TiledReader.h"
#include "PioneerQwen35Embedding.h"

// Offline proposal, not a device allocation, executable route or session.
// Tied embeddings are allocated once. Recurrent/KV sizes are logical storage
// requirements; a future worker must independently prove its state layout.
enum { PIONEER_Q35_WEIGHT=0,PIONEER_Q35_DELTA_STATE=1,PIONEER_Q35_CONV_STATE=2,
 PIONEER_Q35_KV_STATE=3,PIONEER_Q35_WORKSPACE=4 };
struct PioneerQ35PackedPart {
 unsigned kind,layer,role,tensor,bank,firstRow,rows,rowBytes;
 uint32_t proposedAddress,bytes,allocated;
};
struct PioneerQ35EmbeddingProposal {
 unsigned part,bank,lane;
 uint32_t address;
};
class PioneerQwen35PackedPlan {
 std::vector<PioneerQ35PackedPart> parts,empty;
 uint64_t used[7]={},epoch=0;
 bool ready=false;
 const PioneerQwen35Gguf* bound=nullptr;
 mutable PioneerQwen35TiledReader tiledReader;
 bool Stripe(unsigned kind,unsigned layer,unsigned role,unsigned tensor,unsigned rows,unsigned rowBytes,unsigned unit) {
  if (!rows || !rowBytes || !unit) return false;
  unsigned counts[7]={},chunks=rows/unit,tail=rows%unit,count=chunks<7?1:7;
  if (count==1) counts[0]=rows;
  else { for (unsigned b=0;b<7;b++) counts[b]=(chunks/7+unsigned(b<chunks%7))*unit; counts[6]+=tail; }
  unsigned banks[]={0,1,2,3,4,5,6};
  std::sort(banks,banks+7,[this](unsigned a,unsigned b){return used[a]<used[b] || (used[a]==used[b] && a<b);});
  unsigned first=0;
  for (unsigned p=0;p<count;p++) {
   unsigned bank=banks[p]; uint64_t bytes=uint64_t(counts[p])*rowBytes,allocated=(bytes+4095)&~UINT64_C(4095);
   if (!counts[p] || bytes>UINT32_MAX || allocated>Capacity-used[bank]) return false;
   parts.push_back({kind,layer,role,tensor,bank,first,counts[p],rowBytes,
    uint32_t(Base+used[bank]),uint32_t(bytes),uint32_t(allocated)});
   used[bank]+=allocated; first+=counts[p];
  }
  return first==rows;
 }
public:
 static constexpr uint64_t Capacity=UINT64_C(3)*1024*1024*1024;
 static constexpr uint32_t Base=0x04000000;
 bool Ready() const { return ready && bound && bound->Epoch()==epoch && bound->Stable(); }
 bool MatchesSource(const PioneerQwen35Gguf& source) const { return bound==&source && Ready(); }
 const std::vector<PioneerQ35PackedPart>& Parts() const { return Ready()?parts:empty; }
 uint64_t Used(unsigned bank) const { return Ready() && bank<7?used[bank]:0; }
 bool ResolveEmbedding(unsigned token,PioneerQ35EmbeddingProposal& output) const {
  namespace E=PioneerQwen35Embedding;
  if (!Ready() || token>=E::Vocabulary) return false;
  for (unsigned index=0;index<parts.size();index++) {
   const auto& part=parts[index];
   if (part.kind!=PIONEER_Q35_WEIGHT || part.layer!=32 || part.role!=0
    || token<part.firstRow || token-part.firstRow>=part.rows) continue;
   const unsigned row=token-part.firstRow;
   const unsigned offset=(row/16)*E::GroupBytes;
   if (part.rowBytes*16!=E::GroupBytes || offset>part.bytes || E::GroupBytes>part.bytes-offset)
    return false;
   output={index,part.bank,row%16,part.proposedAddress+offset};
   return true;
  }
  return false;
 }
 bool Build(const PioneerQwen35Gguf& source,unsigned context,bool doubleDelta=false) {
  ready=false; bound=nullptr; epoch=0; tiledReader.Reset(); parts.clear();
  for (auto& bytes:used) bytes=0;
  if (!source.Stable() || !context || context>32768) return false;
  for (unsigned ordinal=0;ordinal<=32;ordinal++) {
   unsigned layer=ordinal?ordinal-1:32;
   for (unsigned role=0;;role++) {
    const auto* expected=PioneerQ35Role(layer,role); if (!expected) break;
    char name[96];
    if (layer==32) snprintf(name,sizeof(name),"%s",expected->name);
    else snprintf(name,sizeof(name),"blk.%u.%s",layer,expected->name);
    const auto* tensor=source.Find(name); if (!tensor) return false;
    unsigned index=unsigned(tensor-source.Tensors().data());
    unsigned rowBytes=unsigned(tensor->bytes/tensor->outputs);
    if (tensor->type) {
     if (tensor->outputs%16) return false;
     rowBytes=PioneerQ35TiledRowBytes(tensor->type,tensor->inputs);
     if (!rowBytes) return false;
    }
    if (!Stripe(PIONEER_Q35_WEIGHT,layer,role,index,unsigned(tensor->outputs),rowBytes,64)) return false;
   }
  }
  for (unsigned layer=0;layer<32;layer++) {
   if (layer%4==3) {
    if (!Stripe(PIONEER_Q35_KV_STATE,layer,0,426,8,context*256*2,1)) return false;
   } else {
    for (unsigned slot=0;slot<(doubleDelta?2U:1U);slot++)
     if (!Stripe(PIONEER_Q35_DELTA_STATE,layer,slot,426,32,128*128*4,1)) return false;
    if (!Stripe(PIONEER_Q35_CONV_STATE,layer,0,426,8192,3*4,64)) return false;
   }
  }
  for (unsigned bank=0;bank<7;bank++) {
   const unsigned bytes=64*1024*1024;
   if (bytes>Capacity-used[bank]) return false;
   parts.push_back({PIONEER_Q35_WORKSPACE,32,0,426,bank,0,1,bytes,uint32_t(Base+used[bank]),bytes,bytes});
   used[bank]+=bytes;
  }
  ready=source.Stable() && tiledReader.Bind(source);
  if (ready) { bound=&source; epoch=source.Epoch(); }
  return ready;
 }
 bool ReadWeightPage(PioneerQwen35Gguf& source,unsigned fragment,unsigned page,uint8_t* output,unsigned capacity) const {
  if (!Ready() || &source!=bound || fragment>=parts.size() || !output || capacity<4096) return false;
  const auto& part=parts[fragment];
  if (part.kind!=PIONEER_Q35_WEIGHT || page>=part.allocated/4096) return false;
  if (source.Tensors()[part.tensor].type)
   return tiledReader.ReadPage(source,part.tensor,part.firstRow,part.rows,page,output,capacity);
  uint64_t relative=uint64_t(page)*4096;
  if (relative>=part.bytes) return false;
  unsigned bytes=unsigned(std::min(UINT64_C(4096),uint64_t(part.bytes)-relative));
  uint8_t stage[4096]={};
  if (!source.ReadPacked(part.tensor,uint64_t(part.firstRow)*part.rowBytes+relative,bytes,stage,sizeof(stage))) return false;
  memcpy(output,stage,sizeof(stage)); return true;
 }
};
#endif
