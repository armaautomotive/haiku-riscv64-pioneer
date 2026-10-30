/* SPDX-License-Identifier: MIT */
// TEST-only complete original learned-byte fabric. No CPU model products,
// native capability or permission to upload/execute on physical hardware.
#ifndef PIONEER_Q38_TOKEN_ORIGINAL_FABRIC_H
#define PIONEER_Q38_TOKEN_ORIGINAL_FABRIC_H
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
#include "pioneer_q38_token_flow_test.cpp"
#include "pioneer_q38_whole_original_bytes.h"
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
#include "PioneerQ38ObservedTrace.h"
static_assert(PioneerQ38ObservedTrace::TokenMagic==TL::TokenMagic,"exact recorded DATA format,not execution authority");
#endif
struct OriginalTokenFlowMachine:TokenFlowMachine {
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
 PioneerQ38ObservedTrace* observed=nullptr;
 const char* snapshotFirstLayerDirectory=nullptr;
 void ObservedVocabulary(unsigned pos,unsigned first,const uint32_t* actual,unsigned count)override{
  if(observed)assert(observed->Vocabulary(pos,first,actual,count));
 }
#endif
 WholeModelOriginalBytes& original;
 std::map<uint64_t,const PioneerQ38CanonicalPart*> learned;
 unsigned removedLearned=0,removedRecipes=0;
 OriginalTokenFlowMachine(const char* image,const char* old,const char* worker,WholeModelOriginalBytes& source,bool eos=false)
  :TokenFlowMachine(image,old,worker,eos),original(source){
  assert(original.Ready() && !steps && !packets && !peerWrites && !parent);
  // Classify learned coordinates from the independently rebuilt canonical
  // geometry, NEVER from the reader's Contains/Ready result. A missing or
  // revoked source must not reclassify a learned byte as mutable/synthetic.
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* part=canonical->Part(i);assert(part);
   if(part->kind==canonical->WeightKind)assert(learned.emplace(Key(PioneerQ38ProjectionRoute(part->bank),canonical->Base+part->offset),part).second);
  }
  assert(learned.size()==3551);
  // The base's synthetic seeds are construction-only, BEFORE any ROOT
  // instruction. Erase ALL such learned words and matrix recipes, including
  // learned norms/Conv/GDN/head/embedding, not merely the first layer.
  for(auto it=dram.begin();it!=dram.end();){
   if(Learned(unsigned(it->first>>32),unsigned(it->first),4)){it=dram.erase(it);removedLearned++;}else ++it;
  }
  removedRecipes=unsigned(spans.size());spans.clear();assert(removedLearned && removedRecipes);
  // No supplied synthetic initial hidden: the actual embedding producer
  // must create the first model input. Unknown reads now fail, not zero-fill.
  for(unsigned i=0;i<5120;i++)assert(dram.erase(Key(outer[8],outer[9]+0x600000+i*4))==1);
  input.fill(0); // observer only; StartLayer replaces it from actual embedding
  for(const auto& raw:dram)assert(!OverlapsLearned(unsigned(raw.first>>32),unsigned(raw.first),4));
  for(const auto& zero:rawZero)assert(!OverlapsLearned(unsigned(zero.first>>32),unsigned(zero.first),zero.second));
 }
 const PioneerQ38CanonicalPart* Learned(unsigned route,unsigned address,unsigned bytes)const{
  if(!bytes || uint64_t(address)+bytes>UINT64_C(0x100000000))return nullptr;
  auto found=learned.upper_bound(Key(route,address));if(found==learned.begin())return nullptr;--found;
  const auto* p=found->second;unsigned start=canonical->Base+p->offset;
  return route==PioneerQ38ProjectionRoute(p->bank) && address>=start && address-start<p->allocated
   && bytes<=p->allocated-(address-start)?p:nullptr;
 }
 bool OverlapsLearned(unsigned route,unsigned address,unsigned bytes)const{
  assert(bytes && uint64_t(address)+bytes<=UINT64_C(0x100000000));
  auto next=learned.lower_bound(Key(route,address));
  if(next!=learned.end() && unsigned(next->first>>32)==route && uint64_t(unsigned(next->first))<uint64_t(address)+bytes)return true;
  if(next!=learned.begin()){
   const auto& previous=*std::prev(next);const auto* p=previous.second;
   if(unsigned(previous.first>>32)==route && uint64_t(unsigned(previous.first))+p->allocated>address)return true;
  }
  return false;
 }
 uint32_t Get(unsigned route,unsigned address)const override{
  assert(!(address&3));
  if(Learned(route,address,4)){
   uint8_t raw[4];assert(original.Read(route,address,raw,4));
   return uint32_t(raw[0])|uint32_t(raw[1])<<8|uint32_t(raw[2])<<16|uint32_t(raw[3])<<24;
  }
  assert(!OverlapsLearned(route,address,4) && spans.empty());
  return GlobalChainInstructionMachine::Get(route,address);
 }
 void ReadRaw(unsigned route,unsigned address,uint8_t* out,unsigned bytes)const override{
  assert(out && bytes && bytes<=16384 && !((address|bytes)&3));
  if(Learned(route,address,bytes)){assert(original.Read(route,address,out,bytes));return;}
  assert(!OverlapsLearned(route,address,bytes) && spans.empty());
  CohortBytes::ReadRaw(route,address,out,bytes);
 }
 void Put(unsigned route,unsigned address,uint32_t value)override{
  assert(!(address&3) && !OverlapsLearned(route,address,4));
  GlobalChainInstructionMachine::Put(route,address,value);
 }
 void BeforeFork()override{assert(original.PrepareFork());}
 const char* RawInputScope()const override{return "AUTHENTICATED original GGUF learned bytes;NOT independent whole-model numerical/native/physical/Work/TPS qualification";}
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040;
  unsigned priorLayers=allLayers;
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
  unsigned priorEmbedding=embedded,priorPublications=tokenPublications;
