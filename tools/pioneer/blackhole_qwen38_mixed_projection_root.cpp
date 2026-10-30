/* SPDX-License-Identifier: MIT */
// NEW returning projection controller. Old shape checks are DATA only;
// no old QDM/QMR/native image role or source capability is selected.
#if defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
#define scalar_main q38_joined_dense_frame
#else
#define scalar_main q38_unused_dense_frame
#endif
#include "blackhole_qwen38_dense_partition_root.cpp"
#undef scalar_main
#include "PioneerQ38ProjectionInput.h"
#if defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BlockFfnRecipe.h"
#endif
extern "C" unsigned mixed_projection_wait(unsigned);
extern "C" void* memset(void* target,int value,__SIZE_TYPE__ bytes){
 if(value || ((reinterpret_cast<unsigned>(target)|bytes)&3))scalar_fault_park();
 volatile uint32_t* p=static_cast<volatile uint32_t*>(target);
 for(unsigned i=0;i<bytes/4;i++)p[i]=0;
 return target;
}
namespace Projection {
using namespace DensePartitionRoot;
static void CaptureRecipe(const uint32_t* d){
 Copy(L::RecipeShadow,L::Header,64);Copy(L::HeaderShadow,L::MatrixHeader,16);Copy(L::BankShadow,L::Banks,8);
 Copy(L::PartsShadow,L::Parts,119*8);Copy(L::RowsShadow,L::Rows,119);
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=i>=48?0:d[i];old[0]=TT_Q38_DENSE_MATRIX_MARKER;old[13]=TT_Q38_DENSE_MATRIX_ROWS;
#if defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 old[0]=TT_Q38_MIXED_RECURRENT_MATRIX;
 if(!PioneerQ38MixedRecurrentRecipe(old,reinterpret_cast<const uint32_t*>(L::HeaderShadow),reinterpret_cast<const uint32_t*>(L::BankShadow),
  reinterpret_cast<const uint32_t(*)[8]>(L::PartsShadow),reinterpret_cast<const uint32_t*>(L::RowsShadow)))scalar_fault_park();
#elif defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
 bool ffn=d[43]>=2 && d[43]<=4;
 if(ffn)old[0]=L::MatrixMagic;
 bool valid=ffn?PioneerQ38BlockFfnRecipe(old,reinterpret_cast<const uint32_t*>(L::HeaderShadow),reinterpret_cast<const uint32_t*>(L::BankShadow),
  reinterpret_cast<const uint32_t(*)[8]>(L::PartsShadow),reinterpret_cast<const uint32_t*>(L::RowsShadow)):
  PioneerQ38DenseProjectionRecipe(old,reinterpret_cast<const uint32_t*>(L::HeaderShadow),reinterpret_cast<const uint32_t*>(L::BankShadow),
  reinterpret_cast<const uint32_t(*)[8]>(L::PartsShadow),reinterpret_cast<const uint32_t*>(L::RowsShadow),false);
 if(!valid)scalar_fault_park();
#else
 if(!PioneerQ38DenseProjectionRecipe(old,reinterpret_cast<const uint32_t*>(L::HeaderShadow),reinterpret_cast<const uint32_t*>(L::BankShadow),
  reinterpret_cast<const uint32_t(*)[8]>(L::PartsShadow),reinterpret_cast<const uint32_t*>(L::RowsShadow),false))scalar_fault_park();
#endif
 for(unsigned w=0;w<119;w++)FrameSeal(w);
}
static void Quantize(const uint32_t* d){
 unsigned k=d[14],type=d[15],blocks=k>>(type==8?5:8),padded=(blocks+15)&~15U;
 for(unsigned i=0;i<k;i++){
  unsigned x=Words(d[53])[i];if((x&0x7fffffff)>0x477fe000)scalar_fault_park();Words(L::InputShadow)[i]=x;
 }
 for(unsigned i=0;i<padded;i++)Words(L::Scales)[i]=0;
 for(unsigned at=0;at<k;at+=256){
  auto* q=reinterpret_cast<PioneerQ38ProjectionInput*>(L::QuantizedChunk);
  if(!PioneerQ38ProjectionQuantize(type,reinterpret_cast<const float*>(L::InputShadow+at*4),256,q))scalar_fault_park();
  for(unsigned i=0;i<128;i++)Words(L::Native)[at/2+i]=q->operand[i];
  unsigned n=type==8?8:1,first=at>>(type==8?5:8);
  for(unsigned i=0;i<n;i++)Words(L::Scales)[first+i]=q->scale[i];
 }
 for(unsigned i=0;i<k/2;i++)Words(L::NativeShadow)[i]=Words(L::Native)[i];
 for(unsigned i=0;i<padded;i++)Words(L::ScaleShadow)[i]=Words(L::Scales)[i];
 Fence();
}
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
#include "blackhole_qwen38_precise_activation_root.inc"
#endif
static void InputsSeal(const uint32_t* d){
 unsigned k=d[14],blocks=k>>(d[15]==8?5:8),padded=(blocks+15)&~15U;
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
 // This route's source is the genuine packed worker producer, not a
 // fabricated or stale local activated-F32 vector.
 if(!PreciseDown(d))
#endif
 for(unsigned i=0;i<k;i++)if(Words(d[53])[i]!=Words(L::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<k/2;i++)if(Words(L::Native)[i]!=Words(L::NativeShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<padded;i++)if(Words(L::Scales)[i]!=Words(L::ScaleShadow)[i])scalar_fault_park();
}
static void MatrixJob(const uint32_t* d,unsigned w,unsigned sequence){
 const auto* p=Words(L::PartsShadow+w*32);
 for(unsigned i=0;i<64;i++)Words(L::Scratch)[i]=0;
 const uint32_t head[]={M::Magic,sequence,1,w+1,M::Peer(w+1),d[42],d[40],d[6]};
 for(unsigned i=0;i<8;i++){Words(L::Scratch)[i]=head[i];Words(L::Scratch)[8+i]=d[32+i];}
#if defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 const unsigned job[]={0x31483851,d[15],L::Tile(d[15]),0,0,d[14],p[0],p[2],p[4],1};
#else
 const unsigned job[]={0x31483851,d[15],d[15]==8?576U:3392U,0,0,d[14],p[0],p[2],p[4],1};
#endif
 for(unsigned i=0;i<10;i++)Words(L::Scratch)[16+i]=job[i];
 Words(L::Scratch)[26]=1;
}
#ifndef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
static void MatrixWait(const uint32_t* d,uint32_t* tracked,unsigned w){
 unsigned n=Words(L::Nonces)[w];
 for(unsigned attempt=0;attempt<65536;attempt++){
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  Transfer(d,tracked,w,M::Status,64,false);uint32_t s[16];for(unsigned i=0;i<16;i++)s[i]=Words(L::Scratch)[i];
  if(s[3]!=M::Magic || s[2] || s[0]<n-1 || s[0]>n || s[1]<n-1 || s[1]>n || s[4]>1 || s[5])scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  unsigned pc=Register(d,tracked,w,0xffb13140);
#if !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) && !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
  if(!Executing(pc))scalar_fault_park();
#endif
  if(s[0]!=n || s[1]!=n)continue;
  if(s[4] || s[6]!=d[42] || s[7]!=d[40])scalar_fault_park();
  if(pc==0x2004c || pc==0x20050)return;
 }
 scalar_fault_park();
}
#else
#include "blackhole_qwen38_pushed_completion_root.inc"
#endif
static void Gather(const uint32_t* d,uint32_t* tracked,unsigned w){
#ifndef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
 MatrixWait(d,tracked,w);BootSeal(d,tracked,w);
 MatrixJob(d,w,Words(L::Nonces)[w]);Copy(L::PeerJobs+w*256,L::Scratch,64);ClosedJob(d,tracked,w,false);
#else
 MatrixJob(d,w,Words(L::Nonces)[w]);Copy(L::PeerJobs+w*256,L::Scratch,64);
#endif
 unsigned groups=Words(L::PartsShadow+w*32)[2],k=d[14],type=d[15],blocks=k>>(type==8?5:8);
#if defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 unsigned reads=groups*blocks+(k*2+16383)/16384+1,flits=groups*blocks*L::Tile(type)/64+(k*2+((blocks*4+63)&~63U))/64;
#else
 unsigned reads=groups*blocks+(k*2+16383)/16384+1,flits=groups*blocks*(type==8?576:3392)/64+(k*2+((blocks*4+63)&~63U))/64;
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
 uint32_t receiptAfter[8];
 for(unsigned i=0;i<8;i++)receiptAfter[i]=Words(L::Counters)[w*8+i]+(i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U)+PushedDelta(i);
 PushedWait(d,w,receiptAfter);
#endif
 Transfer(d,tracked,w,0x83f00,256+groups*64,false);uint32_t proof[64];
 for(unsigned i=0;i<64;i++)proof[i]=Words(L::Scratch)[i];
 if(proof[16]!=groups*k/16 || proof[17]!=Words(L::Nonces)[w] || proof[18]!=w+1 || proof[19]!=0x32504651
  || proof[30]!=Boot(w)[16] || proof[31]!=M::Peer(w+1))scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(proof[32+i]!=Boot(w)[16+i])scalar_fault_park();
 for(unsigned i=48;i<64;i++)if(proof[i])scalar_fault_park();
 const auto* part=Words(L::PartsShadow+w*32);
#if defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 const unsigned command[]={part[0]+(groups*blocks-1)*L::Tile(type),0,part[4],0x90000,0,M::Peer(w+1),0,0,L::Tile(type),0};
#else
 const unsigned command[]={part[0]+(groups*blocks-1)*(type==8?576U:3392U),0,part[4],0x90000,0,M::Peer(w+1),0,0,type==8?576U:3392U,0};
#endif
 for(unsigned i=0;i<10;i++)if(proof[20+i]!=command[i])scalar_fault_park();
 unsigned first=Words(L::RowsShadow)[w];
 for(unsigned i=0;i<groups*16;i++){
  unsigned value=Words(L::Scratch)[64+i];if((value&0x7f800000)==0x7f800000)scalar_fault_park();Words(L::Projected)[first+i]=value;
 }
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U;
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
  if(proof[i]!=Words(L::Counters)[w*8+i] || uint32_t(proof[8+i]-proof[i])!=delta)scalar_fault_park();
  const unsigned finished=proof[8+i]+PushedDelta(i);
  if(finished!=receiptAfter[i])scalar_fault_park();
  Words(L::Counters)[w*8+i]=finished; // expected frontier;not a current physical observation or recaptured baseline.
#else
  if(proof[i]!=Words(L::Counters)[w*8+i] || uint32_t(proof[8+i]-proof[i])!=delta
   || Register(d,tracked,w,0xffb20000+Counter(i))!=proof[8+i])scalar_fault_park();
  Words(L::Counters)[w*8+i]=proof[8+i];
#endif
 }
}
static void Scatter(const uint32_t* d,uint32_t* tracked){
 InputsSeal(d);
 for(unsigned at=0;at<d[12];at+=16384){
  unsigned bytes=d[12]-at;if(bytes>16384)bytes=16384;Copy(L::Scratch,L::Projected+at,bytes/4);
  Seal(d);for(unsigned w=0;w<119;w++)FrameSeal(w);
  uint32_t expected[8];for(unsigned i=0;i<8;i++){
   if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
   unsigned delta=i==5?bytes/64:i==2 || i==4 || i==6 || i==7?1U:0U;expected[i]=tracked[i]+delta;
  }
  const uint32_t command[]={L::Scratch,0,129,d[4]+d[10]+at,0,d[8],0,0x212,bytes,0};
  for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
  for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
  Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
   if(done)break;
  }
  if(!done)scalar_fault_park();
  Seal(d);
  for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
  Words(L::Summary)[6]++;Words(L::Summary)[7]+=bytes/64;
 }
}
static unsigned Run(const uint32_t* d){
 CaptureRecipe(d);
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
 if(PreciseDown(d))PreparePrecise(d);else
#endif
 Quantize(d);
 Seal(d);uint32_t tracked[8],before[8];
 for(unsigned i=0;i<64;i++)Words(L::Summary)[i]=0;
 for(unsigned i=0;i<8;i++)before[i]=tracked[i]=*Words(0xffb20000+Counter(i));
 if(tracked[2]!=tracked[6])scalar_fault_park();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
 if(!PreciseDown(d)) // PreparePrecise already admits/proves ALL119 histories.
#endif
 for(unsigned w=0;w<119;w++)Admit(d,tracked,w);
 for(unsigned w=0;w<d[7];w++)if(Words(L::Nonces)[w]>=0x7ffffffe)scalar_fault_park();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
 uint32_t incoming[4];PushedBegin(d,tracked,incoming);
#endif
 for(unsigned w=0;w<d[7];w++){MatrixJob(d,w,Words(L::Nonces)[w]+1);Transfer(d,tracked,w,M::Job,256,true);}
 for(unsigned w=0;w<d[7];w++){*Words(L::Scratch)=Words(L::Nonces)[w]+1;Transfer(d,tracked,w,M::Doorbell,4,true);Words(L::Nonces)[w]++;}
 for(unsigned w=0;w<d[7];w++)Gather(d,tracked,w);
 // Non-pushed Gather observes DONE/WAIT,boot/job and current counters.
 // G3 instead binds the pushed receipt,arithmetic proof and expected counter
 // frontier;that is NOT a fresh physical observation. No later command is
 // sent to those workers here. The single-audit policy omits their duplicate
 // audit;unused workers retain it. Later idle faults in G3's warm path are
 // detected by actual worker guards before its next request,not by a new
 // ROOT remote audit. These policies have different fault-observation timing.
 for(unsigned w=
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT
  d[7]
#else
  0
#endif
  ;w<119;w++){
  Wait(d,tracked,w,0);BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
  for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(L::Counters)[w*8+i])scalar_fault_park();
 }
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
 PushedEnd(d,incoming);
#endif
 Scatter(d,tracked);InputsSeal(d);Seal(d);for(unsigned w=0;w<119;w++)FrameSeal(w);
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
 PushedEnd(d,incoming);
#endif
 auto* s=Words(L::Summary);for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  s[8+i]=before[i];s[16+i]=tracked[i];
 }
 for(unsigned i=0;i<16;i++)s[24+i]=d[16+i];
 for(unsigned i=0;i<8;i++)s[40+i]=d[32+i];
 s[1]=d[7];s[2]=d[11];s[3]=d[14];s[4]=d[42];s[5]=d[43];s[48]=d[48];s[49]=d[40];
 Fence();s[0]=L::Magic;Fence();return d[11];
}
}
#if !defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) && !defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) && !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) && !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
extern "C" void q38_mixed_root_seal(){
 using namespace DensePartitionRoot;const L::PrivateSeal* p;asm volatile("mv %0,s11":"=r"(p));
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x6fe00)scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(L::Boot)[i]!=p->boot[i] || Words(L::Header)[i]!=p->frame[i])scalar_fault_park();
 if(*Words(L::Doorbell)!=p->frame[48])scalar_fault_park();
}
extern "C" void mixed_projection_main(){
 using namespace DensePartitionRoot;L::PrivateSeal seal;uint32_t counters[8];
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(L::Boot)[i];
 if(!L::BootShape(seal.boot) || *Words(L::Doorbell) || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=seal.boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
 if(counters[2]!=counters[6])scalar_fault_park();
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 for(unsigned i=0;i<16;i++)Words(L::Status)[i]=0;
 Words(L::Status)[3]=L::Magic;Fence();unsigned previous=0;
 for(;;){
  unsigned n=mixed_projection_wait(previous);for(unsigned i=0;i<64;i++)seal.frame[i]=Words(L::Header)[i];
  const auto* d=seal.frame;
  if(n!=previous+1 || d[48]!=n || !L::Shape(d) || d[9]!=seal.boot[3])scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(d[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(d[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  q38_mixed_root_seal();Words(L::Status)[0]=n;Words(L::Status)[4]=1;Fence();
  unsigned rows=Projection::Run(d);q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
  for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
  previous=n;Words(L::Status)[5]=d[42];Words(L::Status)[6]=d[43];Words(L::Status)[7]=rows;
  Words(L::Status)[4]=0;Fence();Words(L::Status)[1]=n;Fence();
 }
}
#endif
