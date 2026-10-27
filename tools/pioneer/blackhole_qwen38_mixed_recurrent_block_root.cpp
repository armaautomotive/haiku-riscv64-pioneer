/* SPDX-License-Identifier: MIT */
// NEW complete recurrent hidden producer; only returning arithmetic is
// reused. No old image role, native owner, token or state-slot commit.
#include "blackhole_qwen38_mixed_projection_root.cpp"
#include "blackhole_qwen38_mixed_recurrent_cohort.cpp"
#include "PioneerQ38ResidualRms.h"
extern "C" unsigned mixed_recurrent_block_wait(unsigned);
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
 bool okay=which?PioneerQ38ResidualRms(Floats(R::Projected),Floats(R::OriginalCopy),Floats(R::Weight),Floats(R::PostResidual),Floats(R::PostNormalized)):
  PioneerQ38LearnedRms(Floats(R::OriginalCopy),Floats(R::Weight),Floats(R::Input));
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
 for(unsigned at=0;at<17408;at+=256){
  Read(o[9]+PioneerQ38MixedRecurrentOffset(2)+at*4,o[8],0x92000,1024);
  Read(o[9]+PioneerQ38MixedRecurrentOffset(3)+at*4,o[8],0x92400,1024);
  for(unsigned i=0;i<256;i++){
   float a=Floats(0x92000)[i],b=Floats(0x92400)[i];if(!PioneerQ38RmsDomain(a) || !PioneerQ38RmsDomain(b))scalar_fault_park();
   float value=(a*DensePartitionRoot::Sigmoid(a))*b;if(!PioneerQ38RmsDomain(value))scalar_fault_park();Words(R::Input)[at+i]=PioneerAttentionBits(value);
  }
 }
 OriginalSeal();q38_mixed_root_seal();
}
static void Finish(R::PrivateSeal* p){
 Phase(p,14);for(unsigned i=0;i<5120;i++){
  float a=Floats(R::Projected)[i],b=Floats(R::PostResidual)[i],v=a+b;
  if(!PioneerQ38RmsDomain(a) || !PioneerQ38RmsDomain(b) || !PioneerQ38RmsDomain(v))scalar_fault_park();
  Words(R::Final)[i]=PioneerAttentionBits(v);
 }
 OriginalSeal();uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 for(unsigned at=0;at<20480;at+=16384){unsigned n=20480-at;if(n>16384)n=16384;Copy(R::Scratch,R::Final+at,n/4);
  Transfer(p->outer[9]+R::FinalOffset+at,p->outer[8],R::Scratch,n,true,tracked);}
 OriginalSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
}
}
extern "C" const uint32_t* q38_mixed_recurrent_frame(){return RecurrentBlock::Private()->frame;}
extern "C" void q38_mixed_root_seal(){
 using namespace RecurrentBlock;const auto* p=Private();
 for(unsigned i=0;i<64;i++)if(Words(R::Boot)[i]!=p->boot[i] || Words(R::Header)[i]!=p->frame[i])scalar_fault_park();
 for(unsigned i=0;i<128;i++)if(Words(R::Outer)[i]!=p->outer[i])scalar_fault_park();
 if(!p->phase || p->phase>14 || *Words(R::Doorbell)!=p->outer[48])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=p->boot[16+i])scalar_fault_park();
}
extern "C" void mixed_recurrent_block_main(){
 using namespace RecurrentBlock;R::PrivateSeal seal;uint32_t counters[8];
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(R::Boot)[i];
 if(!R::BootShape(seal.boot) || *Words(R::Doorbell) || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=seal.boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
 if(counters[2]!=counters[6])scalar_fault_park();
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 for(unsigned i=0;i<16;i++)Words(R::Status)[i]=0;
 Words(R::Status)[3]=R::Magic;Fence();unsigned previous=0;
 for(;;){
  unsigned n=mixed_recurrent_block_wait(previous);for(unsigned i=0;i<128;i++)seal.outer[i]=Words(R::Outer)[i];
  const auto* o=seal.outer;if(n!=previous+1 || o[48]!=n || !R::OuterShape(o))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(o[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(o[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  for(unsigned i=0;i<64;i++){seal.frame[i]=Words(R::Header)[i]=0;Words(R::JointSummary)[i]=0;}
  Words(R::Status)[0]=n;Words(R::Status)[4]=1;Phase(&seal,1);
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
  Fence();s[0]=R::Magic;Fence();previous=n;Words(R::Status)[4]=0;Words(R::Status)[6]=o[3];Words(R::Status)[7]=o[4];Fence();Words(R::Status)[1]=n;Fence();
 }
}
