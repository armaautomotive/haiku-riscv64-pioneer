/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK) || PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK != 1
#error Private fixed readback belongs only inside the selected validation owner
#endif
// Members of the SAME owner;HEAP ONLY,never a20KiB kernel stack local.
 PioneerQ38MixedLiveReadbackPlan readbackPlan;
 PioneerQ38MixedLiveReadbackPage readbackPage={};
 uint32_t readbackValues[TT_Q38_MIXED_LIVE_READBACK_WORDS]={};
 unsigned readbackIndex=0;
 bool readbackTraffic=false;
 bool ReadbackHeldState()const{
  if(!ran || !allStopped || traffic || closed || unsafe)return false;
  for(unsigned w=0;w<120;w++)if(running[w] || resets[w]!=0x47800)return false;
  return true;
 }
 template<class IO> bool ReadbackTransferGuard(IO& io){
  PioneerQ38MixedLiveReadbackPage p;
  if(!readbackTraffic || !ReadbackHeldState() || !readbackPlan.Page(readbackIndex,p)
   || p.bank!=readbackPage.bank || p.address!=readbackPage.address || p.route!=readbackPage.route
   || p.firstWord!=readbackPage.firstWord || p.words!=1024)return false;
  const uint32_t expected[]={p.address,0,p.route,PioneerQ38MixedLiveReadbackPlan::Scratch,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)if(command[0][i]!=expected[i] || io.ResidentNocRead(i*4)!=expected[i])return false;
  if(io.ResidentNocRead(0x40)>1)return false;
  for(unsigned i=1;i<4;i++)if(io.ResidentNocRead(0x40+i*0x800))return false;
  const unsigned delta[]={1,64,1,1,0,0,1,0};
  for(unsigned i=0;i<8;i++)if(uint32_t(io.ResidentNocRead(PioneerQ38PanelCounter(i))-counts[0][i])>delta[i])return false;
  return true; // ONLY this owned READ packet;no general busy/counter bypass
 }
 template<class IO> bool ReadbackHeld(IO& io){
  if(readbackTraffic || !ReadbackHeldState())return false;
  for(unsigned w=0;w<120;w++)if(!Guard(io,w) || !Image(io,w))return false;
  return true; // genuine unheld STOPs preceded ALL holds;never sample held PCs
 }
 static bool ReadbackOverlap(const void* a,unsigned bytes,const void* b,size_t size){
  const uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
  return !a || !b || x>UINTPTR_MAX-bytes || y>UINTPTR_MAX-size || (x<y+size && y<x+bytes);
 }
 bool ReadbackRequest(uint32_t* values,unsigned words,const PioneerQ38MixedLiveReadbackResult& out,
  const PioneerQ38MixedLiveResult& r,const PioneerQ38MixedLiveMemoryResult& saved)const{
  if(!values || words!=TT_Q38_MIXED_LIVE_READBACK_WORDS || (reinterpret_cast<uintptr_t>(values)&3)
   || reinterpret_cast<uintptr_t>(values)>UINTPTR_MAX-words*4 || out.version!=1 || out.started || out.pages || out.words || out.completed
   || !ran || unsafe || closed || !r.prepared || r.version!=1 || r.unsafe || r.phase!=5 || r.completed!=119
   || r.stopped || r.held || r.closed || saved.restoredWords || !memory || !canonical || !plan)return false;
  // Trusted kernel allocation only;no direct user pointer. Reject aliases
  // of every retained object/journal and both mutable result structures.
  const void* objects[]={this,&out,&r,&saved,memory,canonical,plan,proof,banks,cohort,ledger};
  const size_t sizes[]={sizeof(*this),sizeof(out),sizeof(r),sizeof(saved),sizeof(*memory),sizeof(*canonical),sizeof(*plan),
   sizeof(*proof),sizeof(*banks),sizeof(*cohort),PioneerResidentBanks::Pages};
  for(unsigned i=0;i<sizeof(objects)/sizeof(objects[0]);i++)if(ReadbackOverlap(values,words*4,objects[i],sizes[i])
   || (ReadbackOverlap(&out,sizeof(out),objects[i],sizes[i]) && i!=1))return false;
  return true;
 }
 template<class IO> bool ValidationReadback(IO& io,PioneerQ38MixedLiveReadbackResult& out,PioneerQ38MixedLiveResult& r){
  if(!out.started || out.pages || out.words || out.completed || readbackPlan.PageCount()!=5 || !ReadbackHeld(io))return false;
  for(readbackIndex=0;readbackIndex<5;readbackIndex++){
   r.worker=0;r.word=readbackIndex;
   if(!readbackPlan.Page(readbackIndex,readbackPage) || readbackPage.firstWord!=out.words || !Guard(io,0))return false;
   for(unsigned word=0;word<1024;word++){
    const unsigned a=PioneerQ38MixedLiveReadbackPlan::Scratch+word*4;const uint32_t poison=0x7fc80000+readbackIndex*1024+word;uint32_t actual;
    if(!Guard(io,0) || !io.MixedLiveMemoryWrite(0,a,poison) || !Guard(io,0)
     || !io.MixedLiveMemoryReadChecked(0,a,actual) || actual!=poison)return false;
   }
   const uint32_t next[]={readbackPage.address,0,readbackPage.route,PioneerQ38MixedLiveReadbackPlan::Scratch,0,129,0,0,4096,0};
   for(unsigned i=0;i<10;i++){
    if(!Scope(io))return false;
    io.ResidentNocWrite(i*4,next[i]);
    if(!Scope(io) || io.ResidentNocRead(i*4)!=next[i])return false;
    command[0][i]=next[i];
   }
   if(!Guard(io,0))return false;
   const uint64_t at=io.Q38CohortNow();if(at>UINT64_MAX-UINT64_C(30000000))return false;
   uint32_t expected[8];const unsigned delta[]={1,64,1,1,0,0,1,0};
   for(unsigned i=0;i<8;i++)expected[i]=counts[0][i]+delta[i];
   readbackTraffic=true;io.ResidentNocWrite(0x40,1);bool done=false;
   for(unsigned poll=0;poll<65536;poll++){
    if(!Guard(io,0) || !Budget(io,at))return false;
    bool exact=!io.ResidentNocRead(0x40);
    for(unsigned i=0;i<8;i++)exact &= io.ResidentNocRead(PioneerQ38PanelCounter(i))==expected[i];
    if(exact){done=true;break;}io.Pause();
   }
   if(!done || !Guard(io,0) || !PioneerQ38GraphCsrIdle(io))return false;
   for(unsigned i=0;i<8;i++)counts[0][i]=expected[i];
   readbackTraffic=false;if(!Guard(io,0))return false;
   for(unsigned word=0;word<1024;word++){
    uint32_t value;if(!Guard(io,0) || !io.MixedLiveMemoryReadChecked(0,PioneerQ38MixedLiveReadbackPlan::Scratch+word*4,value))return false;
    const uint32_t a=value&0x7fffffff;
    if(a>0x477fe000 || (a && a<0x26800000))return false; // ROOT's exact RMS domain;poison/NaN/Inf reject immediately
    readbackValues[out.words++]=value;
   }
   out.pages++;
  }
  return out.pages==5 && out.words==5120 && ReadbackHeld(io);
 }
