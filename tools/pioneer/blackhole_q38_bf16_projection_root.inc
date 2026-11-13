/* SPDX-License-Identifier: MIT */
// Included INSIDE Projection. Actual returning QGW4 producer/consumer
// lifecycle. No host expected products,ready setters or old scale grants.
#include "PioneerQ38Bf16RootRole.h"
namespace BFP=PioneerQ38Bf16ProducerLayout;
using BFC=PioneerQ38Q4FpuCachedLayout;
using BFR=PioneerQ38Bf16RootPairs;
#define BF_ROOT_FN __attribute__((noinline,section(".bf16.root")))
static bool BfQ4(const uint32_t* d){return d[15]==12 && d[43]>=2 && d[43]<=4;}
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
#include "PioneerQ38RecurrentSmallRootJobs.h"
static bool BfSmallQ8(const uint32_t* d){return d[15]==8 && d[42]<64 && (d[42]&3)!=3 && (d[43]==10 || d[43]==11);}
static __attribute__((noinline,section(".bf16.root"))) unsigned BfSmallRecord(const uint32_t* d){
 return 0x3004a00U+((d[42]-d[42]/4)*2+d[43]-10)*128;
}
#endif
static bool BfRecurrentQ8(const uint32_t* d){return d[15]==8 && d[42]<64 && (d[42]&3)!=3 && (d[43]==5 || d[43]==6 || d[43]==13);}
static __attribute__((noinline,section(".bf16.root"))) unsigned BfRecurrentRecord(const uint32_t* d){
 const unsigned layer=d[42]-d[42]/4,role=d[43]==5?0:d[43]==6?1:2;
 return 0x3000200U+(layer*3+role)*128;
}
static void BfBankRead(const uint32_t*,uint32_t*,unsigned,unsigned);
#endif
static bool BfDenseQ8(const uint32_t* d){return (d[15]==8 && d[42]<64 && (d[42]&3)==3 && d[43]>=5 && d[43]<=7)
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 || BfRecurrentQ8(d)
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
 || BfSmallQ8(d)
#endif
#endif
 ;}
// Experimental ROOT-local metadata only. The native owner must still
// authenticate, upload and preserve this table and its BFP8 weights.
struct BfDenseScheduleIO {
 enum {Manifest=0x16dc00,ManifestWords=16*3*7*3,Parts=0x16ebc0,Shadow=0x16f330,TableShadow=0x16fac0,End=0x16fb20};
 unsigned index;
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 enum {RecurrentRecord=0x16fb40};
 unsigned recurrent;
#endif
 unsigned Table(unsigned bank,unsigned field){
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  if(recurrent)return Words(RecurrentRecord)[bank*3+field];
#endif
  return Words(Manifest)[index+bank*3+field];
 }
 void Output(unsigned worker,unsigned field,unsigned value){Words(Parts+worker*16)[field]=value;Words(Shadow+worker*16)[field]=value;}
};
static_assert(BfDenseScheduleIO::Manifest>=0x16da00+7*4
 && BfDenseScheduleIO::Manifest+BfDenseScheduleIO::ManifestWords*4==BfDenseScheduleIO::Parts
 && BfDenseScheduleIO::Parts+119*16<=BfDenseScheduleIO::Shadow
 && BfDenseScheduleIO::Shadow+119*16<=BfDenseScheduleIO::TableShadow
 && BfDenseScheduleIO::TableShadow+21*4<=BfDenseScheduleIO::End && BfDenseScheduleIO::End<=unsigned(L::PeerBoots),
 "isolated dense BFP8 schedule metadata must not overlap the FFN table or peer boots");
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
static_assert(unsigned(BfDenseScheduleIO::RecurrentRecord)>=unsigned(BfDenseScheduleIO::End)
 && BfDenseScheduleIO::RecurrentRecord+128<=unsigned(L::PeerBoots),"recurrent record must not overlap live ROOT data");
