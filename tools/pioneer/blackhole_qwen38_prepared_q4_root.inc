/* SPDX-License-Identifier: MIT */
// Included inside Projection in the distinct WHOLE prepared ROOT build.
// Shared ODR bodies call only passed compiled private seals,not TU-local
// owner casts. New functions need a separately journaled .prepared code view.
using PreparedRoot=PioneerQ38PreparedQ4RootLayout;
using PreparedSeal=void(*)(const uint32_t*);
inline volatile uint32_t* PreparedWords(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
inline void PreparedFence(){asm volatile("fence":::"memory");}
inline unsigned PreparedCounter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
extern "C" inline __attribute__((noinline,noclone)) void q38_prepared_q4_read_shared(
 const uint32_t* d,uint32_t* tracked,unsigned remote,unsigned bytes,PreparedSeal seal){
 namespace R=PioneerQ38MixedProjectionRootLayout;
 if(!bytes || bytes>16384 || ((remote|bytes)&63) || remote<d[4] || remote>d[4]+67108864-bytes)scalar_fault_park();
 seal(d);uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*PreparedWords(0xffb20000+PreparedCounter(i))!=tracked[i])scalar_fault_park();
  expected[i]=tracked[i]+(i==1?bytes/64:i==0 || i==2 || i==3 || i==6?1U:0U);
 }
 const unsigned command[]={remote,0,d[8],R::Scratch,0,129,0,0,bytes,0};
 for(unsigned i=0;i<10;i++)PreparedWords(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(PreparedWords(0xffb20000)[i]!=command[i])scalar_fault_park();
 PreparedFence();*PreparedWords(0xffb20040)=1;(void)*PreparedWords(0xffb20040);PreparedFence();bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  done=!*PreparedWords(0xffb20040);
  for(unsigned i=0;i<8;i++)done &= *PreparedWords(0xffb20000+PreparedCounter(i))==expected[i];
  if(done)break;
 }
 if(!done)scalar_fault_park();
 seal(d);for(unsigned i=0;i<10;i++)if(PreparedWords(0xffb20000)[i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
}
extern "C" inline __attribute__((noinline,noclone)) void q38_prepared_q4_stage_shared(
 const uint32_t* d,uint32_t* tracked,PreparedSeal seal){
 namespace R=PioneerQ38MixedProjectionRootLayout;
 if(d[15]!=12 || d[42]>=64 || !PreparedRoot::PhaseOffset(d[43]))scalar_fault_park();
 const unsigned remote=d[4]+0x1000000+d[42]*0x80000+PreparedRoot::PhaseOffset(d[43]);
 q38_prepared_q4_read_shared(d,tracked,remote,64,seal);
 for(unsigned i=0;i<16;i++)PreparedWords(PreparedRoot::RootHeader)[i]=PreparedWords(R::Scratch)[i];
 PreparedFence();
 if(!PreparedRoot::Header(PreparedWords(PreparedRoot::RootHeader),d[42],d[43]))scalar_fault_park();
 q38_prepared_q4_read_shared(d,tracked,remote+64,PreparedRoot::RootTableBytes,seal);
 for(unsigned i=0;i<PreparedRoot::RootTableBytes/4;i++)PreparedWords(PreparedRoot::RootTable)[i]=PreparedWords(R::Scratch)[i];
 // One new phase capture,from the immutable package source. Never adopt a
 // changed template/ledger between worker packets or during gathering.
 for(unsigned i=0;i<PreparedRoot::Bytes/4;i++)PreparedWords(PreparedRoot::RootShadow)[i]=PreparedWords(PreparedRoot::RootTable)[i];
 PreparedFence();seal(d);
}
extern "C" inline __attribute__((noinline,noclone)) void q38_prepared_q4_bind_shared(
 const uint32_t* d,unsigned worker,unsigned sequence,unsigned& reads,unsigned& flits){
 namespace R=PioneerQ38MixedProjectionRootLayout;
 if(worker>=119 || d[15]!=12)scalar_fault_park();
 const auto* part=PreparedWords(R::PartsShadow+worker*32);
 uint32_t job[16];for(unsigned i=0;i<10;i++)job[i]=PreparedWords(R::Scratch)[16+i];
 job[10]=0;job[11]=worker+1;job[12]=sequence;job[13]=0;job[14]=1;job[15]=0;
 const unsigned slot=PreparedRoot::RootTable+worker*64;
 const unsigned shadow=PreparedRoot::RootShadow+worker*64;
 for(unsigned i=0;i<16;i++){
  if(PreparedWords(PreparedRoot::RootHeader)[i]!=PreparedWords(PreparedRoot::ShadowHeader)[i])scalar_fault_park();
  if(i==2){if(PreparedWords(shadow)[i])scalar_fault_park();}
  else if(PreparedWords(slot)[i]!=PreparedWords(shadow)[i])scalar_fault_park();
 }
 for(unsigned i=0;i<4;i++)if(PreparedWords(PreparedRoot::RootLedger+worker*16)[i]!=PreparedWords(PreparedRoot::ShadowLedger+worker*16)[i])scalar_fault_park();
 if(!PreparedRoot::Bind(PreparedWords(PreparedRoot::RootHeader),d[42],d[43],PreparedWords(slot),
  PreparedWords(PreparedRoot::RootLedger+worker*16),job,PreparedWords(R::RowsShadow)[worker],part[1],
  PreparedWords(slot),reads,flits))scalar_fault_park();
 PreparedFence();
}