#endif
  if(packet)assert(original.Ready());
  TokenFlowMachine::Store(address,value);
  if(packet)assert(original.Ready());
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
  if(observed && embedded!=priorEmbedding){
   assert(embedded==priorEmbedding+1);std::array<uint32_t,5120> actual;
   for(unsigned i=0;i<5120;i++)actual[i]=Get(outer[8],outer[9]+PioneerQ38GlobalFeedbackLayout::Offset+i*4);
   assert(observed->Embedding(embeddingTarget,embeddingToken,actual.data(),5120));
  }
  if(observed && allLayers!=priorLayers){
   assert(observed->Layer(position,layer,input.data(),5120));
   if(snapshotFirstLayerDirectory && !position && !layer){
    assert(allLayers==1 && !committed && observed->Embeddings()==1 && observed->Layers(0)==1
     && !observed->Complete() && observed->SnapshotFirstLayerDiagnostic(snapshotFirstLayerDirectory)
     && !observed->Complete() && !observed->SavedComplete());
    puts("SNAPSHOT genuine FIRST_LAYER_DIAGNOSTIC/actual original embedding+layer0 only/SAME119 histories CONTINUE/no token or STOP completion;NOT numerical match/native/physical/Work/TPS");
   }
  }
  if(observed && tokenPublications!=priorPublications){
   assert(tokenPublications==priorPublications+1);std::array<uint32_t,64> actual;
   for(unsigned i=0;i<64;i++)actual[i]=Get(outer[8],outer[9]+TL::OutputOffset+position*256+i*4);
   assert(observed->Commit(position,actual.data(),64));
  }
#endif
  // Observation only: expose real executed-PC hot spots at each verified
  // layer instead of waiting for all three tokens. SIMULATED polling is
  // included; these counts are never physical timing/TPS or an input hint.
  if(allLayers!=priorLayers){assert(allLayers==priorLayers+1);profile.Dump(profile.Total());}
 }
 void VerifyRawEndpoints(){
  assert(!steps && !packets && !peerWrites && !parent && original.Ready() && spans.empty());
  unsigned reads=0;
  for(const auto& span:learned){
   unsigned route=unsigned(span.first>>32),start=unsigned(span.first),size=span.second->allocated;
   for(unsigned address:{start,start+size-64}){
    std::array<uint8_t,64> direct,batch;assert(original.Read(route,address,direct.data(),64));ReadRaw(route,address,batch.data(),64);assert(direct==batch);
    for(unsigned i=0;i<64;i+=4){unsigned word=0;for(unsigned b=0;b<4;b++)word|=uint32_t(direct[i+b])<<(b*8);assert(Get(route,address+i)==word);}
    reads++;
   }
  }
  std::array<uint8_t,64> direct,batch;unsigned metadata=outer[9]+TL::SessionOffset;
  ReadRaw(outer[8],metadata,batch.data(),64);CohortBytes::ReadRaw(outer[8],metadata,direct.data(),64);assert(direct==batch && Word(std::vector<uint8_t>(direct.begin(),direct.end()),0)==TL::Magic);
  assert(reads==7102 && original.Ready() && !steps && !packets);
  printf("PASS original fabric/3551 independently classified learned parts/7102 endpoint batches+word equivalence/synthetic learned words%u+recipes%u REMOVED/no supplied initial hidden/default mutable bytes unchanged/ZERO ROOT instructions;NOT model execution\n",removedLearned,removedRecipes);
 }
};
#endif