#endif
static BF_ROOT_FN void BfDenseSeal(const uint32_t* d){
 if(!BfDenseQ8(d))scalar_fault_park();
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 // No command is issued during this read-only scan. Select the manifest
 // once, and recheck the descriptor before leaving the scan.
 const volatile uint32_t* descriptor=d;
 const unsigned layer=descriptor[42],role=descriptor[43],format=descriptor[15];
 const bool recurrent=format==8 && layer<64 && (layer&3)!=3 && (role==5 || role==6 || role==13
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
  || role==10 || role==11
#endif
 );
 const bool dense=format==8 && layer<64 && (layer&3)==3 && role>=5 && role<=7;
 if(!recurrent && !dense)scalar_fault_park();
 const unsigned index=((layer/4)*3+role-5)*21;
 const auto* table=recurrent?Words(BfDenseScheduleIO::RecurrentRecord)
  :Words(BfDenseScheduleIO::Manifest)+index;
 for(unsigned i=0;i<21;i++)if(table[i]!=Words(BfDenseScheduleIO::TableShadow)[i])scalar_fault_park();
 if(descriptor[42]!=layer || descriptor[43]!=role || descriptor[15]!=format)scalar_fault_park();
#else
 const unsigned index=((d[42]/4)*3+d[43]-5)*21;
 for(unsigned i=0;i<21;i++)if(
  Words(BfDenseScheduleIO::Manifest)[index+i]
  !=Words(BfDenseScheduleIO::TableShadow)[i])scalar_fault_park();
#endif
 for(unsigned w=0;w<119;w++){
  const auto* actual=Words(BfDenseScheduleIO::Parts+w*16);
  const auto* shadow=Words(BfDenseScheduleIO::Shadow+w*16);
  const uint32_t difference=(actual[0]^shadow[0])|(actual[1]^shadow[1])
   |(actual[2]^shadow[2])|(actual[3]^shadow[3]);
  if(difference)scalar_fault_park();
 }
}
#ifdef PIONEER_Q38_RECURRENT_SCOPED_SEAL_EXPERIMENT
#if PIONEER_Q38_RECURRENT_SCOPED_SEAL_EXPERIMENT != 1 || !defined(PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Scoped recurrent seals are an inspection-only experiment
#endif
// Only this worker's tuple is consumed in the enclosed operation. The full
// schedule is sealed after construction and again before any scatter. A
// change to another tuple may therefore be detected later than in the full
// audit. Each tuple is checked at its own use; exclusive ownership is still
// required to rule out mutations between checks and use.
static BF_ROOT_FN void BfDenseScopedSeal(const uint32_t* d,unsigned w){
 if(!BfRecurrentQ8(d) || w>=119)scalar_fault_park();
 const volatile uint32_t* descriptor=d;
 const unsigned layer=descriptor[42],role=descriptor[43],format=descriptor[15];
 if(format!=8 || layer>=64 || (layer&3)==3 || (role!=5 && role!=6 && role!=13))scalar_fault_park();
 for(unsigned i=0;i<21;i++)if(Words(BfDenseScheduleIO::RecurrentRecord)[i]!=Words(BfDenseScheduleIO::TableShadow)[i])scalar_fault_park();
 const auto* actual=Words(BfDenseScheduleIO::Parts+w*16);
 const auto* shadow=Words(BfDenseScheduleIO::Shadow+w*16);
 if((actual[0]^shadow[0])|(actual[1]^shadow[1])|(actual[2]^shadow[2])|(actual[3]^shadow[3]))scalar_fault_park();
 if(descriptor[42]!=layer || descriptor[43]!=role || descriptor[15]!=format)scalar_fault_park();
}
#define Q38_DENSE_WORKER_SEAL(d,w) do { if(BfRecurrentQ8(d))BfDenseScopedSeal(d,w);else BfDenseSeal(d); } while(0)
#else
#define Q38_DENSE_WORKER_SEAL(d,w) BfDenseSeal(d)
#endif
static BF_ROOT_FN void BfBuildDenseSchedule(const uint32_t* d
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 ,uint32_t* tracked
#endif
 ){
 if(!BfDenseQ8(d))scalar_fault_park();
 BfDenseScheduleIO io;io.index=((d[42]/4)*3+d[43]-5)*21;
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 io.recurrent=BfRecurrentQ8(d)
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
  || BfSmallQ8(d)
#endif
 ;
 if(io.recurrent){
  BfBankRead(d,tracked,
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
   BfSmallQ8(d)?BfSmallRecord(d):
#endif
   BfRecurrentRecord(d),128);
  for(unsigned i=0;i<32;i++)Words(BfDenseScheduleIO::RecurrentRecord)[i]=Words(L::Scratch)[i];
  for(unsigned i=21;i<32;i++)if(Words(BfDenseScheduleIO::RecurrentRecord)[i])scalar_fault_park();
 }
#endif
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 for(unsigned b=0;b<7;b++)for(unsigned f=0;f<3;f++)Words(BfDenseScheduleIO::TableShadow)[b*3+f]=io.Table(b,f);
#else
 for(unsigned i=0;i<21;i++)Words(BfDenseScheduleIO::TableShadow)[i]=Words(BfDenseScheduleIO::Manifest)[io.index+i];
#endif
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)io.Output(w,i,0);
 Fence();if(
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  io.recurrent?
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
   (BfSmallQ8(d)?!PioneerQ38RecurrentSmallRootJobs::Build(io,d[42],d[43]):!PioneerQ38RecurrentQ8MatrixRootJobs::Build(io,d[42],d[43])):
#else
   !PioneerQ38RecurrentQ8MatrixRootJobs::Build(io,d[42],d[43]):
#endif
#endif
  !PioneerQ38DenseQ8MatrixRootJobs::Build(io,d[42],d[43]))scalar_fault_park();
 Fence();BfDenseSeal(d);
}
#endif
static bool BfProducerRole(const uint32_t* d){
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 if(BfDenseQ8(d))return true;
#endif
 return BfQ4(d);
}
static void MatrixWait(const uint32_t*,uint32_t*,unsigned);
struct BfPairIO {
 unsigned Part(unsigned w,unsigned i){return Words(L::PartsShadow+w*32)[i];}
 unsigned Row(unsigned w){return Words(L::RowsShadow)[w];}
 unsigned Route(unsigned b){return Words(L::BankShadow)[b];}
 unsigned Source(unsigned w){return Part(w,5);}
 void Output(unsigned w,unsigned i,unsigned v){Words(BFR::Parts+w*32)[i]=v;Words(BFR::Shadow+w*32)[i]=v;}
 void OutputRow(unsigned w,unsigned v){Words(BFR::Rows)[w]=v;Words(BFR::RowShadow)[w]=v;}
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 // One model-wide 64*3*7 destination manifest, selected on-card per
 // projection. Caller/owner must authenticate all addresses and pages.
 enum {TableAddress=0x16c400,TableShadow=0x16da00,TableWords=64*3*7};
 unsigned Table(unsigned bank,unsigned i){
  if(i==1)return Words(TableShadow)[bank];
  unsigned address=0,first=0,rows=0;bool found=false;
  for(unsigned w=0;w<119;w++)if(Part(w,1)==bank){
   if(!found){address=Part(w,0);first=Row(w);found=true;}
   rows+=Part(w,2)*16;
  }
  return !found?0U:i==0?address:i==2?first:rows;
 }
 unsigned Result(unsigned w,unsigned i){return Words(BFR::Parts+w*32)[i];}
 unsigned ResultRow(unsigned w){return Words(BFR::Rows)[w];}
#endif
};
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
static BF_ROOT_FN void BfPairSeal(unsigned w,const uint32_t* d){
#define BF_PAIR_SEAL(d,w) BfPairSeal(w,d)
#else
static BF_ROOT_FN void BfPairSeal(unsigned w){
#define BF_PAIR_SEAL(d,w) BfPairSeal(w)
#endif
 if(w>=119)scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(Words(BFR::Parts+w*32)[i]!=Words(BFR::Shadow+w*32)[i])scalar_fault_park();
 if(Words(BFR::Rows)[w]!=Words(BFR::RowShadow)[w])scalar_fault_park();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 if(!BfQ4(d) || d[42]>=64)scalar_fault_park();
 const unsigned index=(d[42]*3+d[43]-2)*7;
 for(unsigned i=0;i<7;i++)if(Words(BfPairIO::TableAddress)[index+i]!=Words(BfPairIO::TableShadow)[i])scalar_fault_park();
#endif
}
static BF_ROOT_FN void BfBuildPairs(const uint32_t* d
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 ,uint32_t* tracked
#endif
 ){
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 if(BfDenseQ8(d)){BfBuildDenseSchedule(d
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  ,tracked
#endif
  );return;}
#endif
 static_assert(BFR::Parts>=0x167000 && BFR::Rows>=BFR::Parts+119*32 && BFR::Shadow>=BFR::Rows+119*4
  && BFR::RowShadow>=BFR::Shadow+119*32 && BFR::End>=BFR::RowShadow+119*4 && BFR::End<=unsigned(L::PeerBoots),
  "NEW paired metadata extents exclude retired cohort shadows,final hidden and live peer boot/job storage");
 BfPairIO io;if(!BfQ4(d) || !BFR::Build(io,d[14],d[11]))scalar_fault_park();Fence();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 static_assert(unsigned(BfPairIO::TableAddress)>=unsigned(BFR::End)
  && BfPairIO::TableShadow>=BfPairIO::TableAddress+BfPairIO::TableWords*4
  && BfPairIO::TableShadow+7*4<=unsigned(L::PeerBoots),"experimental ROOT manifest/shadow geometry only");
 if(d[42]>=64)scalar_fault_park();
 const unsigned index=(d[42]*3+d[43]-2)*7;
 for(unsigned i=0;i<7;i++)Words(BfPairIO::TableShadow)[i]=Words(BfPairIO::TableAddress)[index+i];
 Fence();if(!PioneerQ38HybridRootPairs::Relocate(io,d[42],d[14],d[11]))scalar_fault_park();
 for(unsigned w=0;w<119;w++)BF_PAIR_SEAL(d,w);
 Fence();
#endif
}
static BF_ROOT_FN void BfDescriptor(const uint32_t* d,unsigned w,uint32_t* out){
 const bool gated=d[43]==4;const unsigned chunks=gated?68:
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  d[14]/256;
#else
  20;
#endif
 if(!BfProducerRole(d) || w>=chunks)scalar_fault_park();
 for(unsigned i=0;i<64;i++)out[i]=0;
 out[0]=BFP::Magic;out[1]=1;out[2]=67108864;out[3]=d[3];out[4]=d[4];
 out[5]=w;out[6]=d[6];out[7]=w+1;out[8]=d[8];out[9]=M::Peer(w+1);
 out[10]=(gated?unsigned(BFP::GateOffset):unsigned(BFP::ProjectionInput))+w*1024;
 out[11]=gated?BFP::UpOffset+w*1024:
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  d[14];
#else
  5120;
#endif
 out[12]=BFP::NativeOffset+w*512;out[13]=BFP::LinesOffset+w*128;out[14]=BFP::Format;out[15]=chunks;
 for(unsigned i=0;i<16;i++)out[16+i]=Boot(w)[16+i];
 for(unsigned i=0;i<8;i++)out[32+i]=d[32+i];
 out[40]=d[40];out[41]=d[41];out[42]=d[42];out[43]=d[43];
 if(!BFP::DescriptorShape(out))scalar_fault_park();
}
static BF_ROOT_FN void BfProducerJob(const uint32_t* d,unsigned w,unsigned n,uint32_t* out){
 for(unsigned i=0;i<64;i++)out[i]=0;
 const unsigned h[]={M::Magic,n,d[43]==4?3U:5U,w+1,M::Peer(w+1),d[42],d[40],d[6]};
 for(unsigned i=0;i<8;i++){out[i]=h[i];out[8+i]=d[32+i];}
 out[16]=BFP::Magic;out[17]=256;out[18]=512;out[19]=d[6];out[20]=12;
 out[27]=d[41];out[28]=d[43];out[29]=d[14];out[30]=BFP::Format;out[31]=BFP::Witness;
 if(!n)scalar_fault_park();
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 if(!BfQ4(d)){
  if(!PioneerQ38DenseQ8MatrixWorkerLayout::JobShape(out,Boot(w),n-1,0))scalar_fault_park();
 }else
#endif
 if(!M::JobShape(out,Boot(w),n-1,0))scalar_fault_park();
}
static BF_ROOT_FN void BfProducerWait(const uint32_t* d,uint32_t* tracked,unsigned w){
 const unsigned n=Words(L::Nonces)[w],op=d[43]==4?3:5;
 for(unsigned poll=0;poll<65536;poll++){
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  Transfer(d,tracked,w,M::Status,64,false);uint32_t s[16];for(unsigned i=0;i<16;i++)s[i]=Words(L::Scratch)[i];
  if(s[3]!=M::Magic || s[2] || s[0]<n-1 || s[0]>n || s[1]<n-1 || s[1]>s[0]
   || (s[4] && s[4]!=op) || s[5] || s[6]!=d[42] || s[7]!=d[40])scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  const unsigned pc=Register(d,tracked,w,0xffb13140);
  if(s[0]==n && s[1]==n && !s[4] && (pc==0x2004c || pc==0x20050))return;
 }
 scalar_fault_park();
}
static BF_ROOT_FN void BfBankRead(const uint32_t* d,uint32_t* tracked,unsigned offset,unsigned bytes){
 const unsigned wire=d[14]*2,lines=(d[14]/256)*128;
 const bool native=offset>=unsigned(BFP::NativeOffset) && offset<=BFP::NativeOffset+wire && bytes<=BFP::NativeOffset+wire-offset;
 const bool trailer=offset>=unsigned(BFP::LinesOffset) && offset<=BFP::LinesOffset+lines && bytes<=BFP::LinesOffset+lines-offset;
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 const bool record=bytes==128 && ((BfRecurrentQ8(d) && offset==BfRecurrentRecord(d))
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
  || (BfSmallQ8(d) && offset==BfSmallRecord(d))
#endif
 );
 if(!BfProducerRole(d) || (!native && !trailer && !record) || !bytes || bytes>16384 || ((offset|bytes)&63))scalar_fault_park();
#else
 if(!BfProducerRole(d) || (!native && !trailer) || !bytes || bytes>16384 || ((offset|bytes)&63))scalar_fault_park();
#endif
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
  const unsigned busy=*Words(0xffb20040);if(busy>1)scalar_fault_park();done=!busy;
  for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done)break;
 }
 if(!done)scalar_fault_park();
 q38_mixed_root_packet_seal(d);if(!Quiet())scalar_fault_park();
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
 Fence();
}
static BF_ROOT_FN void BfGatherProducer(const uint32_t* d,uint32_t* tracked,unsigned w){
 uint32_t desc[64],job[16],proof[64],before[8],after[8];
 BfDescriptor(d,w,desc);BfProducerWait(d,tracked,w);BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
 Transfer(d,tracked,w,BFP::Descriptor,256,false);
 for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=desc[i])scalar_fault_park();
 Transfer(d,tracked,w,0x74000,64,false);for(unsigned i=0;i<16;i++)job[i]=Words(L::Scratch)[i];
 Transfer(d,tracked,w,0x83f00,256,false);for(unsigned i=0;i<64;i++)proof[i]=Words(L::Scratch)[i];
