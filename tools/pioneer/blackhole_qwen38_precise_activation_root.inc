/* SPDX-License-Identifier: MIT */
// Included INSIDE Projection, after canonical recipe capture. Actual
// on-card producer/consumer path, not a caller-supplied completed flag.
#include "PioneerQ38PreciseActivationRootRole.h"
namespace Precise=PioneerQ38PreciseWorkerLayout;
#define Q38_PRECISE_ROOT_FN __attribute__((noinline,section(".conv.precise")))
static bool PreciseDown(const uint32_t* d){return d[43]==4 && d[14]==17408 && d[15]==12;}
static Q38_PRECISE_ROOT_FN void PreciseDescriptor(const uint32_t* d,unsigned w,uint32_t* out){
 if(w>=68 || !PreciseDown(d))scalar_fault_park();
 for(unsigned i=0;i<64;i++)out[i]=0;
 out[0]=Precise::ActivationMagic;out[1]=1;out[2]=67108864;out[3]=d[3];out[4]=d[4];
 out[5]=w;out[6]=d[6];out[7]=w+1;out[8]=d[8];out[9]=M::Peer(w+1);
 out[10]=Precise::GateOffset+w*1024;out[11]=Precise::UpOffset+w*1024;
 out[12]=Precise::NativeOffset+w*512;out[13]=Precise::LinesOffset+w*128;out[14]=12;out[15]=68;
 for(unsigned i=0;i<16;i++)out[16+i]=Boot(w)[16+i];
 for(unsigned i=0;i<8;i++)out[32+i]=d[32+i];
 out[40]=d[40];out[41]=d[41];out[42]=d[42];out[43]=4;
 if(!Precise::DescriptorShape(out))scalar_fault_park();
}
static Q38_PRECISE_ROOT_FN void PreciseJob(const uint32_t* d,unsigned w,unsigned n,uint32_t* out){
 for(unsigned i=0;i<64;i++)out[i]=0;
 const unsigned h[]={M::Magic,n,3,w+1,M::Peer(w+1),d[42],d[40],d[6]};
 for(unsigned i=0;i<8;i++){out[i]=h[i];out[8+i]=d[32+i];}
 out[16]=Precise::ActivationMagic;out[17]=256;out[18]=576;out[19]=d[6];out[20]=12;out[27]=d[41];
 if(!n || !M::JobShape(out,Boot(w),n-1,0))scalar_fault_park();
}
static Q38_PRECISE_ROOT_FN void PreciseWait(const uint32_t* d,uint32_t* tracked,unsigned w){
 const unsigned n=Words(L::Nonces)[w];
 for(unsigned poll=0;poll<65536;poll++){
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  Transfer(d,tracked,w,M::Status,64,false);uint32_t s[16];for(unsigned i=0;i<16;i++)s[i]=Words(L::Scratch)[i];
  if(s[3]!=M::Magic || s[2] || s[0]<n-1 || s[0]>n || s[1]<n-1 || s[1]>s[0]
   || (s[4] && s[4]!=3) || s[5] || s[6]!=d[42] || s[7]!=d[40])scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  unsigned pc=Register(d,tracked,w,0xffb13140);
  if(s[0]!=n || s[1]!=n || s[4])continue;
  if(pc==0x2004c || pc==0x20050)return;
 }
 scalar_fault_park(); // no later consumer/launch/reset/retry
}
static Q38_PRECISE_ROOT_FN void PreciseBankRead(const uint32_t* d,uint32_t* tracked,unsigned offset,unsigned bytes){
 const bool native=offset>=unsigned(Precise::NativeOffset) && offset<=Precise::NativeOffset+34816
  && bytes<=Precise::NativeOffset+34816-offset;
 const bool lines=offset>=unsigned(Precise::LinesOffset) && offset<=Precise::LinesOffset+68*128
  && bytes<=Precise::LinesOffset+68*128-offset;
 if((!native && !lines) || !bytes || bytes>16384 || ((offset|bytes)&63))scalar_fault_park();
 q38_mixed_root_packet_seal(d);if(!Quiet())scalar_fault_park();uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  expected[i]=tracked[i]+(i==1?bytes/64:i==0 || i==2 || i==3 || i==6?1U:0U);
  if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
 const unsigned command[]={d[4]+offset,0,d[8],L::Scratch,0,129,0,0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];if(done)break;
 }
 if(!done)scalar_fault_park();
 q38_mixed_root_packet_seal(d);if(!Quiet())scalar_fault_park();
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
}
static Q38_PRECISE_ROOT_FN void PreciseGather(const uint32_t* d,uint32_t* tracked,unsigned w){
 uint32_t desc[64],job[16],proof[64],before[8],after[8];
 PreciseDescriptor(d,w,desc);PreciseWait(d,tracked,w);BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
 Transfer(d,tracked,w,Precise::Descriptor,256,false);
 for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=desc[i])scalar_fault_park();
 Transfer(d,tracked,w,0x74000,64,false);for(unsigned i=0;i<16;i++)job[i]=Words(L::Scratch)[i];
 Transfer(d,tracked,w,0x83f00,256,false);for(unsigned i=0;i<64;i++)proof[i]=Words(L::Scratch)[i];
 for(unsigned i=0;i<8;i++){
  before[i]=Words(L::Counters)[w*8+i];after[i]=Register(d,tracked,w,0xffb20000+Counter(i));
 }
 if(!Precise::ActivationProof(proof,job,desc,Boot(w)+16,w+1,M::Peer(w+1),before,after))scalar_fault_park();
 // The inner capsule must name the SAME actual external job/nonce, not
 // merely any shape-valid precise producer from this layer or owner.
 const auto* outer=Words(L::PeerJobs+w*256);
 for(unsigned i=0;i<10;i++)if(job[i]!=outer[16+i])scalar_fault_park();
 if(job[12]!=outer[1])scalar_fault_park();
 PreciseBankRead(d,tracked,Precise::LinesOffset+w*128,128);
 uint32_t line[32];for(unsigned i=0;i<32;i++)line[i]=Words(L::Scratch)[i];
 if(!Precise::ScaleLine(line,desc))scalar_fault_park();
 Words(L::Scales)[w]=line[0];
 for(unsigned i=0;i<8;i++)Words(L::Counters)[w*8+i]=after[i];
}
static Q38_PRECISE_ROOT_FN void PreparePrecise(const uint32_t* d){
 Seal(d);uint32_t tracked[8],desc[64],job[64];
 for(unsigned i=0;i<8;i++){tracked[i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();}
 // EVERY existing participant is admitted before the FIRST descriptor.
 for(unsigned w=0;w<119;w++)Admit(d,tracked,w);
 for(unsigned w=0;w<68;w++)if(Words(L::Nonces)[w]>=0x7ffffffd)scalar_fault_park();
 for(unsigned w=0;w<68;w++){
  PreciseDescriptor(d,w,desc);Copy(L::Scratch,reinterpret_cast<unsigned>(desc),64);Transfer(d,tracked,w,Precise::Descriptor,256,true);
  PreciseJob(d,w,Words(L::Nonces)[w]+1,job);Copy(L::Scratch,reinterpret_cast<unsigned>(job),64);
  Copy(L::PeerJobs+w*256,L::Scratch,64);Transfer(d,tracked,w,M::Job,256,true);
 }
 for(unsigned w=0;w<68;w++){*Words(L::Scratch)=Words(L::Nonces)[w]+1;Transfer(d,tracked,w,M::Doorbell,4,true);Words(L::Nonces)[w]++;}
 for(unsigned i=0;i<80;i++)Words(L::Scales)[i]=0;
 for(unsigned w=0;w<68;w++)PreciseGather(d,tracked,w);
 // The first68 have actual activation+wire proof,external/inner job binding,
 // DONE+WAIT and current physical counters from PreciseGather. They cannot
 // leave WAIT without another owned doorbell. Keep the existing last audit
 // for every unused worker;the single-audit policy deliberately defers a
 // later completed-worker metadata fault to subsequent owned checks.
 for(unsigned w=
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT
  68
#else
  0
#endif
  ;w<119;w++){
  Wait(d,tracked,w,0);BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
  for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(L::Counters)[w*8+i])scalar_fault_park();
 }
 // Consume the actual card-produced native wire, not F32 readback or a
 // second quantizer. Scales compact on-card only after real completion.
 for(unsigned at=0;at<34816;at+=16384){unsigned bytes=34816-at;if(bytes>16384)bytes=16384;
  PreciseBankRead(d,tracked,Precise::NativeOffset+at,bytes);Copy(L::Native+at,L::Scratch,bytes/4);
 }
 Copy(L::NativeShadow,L::Native,8704);Copy(L::ScaleShadow,L::Scales,80);Seal(d);
}
#undef Q38_PRECISE_ROOT_FN
