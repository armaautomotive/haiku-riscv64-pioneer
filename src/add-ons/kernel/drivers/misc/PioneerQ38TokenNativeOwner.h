/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_NATIVE_OWNER_H
#define PIONEER_Q38_TOKEN_NATIVE_OWNER_H
#include "PioneerQ38TokenMemory.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenRuntimePlan.h"
// DISTINCT full-token owner. Current operation stages HELD runtime only.
// No CPU products,old-prefix ownership,reset/bootstrap/RUN/STOP/END grant.
struct PioneerQ38TokenRuntimeResult {
 uint32_t version=1,started=0,phase=0,worker=0,chunk=0,word=0,unsafe=0,staged=0;
 uint32_t packets=0,writes=0,reads=0,bytesWritten=0,bytesRead=0,boots=0,reserved[2]={};
};
class PioneerQ38TokenNativeOwner {
 const tt_dram_samples_result* proof=nullptr;PioneerResidentBanks* banks=nullptr;PioneerQ38DispatchLease* cohort=nullptr;
 PioneerQ38TokenMemory* memory=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38TokenStartupPlan* startup=nullptr;const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
 PioneerQ38TokenRuntimePlan plan;uint32_t staging[4096]={};
 bool started=false,unsafe=false,staged=false,traffic=false;
 bool Bound()const{
  return started && !unsafe && proof && banks && cohort && memory && canonical && startup && ledger
   && memory->Installed() && memory->MatchesNativeOwner(*proof,*banks,*cohort,*canonical,*startup,identity,ledger,PioneerResidentBanks::Pages)
   && plan.MatchesBinding(*canonical,*startup,identity,&cohort->authority.source[0][0]);
 }
 template<class IO> bool Scope(IO& io)const{
  return Bound() && io.TokenRuntimeScope(*this,*memory,*canonical,*startup,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool Fail(IO& io,PioneerQ38TokenRuntimeResult& r){
  unsafe=true;staged=false;r.unsafe=1;r.staged=0;if(memory)memory->Fault();
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.TokenMemoryRetainHostOwners();return false; // MEMORY ONLY,no following IO/cleanup/retry
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io))return false;
  if(!traffic)return memory->Guard(io,w);
  // ONLY this owner's fixed ROOT packet may be in flight. All host/BME,
  // reset,source,mapping and native lifetime checks still hold. The caller
  // independently bounds EVERY actual counter delta before adoption.
  if(w || !io.SelectRuntimeCore(0) || !io.TokenMemoryMappingStable(0) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800)return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=cohort->authority.source[0][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  return true;
 }
 template<class IO> bool Local(IO& io,unsigned worker,uint32_t address,const uint32_t* words,unsigned count,PioneerQ38TokenRuntimeResult& r){
  if(!Guard(io,worker))return false;
  for(unsigned i=0;i<count;i++){r.word=i;if(!io.TokenMemoryWrite(worker,address+i*4,words[i]))return false;}
  for(unsigned i=0;i<count;i++){uint32_t actual;r.word=i;
   if(!io.TokenMemoryReadChecked(worker,address+i*4,actual) || actual!=words[i])return false;}
  return Guard(io,worker);
 }
 template<class IO> bool Packet(IO& io,const PioneerQ38TokenRuntimePlan::Chunk& d,bool write,PioneerQ38TokenRuntimeResult& r){
  namespace C=PioneerQ38GlobalChainLayout;const auto scratch=uint32_t(C::R::Scratch);
  if(d.bank!=plan.Bank() || !d.bytes || (d.address|d.bytes)&63 || d.bytes>16384 || d.address<plan.Workspace()
   || d.address>plan.Workspace()+C::WorkspaceBytes-d.bytes || !Guard(io,0))return false;
  const uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
  uint32_t before[8],expected[8];for(unsigned i=0;i<8;i++){
   before[i]=memory->counters[0][i];expected[i]=before[i]+(i==(write?5U:1U)?d.bytes/64:
    i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U);
  }
  const unsigned route=PioneerQ38ProjectionRoute(d.bank);
  const uint32_t command[]={write?scratch:d.address,0,write?129U:route,write?d.address:scratch,0,write?route:129U,0,write?0x212U:0,d.bytes,0};
  for(unsigned i=0;i<10;i++){
   memory->command[0][i]=command[i];io.ResidentNocWrite(i*4,command[i]);if(io.ResidentNocRead(i*4)!=command[i])return false;
  }
  traffic=true;io.ResidentNocWrite(0x40,1);r.packets++;bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!Guard(io,0))return false;
   const uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   uint32_t busy=io.ResidentNocRead(0x40);if(busy>1)return false;done=!busy;
   for(unsigned i=0;i<8;i++){
    const uint32_t observed=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(uint32_t(observed-before[i])>uint32_t(expected[i]-before[i]))return false;
    done &= observed==expected[i];
   }
   if(done)break;
   io.Pause();
  }
  if(!done)return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return false;
  for(unsigned i=0;i<8;i++)memory->counters[0][i]=expected[i]; // ONLY complete exact receipts,NEVER arbitrary recapture
  traffic=false;if(!Guard(io,0))return false;
  if(write){r.writes++;r.bytesWritten+=d.bytes;}else{r.reads++;r.bytesRead+=d.bytes;}
  return true;
 }
