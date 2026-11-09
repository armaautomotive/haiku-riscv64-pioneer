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
static bool BfDenseQ8(const uint32_t* d){return d[15]==8 && d[42]<64 && (d[42]&3)==3 && d[43]>=5 && d[43]<=7;}
// Experimental ROOT-local metadata only. The native owner must still
// authenticate, upload and preserve this table and its BFP8 weights.
struct BfDenseScheduleIO {
 enum {Manifest=0x16dc00,ManifestWords=16*3*7*3,Parts=0x16ebc0,Shadow=0x16f330,TableShadow=0x16fac0,End=0x16fb20};
 unsigned index;
 unsigned Table(unsigned bank,unsigned field){return Words(Manifest)[index+bank*3+field];}
 void Output(unsigned worker,unsigned field,unsigned value){Words(Parts+worker*16)[field]=value;Words(Shadow+worker*16)[field]=value;}
};
static_assert(BfDenseScheduleIO::Manifest>=0x16da00+7*4
 && BfDenseScheduleIO::Manifest+BfDenseScheduleIO::ManifestWords*4==BfDenseScheduleIO::Parts
 && BfDenseScheduleIO::Parts+119*16<=BfDenseScheduleIO::Shadow
 && BfDenseScheduleIO::Shadow+119*16<=BfDenseScheduleIO::TableShadow
 && BfDenseScheduleIO::TableShadow+21*4<=BfDenseScheduleIO::End && BfDenseScheduleIO::End<=unsigned(L::PeerBoots),
 "isolated dense BFP8 schedule metadata must not overlap the FFN table or peer boots");
static BF_ROOT_FN void BfDenseSeal(const uint32_t* d){
 if(!BfDenseQ8(d))scalar_fault_park();
 const unsigned index=((d[42]/4)*3+d[43]-5)*21;
 for(unsigned i=0;i<21;i++)if(Words(BfDenseScheduleIO::Manifest)[index+i]!=Words(BfDenseScheduleIO::TableShadow)[i])scalar_fault_park();
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)
  if(Words(BfDenseScheduleIO::Parts+w*16)[i]!=Words(BfDenseScheduleIO::Shadow+w*16)[i])scalar_fault_park();
}
static BF_ROOT_FN void BfBuildDenseSchedule(const uint32_t* d){
 if(!BfDenseQ8(d))scalar_fault_park();
 BfDenseScheduleIO io;io.index=((d[42]/4)*3+d[43]-5)*21;
 for(unsigned i=0;i<21;i++)Words(BfDenseScheduleIO::TableShadow)[i]=Words(BfDenseScheduleIO::Manifest)[io.index+i];
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)io.Output(w,i,0);
 Fence();if(!PioneerQ38DenseQ8MatrixRootJobs::Build(io,d[42],d[43]))scalar_fault_park();
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
static BF_ROOT_FN void BfBuildPairs(const uint32_t* d){
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 if(BfDenseQ8(d)){BfBuildDenseSchedule(d);return;}
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
 const bool gated=d[43]==4;const unsigned chunks=gated?68:20;
 if(!BfProducerRole(d) || w>=chunks)scalar_fault_park();
 for(unsigned i=0;i<64;i++)out[i]=0;
 out[0]=BFP::Magic;out[1]=1;out[2]=67108864;out[3]=d[3];out[4]=d[4];
 out[5]=w;out[6]=d[6];out[7]=w+1;out[8]=d[8];out[9]=M::Peer(w+1);
 out[10]=(gated?unsigned(BFP::GateOffset):unsigned(BFP::ProjectionInput))+w*1024;
 out[11]=gated?BFP::UpOffset+w*1024:5120;
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
 if(!BfProducerRole(d) || (!native && !trailer) || !bytes || bytes>16384 || ((offset|bytes)&63))scalar_fault_park();
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
static BF_ROOT_FN void BfPrepare(const uint32_t* d
#ifdef PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE
 ,PioneerQ38Bf16DeferredAudit& audit
#endif
 ){
 Seal(d);BfBuildPairs(d);uint32_t tracked[8],desc[64],job[64];
 static_assert(unsigned(L::InputShadow)==unsigned(BFP::ProjectionInput) && unsigned(L::Native)==0xa0000,"actual QGW4 ROOT buffers");
 if(d[43]!=4)for(unsigned i=0;i<5120;i++){
  const unsigned bits=Words(d[53])[i];if((bits&0x7fffffff)>0x477fe000)scalar_fault_park();Words(L::InputShadow)[i]=bits;
 }
 for(unsigned i=0;i<8;i++){tracked[i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();}
 for(unsigned w=0;w<119;w++)Admit(d,tracked,w);
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
 if(d[43]!=4)for(unsigned i=0;i<5120;i++)if(Words(d[53])[i]!=Words(L::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<d[14]/2;i++)if(Words(L::Native)[i]!=Words(L::NativeShadow)[i])scalar_fault_park();
}
static BF_ROOT_FN void BfMatrixJob(const uint32_t* d,unsigned w,unsigned n){
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
static BF_ROOT_FN void BfGatherMatrix(const uint32_t* d,uint32_t* tracked,unsigned w){
 BF_PAIR_SEAL(d,w);MatrixWait(d,tracked,w);BootSeal(d,tracked,w);
 BfMatrixJob(d,w,Words(L::Nonces)[w]);Copy(L::PeerJobs+w*256,L::Scratch,64);ClosedJob(d,tracked,w,false);
 const unsigned groups=Words(BFR::Parts+w*32)[2];
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
 const unsigned first=Words(BFR::Rows)[w];
 for(unsigned i=0;i<groups*16;i++){
  const unsigned bits=Words(L::Scratch)[64+i];if((bits&0x7f800000)==0x7f800000)scalar_fault_park();Words(L::Projected)[first+i]=bits;
 }
 for(unsigned i=0;i<8;i++){
  Words(L::Counters)[w*8+i]=after[i];
 }
}
#undef BF_ROOT_FN
#undef BF_PAIR_SEAL
