/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY 1
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38PreparedQ4UploadPlan.h"
#include "../PioneerQ38PreparedQ4RootTables.h"
#include "../PioneerQwen38FleetPlan.h"
#include <set>
#ifdef NDEBUG
#error Upload planning checks must remain enabled
#endif
// Independent synthetic authenticated-cache endpoints. No file cache,GPU
// transport,model products or physical measurements are supplied here.
struct Weights {
 struct Entry{std::vector<unsigned> indices;unsigned logical=0,indexBytes=0;};
 const PioneerQwen38PackedPlan& plan;std::vector<Entry> entries;
 bool ready=true,badBank=false;unsigned dense=UINT32_MAX;
 explicit Weights(const PioneerQwen38PackedPlan& p):plan(p),entries(p.Parts().size()){
  for(unsigned f=0;f<entries.size();f++){
   const auto& part=plan.Parts()[f];if(part.kind || part.layer>=64 || part.role<2 || part.role>4)continue;
   auto& e=entries[f];const unsigned tiles=part.bytes/2304;e.indices.resize(tiles);e.indexBytes=(tiles*4+15)/16*16;e.logical=e.indexBytes;
   const bool full=part.layer==0 && part.role==4 && dense==UINT32_MAX;if(full)dense=f;
   for(unsigned t=0;t<tiles;t++){
    const unsigned kind=full || !(t%31)?0:t%7?1:2;
    e.indices[t]=(kind<<30)|(kind==2?0:e.logical);e.logical+=kind==2?0:kind?48:512;
   }
  }
 }
 bool AllPrepared()const{return ready && plan.Ready();}
 const PioneerQwen38PackedPlan& OriginalPlan()const{return plan;}
 bool Correction(unsigned f,PioneerQ38NativeQ4WeightCache::CorrectionProposal& p)const{
  if(!AllPrepared() || f>=entries.size() || !entries[f].logical)return false;
  p={f,plan.Parts()[f].bank+unsigned(badBank),0x70000000,entries[f].logical,(entries[f].logical+4095)/4096*4096};return true;
 }
 bool KeyFor(unsigned f,bool manifest,PioneerModelWeightCache::Key& key)const{
  if(manifest || !AllPrepared() || f>=entries.size() || !entries[f].logical)return false;
  key={};key.bytes=uint64_t(plan.Parts()[f].bytes)+entries[f].logical;return true;
 }
};
struct Pages {
 Weights& weights;bool ready=true,fail=false,revoke=false,badPadding=false;unsigned reads=0;
 explicit Pages(Weights& source):weights(source){}
 bool ReadyFor(const Weights& w)const{return ready && &w==&weights && w.AllPrepared();}
 bool ReadPage(unsigned f,unsigned page,bool side,uint8_t* output,unsigned capacity,std::string& error){
  if(!ReadyFor(weights) || fail || capacity<4096){error="synthetic source refusal";return false;}
  reads++;const auto& p=weights.plan.Parts()[f];const auto& e=weights.entries[f];const unsigned bytes=side?e.logical:p.bytes;
  if(uint64_t(page)*4096>=bytes)return false;
  memset(output,0,4096);
  for(unsigned i=0;i<4096 && uint64_t(page)*4096+i<bytes;i++){
   const unsigned at=page*4096+i;
   output[i]=side && at<e.indices.size()*4?uint8_t(e.indices[at/4]>>((at&3)*8)):side && at<e.indexBytes?0:0xa7;
  }
  if(badPadding && bytes%4096 && page==bytes/4096)output[4095]=1;
  if(revoke)ready=false;return true;
 }
};
using Plan=PioneerQ38PreparedQ4UploadPlan<Weights,Pages>;
struct WithdrawingPlan {
 using Job=Plan::Job;const Plan& source;mutable unsigned calls=0;unsigned withdrawAt=UINT32_MAX;mutable bool withdrawn=false;
 bool Ready()const{return !withdrawn && source.Ready();}
 const PioneerQwen38PackedPlan* OriginalPlan()const{
  if(++calls==withdrawAt){withdrawn=true;return nullptr;}return source.OriginalPlan();
 }
 bool Describe(unsigned f,unsigned first,unsigned groups,unsigned worker,unsigned request,Job& output,std::string& error)const{
  return Ready() && source.Describe(f,first,groups,worker,request,output,error);
 }
};
#ifndef PIONEER_Q38_PREPARED_UPLOAD_FIXTURE_ONLY
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));
 PioneerQwen38PackedPlan original;assert(original.Build(model,8192,true,true));Weights weights(original);Pages pages(weights);Plan plan;std::string error;
 assert(plan.Build(weights,pages,error) && plan.Ready() && !plan.Build(weights,pages,error) && plan.Primaries()==1344 && plan.Spans().size()==4895);
 unsigned originals=0,primaries=0,sides=0,embedding=0;uint64_t uploaded=0,next[7]={};
 for(unsigned bank=0;bank<7;bank++)next[bank]=original.Used(bank);
 for(const auto& s:plan.Spans()){
  const auto& p=original.Parts()[s.fragment];assert(s.bank==p.bank);uploaded+=s.allocated;
  if(s.kind==Plan::Correction){assert(s.address==original.Base+next[s.bank] && s.bytes==weights.entries[s.fragment].logical);next[s.bank]+=s.allocated;sides++;}
  else{assert(s.address==p.proposedAddress && s.bytes==p.bytes && s.allocated==p.allocated);
   if(s.kind==Plan::PreparedPrimary){assert(p.layer<64 && p.role>=2 && p.role<=4);primaries++;}
   else{originals++;if(p.layer==64 && p.role==0)embedding++;}
  }
 }
 assert(originals==2207 && primaries==1344 && sides==1344 && embedding==7 && uploaded==plan.Bytes());
 for(unsigned bank=0;bank<7;bank++)assert(next[bank]==plan.Used(bank) && next[bank]<=original.Capacity);
 const unsigned capturedReads=pages.reads;unsigned jobs=0,multichunk=0,refusals=0;
 std::set<unsigned> selected;
 for(const auto& p:original.Parts())if(!p.kind && p.layer<64 && p.role>=2 && p.role<=4)selected.insert(p.tensor);
 for(unsigned tensor:selected){
  const auto& t=model.Tensors()[tensor];
  PioneerQwen38FleetPlan fleet;assert(fleet.Build(model,original,tensor,119,16) && fleet.Jobs().size()==119);
  for(const auto& j:fleet.Jobs()){
   Plan::Job out;assert(plan.Describe(j.fragment,j.firstRow,j.groups,j.worker+1,17,out,error));
   const auto& p=original.Parts()[j.fragment];const auto& e=weights.entries[j.fragment];
   const unsigned blocks=unsigned(t.inputs/256),firstTile=(j.firstRow-p.firstRow)/16*blocks;
   assert(out.descriptor[6]==original.Base+j.proposedOffset && out.descriptor[10]==firstTile
    && out.descriptor[11]==p.bytes/2304 && out.descriptor[8]>=original.Base+original.Used(j.bank));
   unsigned reads=1,flits=1;
   for(unsigned g=0;g<j.groups;g++){
    const unsigned first=firstTile+g*blocks,indexBytes=((first%16+blocks)*4+63)/64*64;
    unsigned low=UINT32_MAX,high=0;
    for(unsigned b=0;b<blocks;b++){const auto v=e.indices[first+b];if(v>>30==2)continue;low=std::min(low,v&0x3fffffff);high=std::max(high,v&0x3fffffff);}
    unsigned payload=0;if(low!=UINT32_MAX)payload=(std::min(high+512,e.logical)+63)/64*64-low/64*64;
    reads+=1+(payload+16383)/16384;flits+=(indexBytes+payload)/64;multichunk+=payload>16384;
   }
   assert(reads==out.extraReads && flits==out.extraFlits);jobs++;
  }
 }
 assert(jobs==192*119 && multichunk && pages.reads==capturedReads);
 PioneerQ38PreparedQ4RootTables<Plan> tables;assert(tables.Build(plan,model,error) && tables.Ready());
 using R=PioneerQ38PreparedQ4RootLayout;unsigned bound=0;std::array<unsigned,R::Bytes/4> table{};
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  assert(tables.Read(layer,role,0,reinterpret_cast<uint8_t*>(table.data()),R::Bytes) && R::Header(table.data(),layer,role));
  for(unsigned w=0;w<119;w++){
   const auto* d=table.data()+16+w*16;const auto* ledger=table.data()+R::LedgerOffset/4+w*4;
   unsigned job[16]={};job[1]=12;job[2]=2304;job[5]=d[4];job[6]=d[6];job[7]=d[5];job[8]=d[7];job[11]=w+1;job[12]=73;job[14]=1;
   std::array<unsigned,16> output{};unsigned reads=0,flits=0;
   assert(R::Bind(table.data(),layer,role,d,ledger,job,ledger[2],ledger[3],output.data(),reads,flits)
    && reads==ledger[0] && flits==ledger[1] && output[2]==73);
   assert(R::Bind(table.data(),layer,role,output.data(),ledger,job,ledger[2],ledger[3],output.data(),reads,flits));
   job[12]++;const auto prior=output;
   assert(!R::Bind(table.data(),layer,role,output.data(),ledger,job,ledger[2],ledger[3],output.data(),reads,flits) && output==prior);
   bound++;
  }
  const auto* d=table.data()+16;const auto* ledger=table.data()+R::LedgerOffset/4;
  unsigned job[16]={};job[1]=12;job[2]=2304;job[5]=d[4];job[6]=d[6];job[7]=d[5];job[8]=d[7];job[11]=1;job[12]=73;job[14]=1;
  std::array<unsigned,16> output;output.fill(0x93);const auto prior=output;unsigned reads=0,flits=0;
  for(unsigned field=0;field<16;field++){
   auto bad=table;bad[field]^=0x80000001;
   assert(!R::Bind(bad.data(),layer,role,d,ledger,job,ledger[2],ledger[3],output.data(),reads,flits) && output==prior);refusals++;
   std::array<unsigned,16> descriptor;std::copy(d,d+16,descriptor.begin());descriptor[field]^=0x80000001;
   assert(!R::Bind(table.data(),layer,role,descriptor.data(),ledger,job,ledger[2],ledger[3],output.data(),reads,flits) && output==prior);refusals++;
  }
  for(unsigned field=0;field<4;field++){
   std::array<unsigned,4> bad;std::copy(ledger,ledger+4,bad.begin());bad[field]^=0x80000001;
   assert(!R::Bind(table.data(),layer,role,d,bad.data(),job,ledger[2],ledger[3],output.data(),reads,flits) && output==prior);refusals++;
  }
 }
 assert(bound==22848 && pages.reads==capturedReads);
 WithdrawingPlan early{plan};early.withdrawAt=1;PioneerQ38PreparedQ4RootTables<WithdrawingPlan> missing;
 assert(!missing.Build(early,model,error) && !missing.Ready());refusals++;
 WithdrawingPlan late{plan};PioneerQ38PreparedQ4RootTables<WithdrawingPlan> withdrawing;
 assert(withdrawing.Build(late,model,error));late.withdrawAt=late.calls+2;
 std::array<uint8_t,64> unmodified;unmodified.fill(0x93);const auto unmodifiedBefore=unmodified;
 assert(!withdrawing.Read(0,2,0,unmodified.data(),unmodified.size()) && unmodified==unmodifiedBefore && !withdrawing.Ready());refusals++;
 auto raw=[&](unsigned f,unsigned page,uint8_t* out,unsigned capacity){assert(capacity>=4096);const auto& p=original.Parts()[f];
  memset(out,0,4096);memset(out,0x35,std::min(4096U,p.bytes-page*4096));return true;};
 std::array<uint8_t,4096> output;unsigned sampled=0;
 for(unsigned s=0;s<plan.Spans().size();s++){
  const auto& span=plan.Spans()[s];if(span.kind!=Plan::Correction && original.Parts()[span.fragment].layer!=64)continue;
  assert(plan.ReadPage(s,span.allocated/4096-1,output.data(),output.size(),raw,error));sampled++;
  const unsigned logical=span.bytes%4096;if(logical)for(unsigned at=logical;at<4096;at++)assert(!output[at]);
  if(span.kind==Plan::Original)assert(output[0]==0x35); // embedding never reaches prepared page source
 }
 Plan::Job unchanged;memset(&unchanged,0x5b,sizeof(unchanged));const auto before=unchanged;
 for(unsigned f=0;f<original.Parts().size();f++)if(original.Parts()[f].layer==64 && original.Parts()[f].role==0){
  assert(!plan.Describe(f,0,1,1,1,unchanged,error) && !memcmp(&before,&unchanged,sizeof(before)));refusals++;
 }
 const auto& p=original.Parts()[weights.dense];
 for(unsigned s=0;s<plan.Spans().size();s++)if(plan.Spans()[s].kind==Plan::PreparedPrimary){
  assert(plan.ReadPage(s,0,output.data(),output.size(),raw,error) && output[0]==0xa7);break;
 }
 pages.badPadding=true;
 for(unsigned s=0;s<plan.Spans().size();s++)if(plan.Spans()[s].kind==Plan::Correction && plan.Spans()[s].bytes%4096){
  output.fill(0x93);const auto prior=output;
  assert(!plan.ReadPage(s,plan.Spans()[s].allocated/4096-1,output.data(),output.size(),raw,error) && output==prior);refusals++;break;
 }
 pages.badPadding=false;
 for(unsigned s=0;s<plan.Spans().size();s++)if(plan.Spans()[s].kind==Plan::Correction){
  const unsigned f=plan.Spans()[s].fragment;weights.entries[f].indices[0]^=16;
  output.fill(0x93);const auto prior=output;
  assert(!plan.ReadPage(s,0,output.data(),output.size(),raw,error) && output==prior);refusals++;
  weights.entries[f].indices[0]^=16;break;
 }
 pages.ready=false;assert(!plan.Describe(weights.dense,p.firstRow,1,1,1,unchanged,error) && !memcmp(&before,&unchanged,sizeof(before)));refusals++;pages.ready=true;
 pages.fail=true;Plan failed;assert(!failed.Build(weights,pages,error) && !failed.Ready());refusals++;pages.fail=false;
 pages.revoke=true;Plan revoked;assert(!revoked.Build(weights,pages,error) && !revoked.Ready());refusals++;pages.revoke=false;pages.ready=true;
 weights.badBank=true;Plan invalid;assert(!invalid.Build(weights,pages,error) && !invalid.Ready());refusals++;weights.badBank=false;
 weights.ready=false;output.fill(0x93);const auto prior=output;assert(!plan.ReadPage(0,0,output.data(),output.size(),raw,error) && output==prior && !plan.Ready() && !tables.Ready());refusals++;
 printf("PASS_PREPARED_Q4_UPLOAD_PLAN originalSpans%u preparedPrimaries%u correctionSpans%u preservedEmbeddingStripes%u full119Jobs%u ROOTBindings%u multiChunkGroups%u sampledTailPages%u refusals%u;SYNTHETIC_CACHE_NO_DEVICE_ROOT_MODEL_OR_TPS\n",originals,primaries,sides,embedding,jobs,bound,multichunk,sampled,refusals);
}
#endif