public:
 PioneerQ38TokenNativeOwner()=default;
 PioneerQ38TokenNativeOwner(const PioneerQ38TokenNativeOwner&)=delete;
 PioneerQ38TokenNativeOwner& operator=(const PioneerQ38TokenNativeOwner&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Staged()const{return staged && !unsafe && !traffic;}
 bool Pending()const{return traffic;}
 void Fault(){unsafe=true;staged=false;}
 bool MatchesRuntimeBinding(const tt_dram_samples_result& dram,const PioneerResidentBanks& b,const PioneerQ38DispatchLease& participants,
  const PioneerQ38TokenMemory& m,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& data,
  const PioneerQ38GdnIdentity& id,const uint8_t* initialized,unsigned pages)const{
  return proof==&dram && banks==&b && cohort==&participants && memory==&m && canonical==&c && startup==&data && identity==id
   && ledger==initialized && pages==PioneerResidentBanks::Pages && Bound();
 } // necessary same held binding ONLY,not a completed model or native lifetime permission
 template<class IO> bool PrepareHeldRuntime(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& b,PioneerQ38DispatchLease& participants,
  PioneerQ38TokenMemory& mem,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& data,
  const PioneerQ38GdnIdentity& id,const uint8_t* initialized,unsigned pages,PioneerQ38TokenRuntimeResult& r){
  const PioneerQ38TokenRuntimeResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);const auto* p=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=p[i])return false;
  if(started || unsafe || staged || traffic || !mem.Installed() || !mem.MatchesNativeOwner(dram,b,participants,c,data,id,initialized,pages)
   || !data.LearnedPagesPresent(initialized,pages) || !plan.Bind(c,data,id,&participants.authority.source[0][0]))return false;
  started=true;proof=&dram;banks=&b;cohort=&participants;memory=&mem;canonical=&c;startup=&data;identity=id;ledger=initialized;r.started=1;r.phase=1;
  for(unsigned w=0;w<120;w++){r.worker=w;PioneerQ38TokenMemoryResult image;
   if(!Guard(io,w) || !memory->VerifyImage(io,w,image))return Fail(io,r);
  }
  namespace C=PioneerQ38GlobalChainLayout;namespace W=PioneerQ38GlobalWorkerLayout;namespace T=PioneerQ38TokenLayout;
  r.phase=2;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!data.ReadBoot(w,staging) || !Local(io,w,w?unsigned(W::Boot):unsigned(C::R::Boot),staging,64,r))return Fail(io,r);
   for(unsigned i=0;i<128;i++)staging[i]=0;
   if(!Local(io,w,w?unsigned(W::Job):unsigned(C::R::Outer),staging,w?64U:128U,r)
    || !Local(io,w,w?unsigned(W::Doorbell):unsigned(C::R::Doorbell),staging,1,r)
    || !Local(io,w,w?unsigned(W::Status):unsigned(C::R::Status),staging,16,r))return Fail(io,r);
   r.boots++;
  }
  r.worker=0;
  if(!Local(io,0,T::CancelLocal,staging,16,r) || !Local(io,0,T::RecordLocal,staging,64,r))return Fail(io,r);
  r.phase=3;
  for(unsigned n=0;n<PioneerQ38TokenRuntimePlan::Chunks;n++){
   r.chunk=n;PioneerQ38TokenRuntimePlan::Chunk d;auto* raw=reinterpret_cast<uint8_t*>(staging);
   if(!plan.Describe(n,d) || !plan.Read(n,raw,d.bytes))return Fail(io,r);
   for(unsigned i=0;i<d.bytes/4;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
   if(!Local(io,0,C::R::Scratch,staging,d.bytes/4,r) || !Packet(io,d,true,r) || !Packet(io,d,false,r))return Fail(io,r);
   for(unsigned i=0;i<d.bytes/4;i++){uint32_t actual;r.word=i;
    if(!io.TokenMemoryReadChecked(0,C::R::Scratch+i*4,actual) || actual!=staging[i])return Fail(io,r);
   }
  }
  if(r.packets!=4616 || r.writes!=2308 || r.reads!=2308 || r.bytesWritten!=PioneerQ38TokenRuntimePlan::PayloadBytes
   || r.bytesRead!=r.bytesWritten || !data.ReadPublishedPackage(0,0,reinterpret_cast<uint8_t*>(staging),512))return Fail(io,r);
  {auto* raw=reinterpret_cast<uint8_t*>(staging);
   for(unsigned i=0;i<128;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;}
  if(!Local(io,0,C::R::Outer,staging,128,r))return Fail(io,r);
  staging[0]=1;
  if(!Local(io,0,C::R::Doorbell,staging,1,r))return Fail(io,r);
  r.phase=4;
  for(unsigned w=0;w<120;w++){r.worker=w;PioneerQ38TokenMemoryResult image;
   if(!Guard(io,w) || !memory->VerifyImage(io,w,image))return Fail(io,r);
  }
  staged=true;r.staged=1;return true;
 } // empty KV prefix/initial session DATA ONLY;NO boot,model arithmetic,token commit or clean-END capability
};
static_assert(sizeof(PioneerQ38TokenRuntimeResult)==64,"all runtime result bytes explicit");
static_assert(sizeof(PioneerQ38TokenNativeOwner)>16384 && sizeof(PioneerQ38TokenNativeOwner)<32768,"native owner staging buffer HEAP ONLY");
#endif
