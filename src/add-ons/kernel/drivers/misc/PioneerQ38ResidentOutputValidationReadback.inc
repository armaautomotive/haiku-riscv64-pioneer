/* SPDX-License-Identifier: MIT */
// INSIDE the SAME genuine combined owner,never a parallel read owner.
// This is fixed DEBUG transport;NO model arithmetic,Work mode or public
// address/route/count request. ALL120 physical holds precede first read;
// complete original snapshot restoration follows the final transfer.
 PioneerQ38ResidentOutputReadbackPlan readbackPlan;
 PioneerQ38ResidentOutputReadbackPage readbackPage={};
 bool readbackTraffic=false;
 bool ReadbackValue(unsigned word,uint32_t value)const{
  if(readbackPage.kind==PioneerQ38ResidentOutputReadbackPlan::Operands){
   for(unsigned half=0;half<2;half++){
    uint16_t operand=uint16_t(value>>(half*16));if((operand&0x7f80)!=0x4000 || operand==0xc000)return false;
   }return true;
  }
  if(readbackPage.kind==PioneerQ38ResidentOutputReadbackPlan::Scales)
   return !(value&0x80000000) && (!value || (value>=0x2f800000 && value<=0x47800000));
  if(readbackPage.kind==PioneerQ38ResidentOutputReadbackPlan::Lines){
   const unsigned at=readbackPage.firstWord+word-50176,index=at%32;
   if(at>=768)return false;
   uint32_t d[64],line[32]={};PioneerQ38ResidentOutputDescriptor(operation.Manifest(),operation.PostManifest(),at/32+1,d);
   const uint32_t fields[]={TT_Q38_RESIDENT_GDN_OUTPUT_MARKER,d[6],d[5],8,6144,d[42],13,d[40],d[41]};
   for(unsigned i=0;i<9;i++)line[8+i]=fields[i];
   for(unsigned i=0;i<8;i++)line[17+i]=d[32+i];
   line[25]=24;line[26]=d[7];line[27]=d[9];line[28]=d[16];line[29]=TT_Q38_GDN_RENDEZVOUS_MARKER;line[30]=d[45];line[31]=1;
   line[index]=value;return PioneerQ38ResidentGdnOutputLine(line,d);
  }
  // This owner is ONLY first-layer/position0 with genuinely zeroed slots.
  if(readbackPage.kind==PioneerQ38ResidentOutputReadbackPlan::CurrentState)return value==0;
  return (value&0x7fffffff)<0x7f800000;
 } // fixed scalar/receipt grammar ONLY,NOT a CPU model reference or Work mode
 template<class IO> bool ReadbackTransferGuard(IO& io){
  if(!readbackTraffic || !ran || !rootVerified || traffic || !ready || closed || readbackPage.words==0
   || readbackPage.words>1024 || readbackPage.bank>=7 || (readbackPage.address&4095)
   || readbackPage.kind>PioneerQ38ResidentOutputReadbackPlan::NextState
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
  return true; // NEVER read a held-core PC as a live-terminal proof
 }
 template<class IO> bool ValidationReadback(IO& io,uint32_t* values,PioneerQ38ResidentOutputReadbackResult& out,
  PioneerQ38ResidentOutputResult& r){
  if(!values || !out.started || out.pages || out.words || out.completed
   || readbackPlan.PageCount()!=PioneerQ38ResidentOutputReadbackPlan::Pages || !ReadbackHeld(io))return false;
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
    uint32_t value;if(!io.ResidentOutputMemoryReadChecked(0,0x80000+word*4,value) || !ReadbackValue(word,value))return false;
    values[out.words++]=value;
   }
   out.pages++;if(!Guard(io,0))return false;
  }
  if(out.pages!=PioneerQ38ResidentOutputReadbackPlan::Pages || out.words!=PioneerQ38ResidentOutputReadbackPlan::Words
   || !ReadbackHeld(io))return false;
  out.completed=1;return true;
 }
