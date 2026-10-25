/* SPDX-License-Identifier: MIT */
// INSIDE the SAME genuine complete-layer owner,not an old prefix observer.
// Fixed QLV1 DEBUG transport ONLY. No public address/route/count, model
// arithmetic, CPU oracle, current-slot publication or Work-mode path.
// FFN has overwritten old operands/scales/raw-RMS backup: observe only
// the NEW final hidden vector and preserved Conv/GDN/history/projection.
 PioneerQ38ResidentLayerReadbackPlan readbackPlan;
 PioneerQ38ResidentLayerReadbackPage readbackPage={};
 bool readbackTraffic=false;
 bool ReadbackValue(uint32_t value)const{
  // Only the first recurrent layer/position0 is admitted. Its CURRENT
  // slot was genuinely zeroed and remains unmodified by this execution.
  if(readbackPage.kind==PioneerQ38ResidentLayerReadbackPlan::CurrentState)return value==0;
  return (value&0x7fffffff)<0x7f800000;
 } // scalar grammar ONLY;independent original-model comparison is separate
 template<class IO> bool ReadbackTransferGuard(IO& io){
  if(!readbackTraffic || !ran || !rootVerified || traffic || !ready || closed || readbackPage.words!=1024
   || readbackPage.bank>=7 || (readbackPage.address&4095)
   || readbackPage.kind>PioneerQ38ResidentLayerReadbackPlan::NextState
   || readbackPage.route!=PioneerQ38ProjectionRoute(readbackPage.bank))return false;
  for(unsigned w=0;w<120;w++)if(running[w] || resets[w]!=0x47800 || !accepted[w] || (w && !stopped[w-1]))return false;
  const uint32_t expected[]={readbackPage.address,0,readbackPage.route,0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)if(command[0][i]!=expected[i] || io.ResidentNocRead(i*4)!=expected[i])return false;
  if(io.ResidentNocRead(0x40)>1)return false;
  for(unsigned i=1;i<4;i++)if(io.ResidentNocRead(0x40+i*0x800))return false;
  const unsigned delta[]={1,64,1,1,0,0,1,0};
  for(unsigned i=0;i<8;i++)if(uint32_t(io.ResidentNocRead(PioneerQ38PanelCounter(i))-counts[0][i])>delta[i])return false;
  return true;
 }
 template<class IO> bool ReadbackHeld(IO& io){
  if(!Completed() || readbackTraffic || !rootVerified || traffic)return false;
  for(unsigned w=0;w<120;w++)if(running[w] || resets[w]!=0x47800 || !accepted[w] || (w && !stopped[w-1])
   || !Guard(io,w) || !Image(io,w))return false;
  return true; // NO held-PC read;ALL119 fresh unheld STOPs are already owed
 }
 template<class IO> bool ValidationReadback(IO& io,uint32_t* values,PioneerQ38ResidentLayerReadbackResult& out,
  PioneerQ38ResidentLayerResult& r){
  if(!values || !out.started || out.pages || out.words || out.completed
   || readbackPlan.PageCount()!=PioneerQ38ResidentLayerReadbackPlan::Pages || !ReadbackHeld(io))return false;
  for(unsigned page=0;page<readbackPlan.PageCount();page++){
   r.worker=0;r.word=page;
   if(!readbackPlan.Page(page,readbackPage) || readbackPage.firstWord!=out.words || !Guard(io,0))return false;
   for(unsigned word=0;word<1024;word++){
    uint32_t poison=0x7fc70000+page*1024+word;if(!Stage(io,0,0x80000+word*4,&poison,1))return false;
   }
   const uint32_t next[]={readbackPage.address,0,readbackPage.route,0x80000,0,129,0,0,4096,0};
   for(unsigned i=0;i<10;i++){
    io.ResidentNocWrite(i*4,next[i]);if(io.ResidentNocRead(i*4)!=next[i])return false;command[0][i]=next[i];
   }
   if(!Guard(io,0))return false;
   uint64_t start=io.Q38CohortNow();if(start>UINT64_MAX-UINT64_C(30000000))return false;
   uint32_t expected[8];const unsigned delta[]={1,64,1,1,0,0,1,0};
   for(unsigned i=0;i<8;i++)expected[i]=counts[0][i]+delta[i];
   readbackTraffic=true;io.ResidentNocWrite(0x40,1);bool done=false;
   for(unsigned poll=0;poll<65536;poll++){
    if(!Guard(io,0) || !Budget(io,start))return false;
    bool exact=!io.ResidentNocRead(0x40);
    for(unsigned i=0;i<8;i++)exact &= io.ResidentNocRead(PioneerQ38PanelCounter(i))==expected[i];
    if(exact){done=true;break;}io.Pause();
   }
   if(!done || !PioneerQ38GraphCsrIdle(io))return false;
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=next[i])return false;
   for(unsigned i=0;i<8;i++)counts[0][i]=expected[i];
   readbackTraffic=false;if(!Guard(io,0))return false;
   for(unsigned word=0;word<readbackPage.words;word++){
    uint32_t value;if(!io.ResidentLayerMemoryReadChecked(0,0x80000+word*4,value) || !ReadbackValue(value))return false;
    values[out.words++]=value;
   }
   out.pages++;if(!Guard(io,0))return false;
  }
  if(out.pages!=PioneerQ38ResidentLayerReadbackPlan::Pages || out.words!=PioneerQ38ResidentLayerReadbackPlan::Words
   || !ReadbackHeld(io))return false;
  out.completed=1;return true;
 }