#ifdef PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE
 for(unsigned i=0;i<8;i++)before[i]=Words(L::Counters)[w*8+i];
 BF_PAIR_SEAL(d,w);GatherCounters(d,tracked,w,after);BF_PAIR_SEAL(d,w);
#else
 for(unsigned i=0;i<8;i++){before[i]=Words(L::Counters)[w*8+i];after[i]=Register(d,tracked,w,0xffb20000+Counter(i));}
#endif
 if(!BFP::Proof(proof,job,desc,Boot(w)+16,w+1,M::Peer(w+1),before,after))scalar_fault_park();
 const auto* outer=Words(L::PeerJobs+w*256);
 for(unsigned i=0;i<10;i++)if(job[i]!=outer[16+i])scalar_fault_park();
 if(job[12]!=outer[1])scalar_fault_park();
 BfBankRead(d,tracked,BFP::LinesOffset+w*128,128);uint32_t line[32];for(unsigned i=0;i<32;i++)line[i]=Words(L::Scratch)[i];
 if(!BFP::Line(line,desc,job[12]))scalar_fault_park();
 for(unsigned i=0;i<8;i++)Words(L::Counters)[w*8+i]=after[i];
}
#ifdef PIONEER_Q38_SMALL_INPUT_REUSE_INSPECTION
// The previous completed alpha left the SAME BF16 input in the protected
// ROOT input/native shadows. Require both alpha matrix histories and all 18
// producer-only histories to bind the same layer and token and the immediately
// preceding projection invocation.
// Actual admission below authenticates every retained job and receipt before
// this candidate may be used; no caller-ready flag grants reuse.
static BF_ROOT_FN bool BfSmallReuseCandidate(const uint32_t* d){
#ifndef PIONEER_Q38_QKV_ALPHA_INPUT_REUSE_INSPECTION
 if(!BfSmallQ8(d) || d[43]!=11 || d[14]!=5120 || d[6]<=1)return false;
 for(unsigned w=0;w<20;w++){
  const auto* p=Words(L::PeerJobs+w*256);const unsigned nonce=Words(L::Nonces)[w];
  if(!nonce || p[0]!=M::Magic || p[1]!=nonce || p[3]!=w+1 || p[4]!=M::Peer(w+1)
   || p[5]!=d[42] || p[6]!=d[40] || p[7]!=d[6]-1)return false;
  for(unsigned i=0;i<8;i++)if(p[8+i]!=d[32+i])return false;
  if(w<2){
   if(p[2]!=1 || p[16]!=PioneerQ38Bfp8WorkerJob::Magic || p[32]!=w*32 || p[33]!=64 || p[34]!=10 || p[35]!=8)return false;
  }else if(p[2]!=5 || p[16]!=BFP::Magic || p[27]!=d[41] || p[28]!=10 || p[29]!=5120)return false;
 }
#else
 if(!BfSmallQ8(d) || d[14]!=5120 || d[6]<=1)return false;
 const unsigned checked=d[43]==10?119:20;
 for(unsigned w=0;w<checked;w++){
  const auto* p=Words(L::PeerJobs+w*256);const unsigned nonce=Words(L::Nonces)[w];
  if(!nonce || p[0]!=M::Magic || p[1]!=nonce || p[3]!=w+1 || p[4]!=M::Peer(w+1)
   || p[5]!=d[42] || p[6]!=d[40] || p[7]!=d[6]-1)return false;
  for(unsigned i=0;i<8;i++)if(p[8+i]!=d[32+i])return false;
  if(d[43]==10){
   if(p[2]!=1 || p[16]!=PioneerQ38Bfp8WorkerJob::Magic || p[18]!=5120 || p[33]!=10240 || p[34]!=6 || p[35]!=8)return false;
  }else if(w<2){
   if(p[2]!=1 || p[16]!=PioneerQ38Bfp8WorkerJob::Magic || p[32]!=w*32 || p[33]!=64 || p[34]!=10 || p[35]!=8)return false;
  }else {
   const bool producer=p[2]==5 && p[16]==BFP::Magic && p[27]==d[41] && p[28]==10 && p[29]==5120;
   const bool qkv=p[2]==1 && p[16]==PioneerQ38Bfp8WorkerJob::Magic && p[18]==5120 && p[33]==10240 && p[34]==6 && p[35]==8;
   if(!producer && !qkv)return false;
  }
 }
#endif
 for(unsigned i=0;i<5120;i++)if(Words(d[53])[i]!=Words(L::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<2560;i++)if(Words(L::Native)[i]!=Words(L::NativeShadow)[i])scalar_fault_park();
 return true;
}
#endif
#ifdef PIONEER_Q38_RECURRENT_GATE_INPUT_REUSE_INSPECTION
// QKV immediately after recurrent gate can reuse the same prepared input
// only when every retained peer belongs to the same gate invocation. Admit
// still validates the actual closed jobs and receipts before reuse.
static BF_ROOT_FN bool BfRecurrentGateReuseCandidate(const uint32_t* d){
 if(!BfRecurrentQ8(d) || d[43]!=6 || d[14]!=5120 || d[6]<=1)return false;
 for(unsigned w=0;w<119;w++){
  const auto* p=Words(L::PeerJobs+w*256);const unsigned nonce=Words(L::Nonces)[w];
  if(!nonce || p[0]!=M::Magic || p[1]!=nonce || p[2]!=1 || p[3]!=w+1 || p[4]!=M::Peer(w+1)
   || p[5]!=d[42] || p[6]!=d[40] || p[7]!=d[6]-1 || p[16]!=PioneerQ38Bfp8WorkerJob::Magic
   || p[18]!=5120 || p[33]!=6144 || p[34]!=5 || p[35]!=8)return false;
  for(unsigned i=0;i<8;i++)if(p[8+i]!=d[32+i])return false;
 }
 for(unsigned i=0;i<5120;i++)if(Words(d[53])[i]!=Words(L::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<2560;i++)if(Words(L::Native)[i]!=Words(L::NativeShadow)[i])scalar_fault_park();
 return true;
}
#ifdef PIONEER_Q38_QKV_ALPHA_INPUT_REUSE_INSPECTION
// After the small role-11 projection, workers 1 and 2 hold its matrix
// receipt; workers 3..20 still hold the role-10 producer receipt. The
// subsequent 119-worker admission authenticates every actual closed job.
static BF_ROOT_FN bool BfRecurrentSmallToGateReuseCandidate(const uint32_t* d){
 if(!BfRecurrentQ8(d) || d[43]!=5 || d[14]!=5120 || d[6]<=2)return false;
 for(unsigned w=0;w<20;w++){
  const auto* p=Words(L::PeerJobs+w*256);const unsigned nonce=Words(L::Nonces)[w];
  if(!nonce || p[0]!=M::Magic || p[1]!=nonce || p[3]!=w+1 || p[4]!=M::Peer(w+1)
   || p[5]!=d[42] || p[6]!=d[40] || p[7]!=d[6]-(w<2?1U:2U))return false;
  for(unsigned i=0;i<8;i++)if(p[8+i]!=d[32+i])return false;
  if(w<2){
   if(p[2]!=1 || p[16]!=PioneerQ38Bfp8WorkerJob::Magic || p[32]!=w*32 || p[33]!=64 || p[34]!=11 || p[35]!=8)return false;
  }else if(p[2]!=5 || p[16]!=BFP::Magic || p[27]!=d[41] || p[28]!=10 || p[29]!=5120)return false;
 }
 for(unsigned i=0;i<5120;i++)if(Words(d[53])[i]!=Words(L::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<2560;i++)if(Words(L::Native)[i]!=Words(L::NativeShadow)[i])scalar_fault_park();
 return true;
}
#endif
#endif
static BF_ROOT_FN void BfPrepare(const uint32_t* d
#ifdef PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE
 ,PioneerQ38Bf16DeferredAudit& audit
#endif
 ){
 Seal(d);
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 uint32_t tracked[8],desc[64],job[64];
 for(unsigned i=0;i<8;i++){tracked[i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();}
 BfBuildPairs(d,tracked);
#else
 BfBuildPairs(d);uint32_t tracked[8],desc[64],job[64];
#endif
#ifdef PIONEER_Q38_SMALL_INPUT_REUSE_INSPECTION
 const bool reuseSmallInput=BfSmallReuseCandidate(d);
#endif
#ifdef PIONEER_Q38_RECURRENT_GATE_INPUT_REUSE_INSPECTION
 const bool reuseGateInput=BfRecurrentGateReuseCandidate(d);
#ifdef PIONEER_Q38_QKV_ALPHA_INPUT_REUSE_INSPECTION
 const bool reuseSmallToGateInput=BfRecurrentSmallToGateReuseCandidate(d);
#endif
#endif
 static_assert(unsigned(L::InputShadow)==unsigned(BFP::ProjectionInput) && unsigned(L::Native)==0xa0000,"actual QGW4 ROOT buffers");
 if(d[43]!=4
#ifdef PIONEER_Q38_SMALL_INPUT_REUSE_INSPECTION
  && !reuseSmallInput
#endif
#ifdef PIONEER_Q38_RECURRENT_GATE_INPUT_REUSE_INSPECTION
  && !reuseGateInput
#ifdef PIONEER_Q38_QKV_ALPHA_INPUT_REUSE_INSPECTION
  && !reuseSmallToGateInput
#endif
#endif
 )for(unsigned i=0;i<
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  d[14]
#else
  5120
#endif
  ;i++){
  const unsigned bits=Words(d[53])[i];if((bits&0x7fffffff)>0x477fe000)scalar_fault_park();Words(L::InputShadow)[i]=bits;
 }
#ifndef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
 for(unsigned i=0;i<8;i++){tracked[i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();}
#endif
 for(unsigned w=0;w<119;w++)Admit(d,tracked,w);
#ifdef PIONEER_Q38_SMALL_INPUT_REUSE_INSPECTION
 if(reuseSmallInput){Seal(d);return;}
#endif
#ifdef PIONEER_Q38_RECURRENT_GATE_INPUT_REUSE_INSPECTION
 if(reuseGateInput){Seal(d);return;}
#ifdef PIONEER_Q38_QKV_ALPHA_INPUT_REUSE_INSPECTION
 if(reuseSmallToGateInput){Seal(d);return;}
#endif
#endif
#ifdef PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE
 if(!audit.Begin(d[14],d[7]))scalar_fault_park();
#endif
 const unsigned chunks=d[14]/256;
 for(unsigned w=0;w<chunks;w++)if(Words(L::Nonces)[w]>=0x7ffffffc)scalar_fault_park();
 for(unsigned w=0;w<chunks;w++){
  BfDescriptor(d,w,desc);Copy(L::Scratch,reinterpret_cast<unsigned>(desc),64);Transfer(d,tracked,w,BFP::Descriptor,256,true);
  BfProducerJob(d,w,Words(L::Nonces)[w]+1,job);Copy(L::Scratch,reinterpret_cast<unsigned>(job),64);
  Copy(L::PeerJobs+w*256,L::Scratch,64);Transfer(d,tracked,w,M::Job,256,true);
 }
 for(unsigned w=0;w<chunks;w++){*Words(L::Scratch)=Words(L::Nonces)[w]+1;Transfer(d,tracked,w,M::Doorbell,4,true);Words(L::Nonces)[w]++;}
 for(unsigned w=0;w<chunks;w++)BfGatherProducer(d,tracked,w);
#ifndef PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE
 for(unsigned w=chunks;w<119;w++){
  Wait(d,tracked,w,0);BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
  for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(L::Counters)[w*8+i])scalar_fault_park();
 }
#endif
 // Opt-in policy: producer-unused workers have received NO commands since
 // initial Admit. Their extra pre-matrix idle audit is deferred,not a fresh
 // receipt. Run MUST gather/audit ALL119 before Scatter/return;the ledger
 // enforces that boundary. Worker private admission and all transfer guards
 // remain. An idle fault can be detected later than under the old policy.
 for(unsigned at=0;at<d[14]*2;at+=16384){
  unsigned bytes=d[14]*2-at;if(bytes>16384)bytes=16384;
  BfBankRead(d,tracked,BFP::NativeOffset+at,bytes);Copy(L::Native+at,L::Scratch,bytes/4);
 }
 Copy(L::NativeShadow,L::Native,d[14]/2);Seal(d);
}
static BF_ROOT_FN void BfInputsSeal(const uint32_t* d){
 if(d[43]!=4)for(unsigned i=0;i<
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  d[14]
#else
  5120
#endif
  ;i++)if(Words(d[53])[i]!=Words(L::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<d[14]/2;i++)if(Words(L::Native)[i]!=Words(L::NativeShadow)[i])scalar_fault_park();
}
static BF_ROOT_FN void BfMatrixJob(const uint32_t* d,unsigned w,unsigned n){
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 if(BfDenseQ8(d)){
  if(w>=
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
   (
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
    BfSmallQ8(d)?2U:
#endif
    BfRecurrentQ8(d)?119U:PioneerQ38DenseQ8MatrixRootJobs::Active(d[43]))
#else
   PioneerQ38DenseQ8MatrixRootJobs::Active(d[43])
#endif
   || !n)scalar_fault_park();
  Q38_DENSE_WORKER_SEAL(d,w);uint32_t tuple[4],job[64];
  for(unsigned i=0;i<4;i++)tuple[i]=Words(BfDenseScheduleIO::Parts+w*16)[i];
  if(!PioneerQ38DenseQ8MatrixRootFrame::Build(tuple,Boot(w),d[42],d[43],d[40],d[6],n-1,job))scalar_fault_park();
  for(unsigned i=0;i<64;i++)Words(L::Scratch)[i]=job[i];
  Q38_DENSE_WORKER_SEAL(d,w);return;
 }
#endif
 BF_PAIR_SEAL(d,w);if(!n || n>=0x7ffffffe)scalar_fault_park();const auto* p=Words(BFR::Parts+w*32);
 for(unsigned i=0;i<64;i++)Words(L::Scratch)[i]=0;
 const unsigned h[]={M::Magic,n,1,w+1,M::Peer(w+1),d[42],d[40],d[6]};
 for(unsigned i=0;i<8;i++){Words(L::Scratch)[i]=h[i];Words(L::Scratch)[8+i]=d[32+i];}
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
#ifdef PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION
 const bool eight=d[42]>=50 && d[42]<=60;
#else
 const bool eight=d[42]>=48 && d[42]<=60;
#endif
 const unsigned leaf[]={eight?unsigned(PioneerQ38Bfp8WorkerJob::Magic):unsigned(PioneerQ38Bfp4WorkerJob::Magic),
  1,d[14],p[0],p[2]/2,p[4],w+1,M::Peer(w+1),BFP::Format,0xa0000,129,n,0x84000,0,0,0};
#else
 const unsigned leaf[]={BFC::Magic,1,d[14],p[0],p[2],p[4],w+1,M::Peer(w+1),BFP::Format,0xa0000,129,n,0x84000,0,0,0};
#endif
 for(unsigned i=0;i<16;i++)Words(L::Scratch)[16+i]=leaf[i];
 Words(L::Scratch)[32]=Words(BFR::Rows)[w];Words(L::Scratch)[33]=d[11];Words(L::Scratch)[34]=d[43];
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 Words(L::Scratch)[35]=eight?8:4;
#else
 Words(L::Scratch)[35]=12;
#endif
 if(!M::JobShape(const_cast<const uint32_t*>(Words(L::Scratch)),Boot(w),n-1,0))scalar_fault_park();
}
static BF_ROOT_FN bool BfAdmitPrevious(const uint32_t* d,uint32_t* tracked,unsigned w,const uint32_t* previous){
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
 // A small matrix leaves workers 2..19 on their producer job. Admit that
 // actual prior frontier rather than treating its receipt as an integer
 // matrix. Rebuild the descriptor from the closed job's own historical
 // layer/position/token/role; the current projection may be different.
 if(previous[2]==5 && previous[16]==BFP::Magic && (previous[28]==10 || previous[28]==11)){
  // The next projection has its own increasing invocation. Workers not
  // used by the small matrix still hold a producer from the prior one;
  // authenticate that producer against its own historical descriptor.
  if(!previous[7] || previous[7]>d[6] || previous[29]!=5120 || w>=20)scalar_fault_park();
  uint32_t old[64],desc[64],job[16],proof[64],after[8];
  for(unsigned i=0;i<64;i++)old[i]=d[i];
  old[6]=previous[7];old[40]=previous[6];old[41]=previous[27];old[42]=previous[5];old[43]=previous[28];old[14]=previous[29];
  BfDescriptor(old,w,desc);
  Transfer(d,tracked,w,BFP::Descriptor,256,false);
  for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=desc[i])scalar_fault_park();
  Transfer(d,tracked,w,0x74000,64,false);for(unsigned i=0;i<16;i++)job[i]=Words(L::Scratch)[i];
  Transfer(d,tracked,w,0x83f00,256,false);for(unsigned i=0;i<64;i++)proof[i]=Words(L::Scratch)[i];
  for(unsigned i=0;i<8;i++){
   after[i]=Register(d,tracked,w,0xffb20000+Counter(i));
   if(after[i]!=Words(L::Counters)[w*8+i])scalar_fault_park();
  }
  if(!BFP::Proof(proof,job,desc,Boot(w)+16,w+1,M::Peer(w+1),proof,after))scalar_fault_park();
  for(unsigned i=0;i<10;i++)if(job[i]!=previous[16+i])scalar_fault_park();
  if(job[12]!=previous[1] || job[13]!=previous[2])scalar_fault_park();
  return true;
 }
#endif
 if(!M::FpuJob(previous))return false;
 Transfer(d,tracked,w,0x83f00,256,false);uint32_t proof[64],after[8];
 for(unsigned i=0;i<64;i++)proof[i]=Words(L::Scratch)[i];
 for(unsigned i=0;i<8;i++){
  after[i]=Register(d,tracked,w,0xffb20000+Counter(i));
  if(after[i]!=Words(L::Counters)[w*8+i])scalar_fault_park();
 }
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 if(!PioneerQ38HybridMatrixReceipt::Matrix(proof,previous,Boot(w),proof,after))scalar_fault_park();
#else
 if(!PioneerQ38Bf16MatrixReceipt::Matrix(proof,previous,Boot(w),proof,after))scalar_fault_park();
#endif
 // No counter or nonce rebase. ClosedJob authenticated the SAME real
 // completed job before this helper;the next worker rechecks its private
 // source and retained frontier before its first request.
 return true;
}
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
// Caller must authenticate the completed worker job and its receipt before
// publishing any of these values. This copies already-device-produced data;
// it performs no CPU-reference validation and grants no ownership.
static BF_ROOT_FN void BfStoreDenseMatrix(const uint32_t* d,unsigned w){
 if(!BfDenseQ8(d) || w>=
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  (
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
   BfSmallQ8(d)?2U:
#endif
   BfRecurrentQ8(d)?119U:PioneerQ38DenseQ8MatrixRootJobs::Active(d[43]))
#else
  PioneerQ38DenseQ8MatrixRootJobs::Active(d[43])
#endif
  )scalar_fault_park();
 Q38_DENSE_WORKER_SEAL(d,w);const unsigned first=Words(BfDenseScheduleIO::Parts+w*16)[2];
 const unsigned count=Words(BfDenseScheduleIO::Parts+w*16)[3]*32;
#ifdef PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION
 const unsigned publish=PioneerQ38DenseQ8MatrixRootFrame::PublishedRows(d[42],d[43],d[11],first,count);
 if(!publish)scalar_fault_park();
 for(unsigned i=0;i<count;i++)if((Words(L::Scratch)[64+i]&0x7f800000)==0x7f800000)scalar_fault_park();
 if(!PioneerQ38DenseQ8MatrixRootFrame::ZeroPadding(Words(L::Scratch)+64,publish,count))scalar_fault_park();
 Q38_DENSE_WORKER_SEAL(d,w);for(unsigned i=0;i<publish;i++)Words(L::Projected)[first+i]=Words(L::Scratch)[64+i];
#else
 if(!count || first>=d[11] || count>d[11]-first)scalar_fault_park();
 for(unsigned i=0;i<count;i++)if((Words(L::Scratch)[64+i]&0x7f800000)==0x7f800000)scalar_fault_park();
 Q38_DENSE_WORKER_SEAL(d,w);for(unsigned i=0;i<count;i++)Words(L::Projected)[first+i]=Words(L::Scratch)[64+i];
#endif
 Q38_DENSE_WORKER_SEAL(d,w);
}
#endif
static BF_ROOT_FN void BfGatherMatrix(const uint32_t* d,uint32_t* tracked,unsigned w){
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 if(BfDenseQ8(d))Q38_DENSE_WORKER_SEAL(d,w);else
#endif
 BF_PAIR_SEAL(d,w);
 MatrixWait(d,tracked,w);BootSeal(d,tracked,w);
 BfMatrixJob(d,w,Words(L::Nonces)[w]);Copy(L::PeerJobs+w*256,L::Scratch,64);ClosedJob(d,tracked,w,false);
 const unsigned groups=
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
  BfDenseQ8(d)?Words(BfDenseScheduleIO::Parts+w*16)[3]*2:
#endif
  Words(BFR::Parts+w*32)[2];
 Transfer(d,tracked,w,0x83f00,256+groups*64,false);uint32_t proof[64];for(unsigned i=0;i<64;i++)proof[i]=Words(L::Scratch)[i];
 uint32_t before[8],after[8];
#ifdef PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE
 for(unsigned i=0;i<8;i++)before[i]=Words(L::Counters)[w*8+i];
 BF_PAIR_SEAL(d,w);GatherCounters(d,tracked,w,after);BF_PAIR_SEAL(d,w);
#else
 for(unsigned i=0;i<8;i++){
  before[i]=Words(L::Counters)[w*8+i];after[i]=Register(d,tracked,w,0xffb20000+Counter(i));
 }
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 if(!PioneerQ38HybridMatrixReceipt::Matrix(proof,reinterpret_cast<const uint32_t*>(L::PeerJobs+w*256),Boot(w),before,after))scalar_fault_park();
#else
 if(!PioneerQ38Bf16MatrixReceipt::Matrix(proof,reinterpret_cast<const uint32_t*>(L::PeerJobs+w*256),Boot(w),before,after))scalar_fault_park();
#endif
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 if(BfDenseQ8(d))BfStoreDenseMatrix(d,w);else
#endif
 {
 const unsigned first=Words(BFR::Rows)[w];
 for(unsigned i=0;i<groups*16;i++){
  const unsigned bits=Words(L::Scratch)[64+i];if((bits&0x7f800000)==0x7f800000)scalar_fault_park();Words(L::Projected)[first+i]=bits;
 }
 }
 for(unsigned i=0;i<8;i++){
  Words(L::Counters)[w*8+i]=after[i];
 }
}
#undef BF_ROOT_FN
#undef BF_PAIR_SEAL
