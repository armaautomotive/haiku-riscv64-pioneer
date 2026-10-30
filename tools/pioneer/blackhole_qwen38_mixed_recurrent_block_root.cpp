/* SPDX-License-Identifier: MIT */
// NEW complete recurrent hidden producer; only returning arithmetic is
// reused. No old image role, native owner, token or state-slot commit.
#include "blackhole_qwen38_mixed_projection_root.cpp"
#include "blackhole_qwen38_mixed_recurrent_cohort.cpp"
#include "PioneerQ38StateMath.h"
#ifdef PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE
#include "PioneerQ38RecurrentHistoryImageRole.h"
#include "PioneerQ38RecurrentHistoryLayout.h"
#endif
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
#include "PioneerQ38RecurrentPrefixImageRole.h"
#include "PioneerQ38RecurrentPrefixLayout.h"
#endif
extern "C" unsigned mixed_recurrent_block_wait(unsigned);
extern "C" void mixed_recurrent_block_stopped() __attribute__((noreturn));
namespace RecurrentBlock {
namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
namespace C=PioneerQ38MixedRecurrentCohortLayout;
using DensePartitionRoot::Words;using DensePartitionRoot::Floats;using DensePartitionRoot::Copy;
using DensePartitionRoot::Fence;using DensePartitionRoot::Counter;using DensePartitionRoot::Source;using DensePartitionRoot::Quiet;
static __attribute__((noinline)) R::PrivateSeal* Private(){
 R::PrivateSeal* p;asm volatile("mv %0,s11":"=r"(p));
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x70000-sizeof(*p))scalar_fault_park();
 return p;
}
static void Phase(R::PrivateSeal* p,unsigned phase){p->phase=phase;Words(R::Status)[5]=phase;Fence();q38_mixed_root_seal();}
static void OriginalSeal(){for(unsigned i=0;i<5120;i++)if(Words(R::Original)[i]!=Words(R::OriginalCopy)[i])scalar_fault_park();}
static void Transfer(unsigned remote,unsigned route,unsigned local,unsigned bytes,bool write,uint32_t* tracked){
 if(!bytes || bytes>16384 || ((remote|bytes)&63) || local<R::Scratch || local+bytes>R::Scratch+16384)scalar_fault_park();
 q38_mixed_root_seal();if(!Quiet())scalar_fault_park();uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  unsigned delta=i==(write?5U:1U)?bytes/64:i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U;
  expected[i]=tracked[i]+delta;if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
 const unsigned command[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];if(done)break;
 }
 if(!done)scalar_fault_park();
 q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
}
static void Read(unsigned remote,unsigned route,unsigned destination,unsigned bytes){
 uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 for(unsigned at=0;at<bytes;at+=16384){
  unsigned n=bytes-at;if(n>16384)n=16384;Transfer(remote+at,route,R::Scratch,n,false,tracked);Copy(destination+at,R::Scratch,n/4);
 }
}
static void Norm(R::PrivateSeal* p,unsigned which){
 const auto* o=p->outer;Read(o[65+which*2],PioneerQ38ProjectionRoute(o[64+which*2]),R::Weight,20480);
 bool okay=which?PioneerQ38StateResidualRms(Floats(R::Projected),Floats(R::OriginalCopy),Floats(R::Weight),Floats(R::PostResidual),Floats(R::PostNormalized)):
  PioneerQ38StateRms(Floats(R::OriginalCopy),Floats(R::Weight),Floats(R::Input));
 if(!okay)scalar_fault_park();
 OriginalSeal();q38_mixed_root_seal();
}
static void RecipeSeal(unsigned count){
 for(unsigned i=0;i<count*R::RecipeStride/4;i++)if(Words(R::Recipes)[i]!=Words(R::RecipeCopies)[i])scalar_fault_park();
 OriginalSeal();q38_mixed_root_seal();
}
static void CaptureRecipes(R::PrivateSeal* p,unsigned first,unsigned count){
 const auto* o=p->outer;
 for(unsigned slot=0;slot<count;slot++){
  Read(o[57]+(first+slot)*R::RecipeStride,o[8],R::Recipes+slot*R::RecipeStride,R::RecipeStride);
  Copy(R::RecipeCopies+slot*R::RecipeStride,R::Recipes+slot*R::RecipeStride,R::RecipeStride/4);
  const auto* r=reinterpret_cast<const uint32_t*>(R::RecipeCopies+slot*R::RecipeStride);
  if(!PioneerQ38MixedRecurrentRecipe(r,r+64,r+1151,reinterpret_cast<const uint32_t(*)[8]>(r+80),r+1032)
   || r[3]!=o[7] || r[4]!=o[9] || r[6]!=o[6]+R::PhaseDelta(first+slot) || r[40]!=o[4] || r[41]!=o[13]
   || r[42]!=o[3] || r[43]!=R::Role(first+slot))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(r[16+i]!=o[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(r[32+i]!=o[32+i])scalar_fault_park();
  for(unsigned i=1159;i<1280;i++)if(r[i])scalar_fault_park();
 }
 RecipeSeal(count);
}
static void Matrix(R::PrivateSeal* p,unsigned slot,unsigned count,unsigned phase){
 RecipeSeal(count);unsigned from=R::RecipeCopies+slot*R::RecipeStride;const auto* s=Words(from);
 for(unsigned i=0;i<64;i++)p->frame[i]=s[i];
 auto* d=p->frame;
 d[0]=PioneerQ38MixedRecurrentProjectionRootLayout::Magic;d[13]=R::Rows;d[48]=p->outer[48];d[49]=119;
 d[50]=R::Parts;d[51]=R::MatrixHeader;d[52]=R::Banks;
 d[53]=d[43]==13?R::Output:R::Input;
 d[54]=R::Projected;d[55]=R::PeerJobs;d[56]=R::Nonces;d[57]=R::PeerBoots;
 // The component shape fixes raw Input for gate/up. Copy real post-RMS,
 // never pass host products or weaken its established input grammar.
 if(d[43]==2 || d[43]==3){Copy(R::Input,R::PostNormalized,5120);d[53]=R::Input;}
 if(!PioneerQ38MixedRecurrentProjectionRootLayout::Shape(d))scalar_fault_park();
 Copy(R::MatrixHeader,from+256,16);Copy(R::Parts,from+320,119*8);Copy(R::Rows,from+4128,119);Copy(R::Banks,from+4604,8);
 for(unsigned i=0;i<64;i++)Words(R::Header)[i]=d[i];
 Phase(p,phase);
 unsigned n=d[11];if(Projection::Run(d)!=n)scalar_fault_park();RecipeSeal(count);
}
static void Cohort(R::PrivateSeal* p){
 const auto* o=p->outer;auto* d=p->frame;for(unsigned i=0;i<64;i++)d[i]=0;
 const unsigned head[]={C::Magic,1,64,o[3],o[4],o[6]+4,o[6]+5,o[7],o[9],o[8],129,119,96,640,o[12],o[13]};
 for(unsigned i=0;i<16;i++){d[i]=head[i];d[16+i]=o[16+i];}for(unsigned i=0;i<8;i++)d[32+i]=o[32+i];
 const unsigned tail[]={C::Conv,C::ConvShadow,C::Gdn,C::GdnShadow,C::ConvWords,C::GdnWords,C::PeerBoots,C::PeerJobs,o[48],C::Nonces,C::Counters,1};
 for(unsigned i=0;i<12;i++)d[40+i]=tail[i];
 if(!C::Shape(d))scalar_fault_park();
 for(unsigned i=0;i<64;i++)Words(R::Header)[i]=d[i];
 Phase(p,6);
 Read(o[58],o[8],C::Conv,C::ConvWords*4);Read(o[59],o[8],C::Gdn,C::GdnWords*4);
 if(q38_mixed_recurrent_cohort()!=6144)scalar_fault_park();
 OriginalSeal();q38_mixed_root_seal();
 // ALL cohort table/proof consumers have returned. Retire that phase
 // BEFORE gated F32/output/post-residual overwrite any shadow bytes.
 Phase(p,7);Read(o[9]+TT_Q38_GDN_GATED_OUTPUT,o[8],R::Output,6144*4);OriginalSeal();
}
static void Activation(R::PrivateSeal* p){
 Phase(p,12);const auto* o=p->outer;
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
 (void)o; // exact down recipe dispatches/consumes the actual worker producer
#else
 for(unsigned at=0;at<17408;at+=256){
  Read(o[9]+PioneerQ38MixedRecurrentOffset(2)+at*4,o[8],0x92000,1024);
  Read(o[9]+PioneerQ38MixedRecurrentOffset(3)+at*4,o[8],0x92400,1024);
  for(unsigned i=0;i<256;i++){
   float a=Floats(0x92000)[i],b=Floats(0x92400)[i];if(!PioneerQ38RmsDomain(a) || !PioneerQ38RmsDomain(b))scalar_fault_park();
   float value=(a*DensePartitionRoot::Sigmoid(a))*b;if(!PioneerQ38RmsDomain(value))scalar_fault_park();Words(R::Input)[at+i]=PioneerAttentionBits(value);
  }
 }
#endif
 OriginalSeal();q38_mixed_root_seal();
}
#ifdef PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE
static void StageHistory(R::PrivateSeal* p){
 namespace H=PioneerQ38RecurrentHistoryLayout;
 static_assert(H::Offset>=R::GdnMetaOffset+C::GdnWords*4,"history slots cannot overwrite immutable layer recipes");
 uint32_t destination=0;const auto* o=p->outer;
 if(!H::Address(o[9],o[3],destination))scalar_fault_park();
 uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 // Preserve actual Conv-generated bytes ON CARD before the next layer
 // overwrites common NEXT scratch. No host buffer/readback/numeric work.
 for(unsigned at=0;at<H::Bytes;at+=16384){
  unsigned n=H::Bytes-at;if(n>16384)n=16384;
  Transfer(o[9]+TT_Q38_CONV_NEXT+at,o[8],R::Scratch,n,false,tracked);
  Transfer(destination+at,o[8],R::Scratch,n,true,tracked);
 }
 OriginalSeal();q38_mixed_root_seal();
}
#endif
static void Finish(R::PrivateSeal* p){
 Phase(p,14);
#ifdef PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE
 StageHistory(p);
#endif
 for(unsigned i=0;i<5120;i++){
  float a=Floats(R::Projected)[i],b=Floats(R::PostResidual)[i],v=a+b;
  if(!PioneerQ38StateDomain(a) || !PioneerQ38StateDomain(b) || !PioneerQ38StateDomain(v))scalar_fault_park();
  Words(R::Final)[i]=PioneerAttentionBits(v);
 }
 OriginalSeal();uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 for(unsigned at=0;at<20480;at+=16384){unsigned n=20480-at;if(n>16384)n=16384;Copy(R::Scratch,R::Final+at,n/4);
  Transfer(p->outer[9]+R::FinalOffset+at,p->outer[8],R::Scratch,n,true,tracked);}
 OriginalSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
}
static void EndSeal(R::PrivateSeal* p){
 RecipeSeal(3);Projection::InputsSeal(p->frame);
 for(unsigned w=0;w<119;w++)DensePartitionRoot::FrameSeal(w);
}
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
static void PrefixMetadata(R::PrivateSeal* p,unsigned bank,uint32_t workspace){
 namespace P=PioneerQ38RecurrentPrefixLayout;
 const auto* o=p->outer;uint32_t package=0,copy[128];
 if(!P::Frame(o,o[3],bank,workspace,p->boot) || !P::Address(workspace,o[3],package))scalar_fault_park();
 Read(package,o[8],reinterpret_cast<unsigned>(copy),512);
 for(unsigned i=0;i<128;i++)if(copy[i]!=o[i])scalar_fault_park();
 const unsigned regions[][3]={{P::RecipesOffset,o[57],P::RecipeBytes},{P::ConvOffset,o[58],P::ConvBytes},{P::GdnOffset,o[59],P::GdnBytes}};
 uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 for(const auto& region:regions)for(unsigned at=0;at<region[2];at+=16384){
  unsigned bytes=region[2]-at;if(bytes>16384)bytes=16384;
  Transfer(package+region[0]+at,o[8],R::Scratch,bytes,false,tracked);
  Transfer(region[1]+at,o[8],R::Scratch,bytes,true,tracked);
 }
 // The private frame and exact original outer stay sealed throughout all
 // copies. Ordinary recipe/cohort consumers still validate the loaded DATA.
 q38_mixed_root_seal();
}
static void PrefixNext(R::PrivateSeal* p,unsigned completed,unsigned bank,uint32_t workspace){
 namespace P=PioneerQ38RecurrentPrefixLayout;
 if(!completed || completed>P::Layers || !P::Frame(p->outer,completed-1,bank,workspace,p->boot))scalar_fault_park();
 EndSeal(p);Phase(p,16);uint32_t next[128];
 if(completed<P::Layers){
  uint32_t package=0;if(!P::Address(workspace,completed,package))scalar_fault_park();
  Read(package,p->outer[8],reinterpret_cast<unsigned>(next),512);
  if(!P::Frame(next,completed,bank,workspace,p->boot))scalar_fault_park();
 }else{
  for(unsigned i=0;i<128;i++)next[i]=0;
  next[0]=R::Magic;next[1]=1;next[2]=128;next[5]=129;next[48]=completed+1;next[49]=119;next[72]=R::Stop;
  for(unsigned i=0;i<16;i++)next[16+i]=p->boot[16+i];
  for(unsigned i=0;i<8;i++)next[32+i]=p->boot[8+i];
  if(!R::StopShape(next))scalar_fault_park();
 }
 // ROOT itself stages and releases the next exact frame. There is no host
 // metadata write, hidden readback, tensor product or doorbell between layers.
 for(unsigned i=0;i<128;i++)Words(R::Outer)[i]=p->outer[i]=next[i];
 Fence();*Words(R::Doorbell)=next[48];Fence();q38_mixed_root_seal();
}
#endif
static void StopJob(const uint32_t* d,unsigned w,unsigned n){
 namespace M=PioneerQ38MixedWorkerLayout;
 for(unsigned i=0;i<64;i++)Words(R::Scratch)[i]=0;
 const unsigned head[]={M::Magic,n,unsigned(M::Stop),w+1,M::Peer(w+1),d[42],d[40],d[6]+1};
 for(unsigned i=0;i<8;i++){Words(R::Scratch)[i]=head[i];Words(R::Scratch)[8+i]=d[32+i];}
}
static void Stopped(const uint32_t* d,uint32_t* tracked,unsigned w){
 using namespace DensePartitionRoot;
 const unsigned n=Words(R::Nonces)[w];bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  DensePartitionRoot::Transfer(d,tracked,w,M::Status,64,false);
  uint32_t s[16];for(unsigned i=0;i<16;i++)s[i]=Words(R::Scratch)[i];
  if(s[2] || s[3]!=M::Magic || s[0]<n-1 || s[0]>n || s[1]<n-1 || s[1]>s[0] || (s[4] && s[4]!=M::Stop)
   || s[5] || s[6]!=d[42] || s[7]!=d[40])scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  if(s[0]!=n || s[1]!=n || s[4]!=M::Stop)continue;
  unsigned a=Register(d,tracked,w,0xffb13140);
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  unsigned b=Register(d,tracked,w,0xffb13140);
  // A speculative frontend PC is never completion by itself. Require the
  // actual STOP record AND two positive terminal samples,not a range guess.
  if((a!=0x20060 && a!=0x20064) || (b!=0x20060 && b!=0x20064))continue;
  done=true;break;
 }
 if(!done)scalar_fault_park();
 BootSeal(d,tracked,w);DensePartitionRoot::Transfer(d,tracked,w,M::Job,256,false);
 for(unsigned i=0;i<64;i++)if(Words(R::Scratch)[i]!=Words(R::PeerJobs+w*256)[i])scalar_fault_park();
 if(Register(d,tracked,w,M::Doorbell)!=n)scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(R::Counters)[w*8+i])scalar_fault_park();
}
static void Stop(R::PrivateSeal* p,unsigned previous){
 using namespace DensePartitionRoot;
 const auto* d=p->frame;
 if(!PioneerQ38MixedRecurrentProjectionRootLayout::Shape(d) || d[43]!=4 || d[48]!=previous)scalar_fault_park();
 Phase(p,15);Words(R::Status)[0]=p->outer[48];Words(R::Status)[4]=2;EndSeal(p);
 uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 // Prove every existing closed chain BEFORE the first peer STOP write.
 for(unsigned w=0;w<119;w++){
  if(Words(R::Nonces)[w]>=0x7ffffffe)scalar_fault_park();
  Wait(d,tracked,w,0);BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
  for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(R::Counters)[w*8+i])scalar_fault_park();
 }
 for(unsigned w=0;w<119;w++){
  StopJob(d,w,Words(R::Nonces)[w]+1);Copy(R::PeerJobs+w*256,R::Scratch,64);
  DensePartitionRoot::Transfer(d,tracked,w,M::Job,256,true);
 }
 for(unsigned w=0;w<119;w++){
  *Words(R::Scratch)=Words(R::Nonces)[w]+1;
  DensePartitionRoot::Transfer(d,tracked,w,M::Doorbell,4,true);Words(R::Nonces)[w]++;
 }
 for(unsigned w=0;w<119;w++)Stopped(d,tracked,w);
 EndSeal(p);if(!Quiet())scalar_fault_park();
 auto* s=Words(R::StopSummary);for(unsigned i=0;i<64;i++)s[i]=0;
 const unsigned head[]={0,p->outer[48],previous,119,d[42],d[40],129,1};
 for(unsigned i=0;i<8;i++){s[i]=head[i];s[8+i]=tracked[i];s[26+i]=d[32+i];}
 for(unsigned i=0;i<10;i++)s[16+i]=Words(0xffb20000)[i];
 for(unsigned i=0;i<16;i++)s[34+i]=p->boot[16+i];
 Fence();s[0]=R::StopMagic;Fence();Words(R::Status)[1]=p->outer[48];Fence();mixed_recurrent_block_stopped();
}
}
extern "C" const uint32_t* q38_mixed_recurrent_frame(){return RecurrentBlock::Private()->frame;}
#ifdef PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS
extern "C" void q38_mixed_root_packet_seal(const uint32_t* frame){
 using namespace RecurrentBlock;const auto* p=Private();
 if(frame!=p->frame || !p->phase || p->phase>15 || !p->outer[48] || p->outer[48]>=0x7fffffff
  || *Words(R::Doorbell)!=p->outer[48])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(frame[16+i]!=p->boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(frame[32+i]!=p->boot[8+i])scalar_fault_park();
 PioneerQ38WordSealSources(p->boot+16);
} // exclusive owner per-packet checks,NOT a full immutable metadata audit
#endif
extern "C" void q38_mixed_root_seal(){
 using namespace RecurrentBlock;const auto* p=Private();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL
 PioneerQ38WordSealPairs<64>(Words(R::Boot),p->boot,Words(R::Header),p->frame);
 PioneerQ38WordSeal<128>(Words(R::Outer),p->outer);
#else
 for(unsigned i=0;i<64;i++)if(Words(R::Boot)[i]!=p->boot[i] || Words(R::Header)[i]!=p->frame[i])scalar_fault_park();
 for(unsigned i=0;i<128;i++)if(Words(R::Outer)[i]!=p->outer[i])scalar_fault_park();
#endif
 if(!p->phase || p->phase>
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
  16
#else
  15
#endif
  || (p->phase==15 && !R::StopShape(p->outer)) || *Words(R::Doorbell)!=p->outer[48])scalar_fault_park();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL
 PioneerQ38WordSealSources(p->boot+16);
#else
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=p->boot[16+i])scalar_fault_park();
#endif
}
extern "C" void mixed_recurrent_block_main(){
 using namespace RecurrentBlock;R::PrivateSeal seal;uint32_t counters[8];
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(R::Boot)[i];
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
 namespace P=PioneerQ38RecurrentPrefixLayout;unsigned prefixBank=0;uint32_t prefixWorkspace=0;
 if(!P::BootShape(seal.boot) || *Words(R::Doorbell) || !Quiet())scalar_fault_park();
#else
 if(!R::BootShape(seal.boot) || *Words(R::Doorbell) || !Quiet())scalar_fault_park();
#endif
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=seal.boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
 if(counters[2]!=counters[6])scalar_fault_park();
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 for(unsigned i=0;i<16;i++)Words(R::Status)[i]=0;
 for(unsigned i=0;i<64;i++)Words(R::StopSummary)[i]=0;
 Words(R::Status)[3]=R::Magic;Fence();unsigned previous=0;
 for(;;){
  unsigned n=mixed_recurrent_block_wait(previous);for(unsigned i=0;i<128;i++)seal.outer[i]=Words(R::Outer)[i];
  const auto* o=seal.outer;bool stop=o[72]==R::Stop;
  if(n!=previous+1 || o[48]!=n || (stop?!previous || !R::StopShape(o):!R::OuterShape(o)))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(o[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(o[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  if(stop)Stop(&seal,previous);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
  if(!previous){prefixBank=o[7];prefixWorkspace=o[9];}
  if(!P::Frame(o,previous,prefixBank,prefixWorkspace,seal.boot))scalar_fault_park();
#endif
  for(unsigned i=0;i<64;i++){seal.frame[i]=Words(R::Header)[i]=0;Words(R::JointSummary)[i]=0;}
  Words(R::Status)[0]=n;Words(R::Status)[4]=1;Phase(&seal,1);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
  PrefixMetadata(&seal,prefixBank,prefixWorkspace);
#endif
  Read(o[69],PioneerQ38ProjectionRoute(o[68]),R::Original,20480);Copy(R::OriginalCopy,R::Original,5120);Norm(&seal,0);CaptureRecipes(&seal,0,4);
  for(unsigned i=0;i<4;i++)Matrix(&seal,i,4,2+i);
  Cohort(&seal);CaptureRecipes(&seal,4,1);Matrix(&seal,0,1,8);
  Phase(&seal,9);Norm(&seal,1);CaptureRecipes(&seal,5,3);Matrix(&seal,0,3,10);Matrix(&seal,1,3,11);
  Activation(&seal);Matrix(&seal,2,3,13);Finish(&seal);
  auto* s=Words(R::JointSummary);for(unsigned i=0;i<64;i++)s[i]=0;
  const unsigned result[]={R::Magic,n,o[3],o[4],5120,119,8,640,96,6144,14,o[12]};
  for(unsigned i=1;i<12;i++)s[i]=result[i];
  for(unsigned i=0;i<8;i++){counters[i]=*Words(0xffb20000+Counter(i));s[16+i]=counters[i];}
  for(unsigned i=0;i<16;i++)s[32+i]=o[16+i];
  for(unsigned i=0;i<8;i++)s[48+i]=o[32+i];
#ifdef PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE
  namespace H=PioneerQ38RecurrentHistoryLayout;uint32_t history=0;
  if(!H::Address(o[9],o[3],history))scalar_fault_park();
  const unsigned staged[]={H::Marker,history,H::Bytes,n,o[3],o[4],o[12],1};
  for(unsigned i=0;i<8;i++)s[56+i]=staged[i]; // provisional per-layer receipt,NEVER whole-token commit
#endif
  Fence();s[0]=R::Magic;Fence();previous=n;Words(R::Status)[4]=0;Words(R::Status)[6]=o[3];Words(R::Status)[7]=o[4];Fence();Words(R::Status)[1]=n;Fence();
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
  PrefixNext(&seal,n,prefixBank,prefixWorkspace);
  for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
#endif
 }
}
