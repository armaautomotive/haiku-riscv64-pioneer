/* SPDX-License-Identifier: MIT */
// NEW complete dense decoder BLOCK; no native/source/commit/Work grant.
#include "blackhole_qwen38_mixed_joined_root.cpp"
#include "PioneerQ38StateMath.h"
extern "C" unsigned mixed_block_wait(unsigned);
namespace Block {
using namespace DensePartitionRoot;
static void OriginalSeal(){for(unsigned i=0;i<5120;i++)if(Words(L::Original)[i]!=Words(L::OriginalCopy)[i])scalar_fault_park();}
static void Norm(const uint32_t* o,unsigned role){
 uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 unsigned address=o[65+role*2],route=PioneerQ38ProjectionRoute(o[64+role*2]);
 Joined::Read(address,route,L::Weight,16384,tracked);Joined::Read(address+16384,route,L::Weight+16384,4096,tracked);
 if(!role){
  if(!PioneerQ38StateRms(Floats(L::OriginalCopy),Floats(L::Weight),Floats(L::Raw)))scalar_fault_park();
 }else if(!PioneerQ38StateResidualRms(Floats(L::Projected),Floats(L::OriginalCopy),Floats(L::Weight),Floats(L::PostResidual),Floats(L::PostNormalized)))scalar_fault_park();
 OriginalSeal();q38_mixed_root_seal();
}
static void FfnSeal(){
 Joined::MetadataSeal();OriginalSeal();
 for(unsigned i=0;i<3*L::RecipeStride/4;i++)if(Words(L::FfnRecipes)[i]!=Words(L::FfnCopies)[i])scalar_fault_park();
}
static void CaptureFfn(const uint32_t* o){
 uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 for(unsigned slot=0;slot<3;slot++)Joined::Read(o[68]+slot*L::RecipeStride,o[69],L::FfnRecipes+slot*L::RecipeStride,L::RecipeStride,tracked);
 Copy(L::FfnCopies,L::FfnRecipes,3*L::RecipeStride/4);
 for(unsigned slot=0;slot<3;slot++){
  const auto* d=reinterpret_cast<const uint32_t*>(L::FfnCopies+slot*L::RecipeStride);
  if(!PioneerQ38BlockFfnRecipe(d,d+64,d+1151,reinterpret_cast<const uint32_t(*)[8]>(d+80),d+1032)
   || d[3]!=o[7] || d[4]!=o[9] || d[6]!=o[6]+5+slot || d[40]!=o[4] || d[41]!=o[52] || d[42]!=o[3] || d[43]!=slot+2)scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(d[16+i]!=o[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(d[32+i]!=o[32+i])scalar_fault_park();
 }
 FfnSeal();
}
static void BindFfn(L::PrivateSeal* p,unsigned slot){
 FfnSeal();unsigned from=L::FfnCopies+slot*L::RecipeStride;const auto* s=Words(from);
 for(unsigned i=0;i<64;i++)p->frame[i]=s[i];
 auto* d=p->frame;d[0]=PioneerQ38MixedProjectionRootLayout::Magic;d[13]=L::Rows;
 d[48]=p->outer[48];d[49]=119;d[50]=L::Parts;d[51]=L::MatrixHeader;d[52]=L::Banks;
 d[53]=slot==2?unsigned(L::Input):unsigned(L::PostNormalized);d[54]=L::Projected;d[55]=L::PeerJobs;d[56]=L::Nonces;d[57]=L::PeerBoots;
 Copy(L::MatrixHeader,from+256,16);Copy(L::Parts,from+320,119*8);Copy(L::Rows,from+4128,119);Copy(L::Banks,from+4604,8);
 for(unsigned i=0;i<64;i++)Words(L::Header)[i]=d[i];
 p->phase=slot==2?11:8+slot;Words(L::Status)[5]=p->phase;Fence();q38_mixed_root_seal();
}
static void Activation(L::PrivateSeal* p){
 FfnSeal();p->phase=10;Words(L::Status)[5]=10;uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 const auto* o=p->outer;
 for(unsigned at=0;at<17408;at+=256){
  Joined::Read(o[9]+L::GateOffset+at*4,o[8],0x92000,1024,tracked);
  Joined::Read(o[9]+L::UpOffset+at*4,o[8],0x92400,1024,tracked);
  for(unsigned i=0;i<256;i++){
   float gate=Floats(0x92000)[i],up=Floats(0x92400)[i];
   if(!PioneerQ38RmsDomain(gate) || !PioneerQ38RmsDomain(up))scalar_fault_park();
   float value=(gate*Sigmoid(gate))*up;if(!PioneerQ38RmsDomain(value))scalar_fault_park();
   Words(L::Input)[at+i]=PioneerAttentionBits(value);
  }
 }
 FfnSeal();q38_mixed_root_seal();
}
static void WriteFinal(const uint32_t* o){
 uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 for(unsigned at=0;at<20480;at+=16384){
  unsigned bytes=20480-at;if(bytes>16384)bytes=16384;Copy(L::Scratch,L::Final+at,bytes/4);
  q38_mixed_root_seal();if(!Quiet())scalar_fault_park();uint32_t expected[8];
  for(unsigned i=0;i<8;i++){
   if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
   unsigned delta=i==5?bytes/64:i==2 || i==4 || i==6 || i==7?1:0;expected[i]=tracked[i]+delta;
  }
  const unsigned command[]={L::Scratch,0,129,o[9]+L::FinalOffset+at,0,o[8],0,0x212,bytes,0};
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
}
}
#ifdef PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS
extern "C" void q38_mixed_root_packet_seal(const uint32_t* frame){
 using namespace DensePartitionRoot;const auto* p=Joined::Private();
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x70000-sizeof(*p)
  || frame!=p->frame || !p->phase || p->phase>13 || !p->outer[48] || p->outer[48]>=0x7fffffff
  || *Words(L::Doorbell)!=p->outer[48])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(frame[16+i]!=p->boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(frame[32+i]!=p->boot[8+i])scalar_fault_park();
 PioneerQ38WordSealSources(p->boot+16);
} // no metadata-cache ready bit,NoC/counter recapture or new lifecycle grant
#endif
extern "C" void q38_mixed_root_seal(){
 using namespace DensePartitionRoot;const auto* p=Joined::Private();
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x70000-sizeof(*p))scalar_fault_park();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL
 PioneerQ38WordSealPairs<64>(Words(L::Boot),p->boot,Words(L::Header),p->frame);
 PioneerQ38WordSeal<L::OuterWords>(Words(L::Outer),p->outer);
#else
 for(unsigned i=0;i<64;i++)if(Words(L::Boot)[i]!=p->boot[i] || Words(L::Header)[i]!=p->frame[i])scalar_fault_park();
 for(unsigned i=0;i<L::OuterWords;i++)if(Words(L::Outer)[i]!=p->outer[i])scalar_fault_park();
#endif
 if(!p->phase || p->phase>
#ifdef PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE
 13
#else
 12
#endif
 || *Words(L::Doorbell)!=p->outer[48])scalar_fault_park();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL
 PioneerQ38WordSealSources(p->boot+16);
#else
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=p->boot[16+i])scalar_fault_park();
#endif
}
#ifndef PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE
extern "C" void mixed_block_main(){
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
  unsigned n=mixed_block_wait(previous);for(unsigned i=0;i<L::OuterWords;i++)seal.outer[i]=Words(L::Outer)[i];
  const auto* o=seal.outer;if(n!=previous+1 || n!=o[48] || !L::OuterShape(o))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(o[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(o[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  for(unsigned i=0;i<64;i++){Words(L::JointSummary)[i]=0;seal.frame[i]=Words(L::Header)[i]=0;}
  for(unsigned i=0;i<5120;i++){
   float value=Floats(L::Original)[i];if(!PioneerQ38StateDomain(value))scalar_fault_park();Words(L::OriginalCopy)[i]=PioneerAttentionBits(value);
  }
  seal.phase=1;Words(L::Status)[0]=n;Words(L::Status)[4]=1;Words(L::Status)[5]=1;Block::Norm(o,0);
  Joined::Capture(o);Copy(L::Input,L::RawCopy,5120);
  for(unsigned slot=0;slot<3;slot++){Joined::BindProjection(&seal,slot);if(Projection::Run(seal.frame)!=(slot?1024U:12288U))scalar_fault_park();}
  Joined::Prepare(&seal);Joined::BindDense(&seal);if(q38_joined_dense_frame()!=6144)scalar_fault_park();
  Joined::BindProjection(&seal,3);if(Projection::Run(seal.frame)!=5120)scalar_fault_park();
  seal.phase=7;Words(L::Status)[5]=7;Block::Norm(o,1);Block::CaptureFfn(o);
  for(unsigned slot=0;slot<2;slot++){Block::BindFfn(&seal,slot);if(Projection::Run(seal.frame)!=17408)scalar_fault_park();}
  Block::Activation(&seal);Block::BindFfn(&seal,2);if(Projection::Run(seal.frame)!=5120)scalar_fault_park();
  seal.phase=12;Words(L::Status)[5]=12;
  for(unsigned i=0;i<5120;i++){
   float a=Floats(L::Projected)[i],b=Floats(L::PostResidual)[i],value=a+b;
   if(!PioneerQ38StateDomain(a) || !PioneerQ38StateDomain(b) || !PioneerQ38StateDomain(value))scalar_fault_park();
   Words(L::Final)[i]=PioneerAttentionBits(value);
  }
  Block::FfnSeal();Block::WriteFinal(o);Block::FfnSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
  for(unsigned i=0;i<8;i++){counters[i]=*Words(0xffb20000+Counter(i));Words(L::JointSummary)[8+i]=counters[i];}
  const unsigned result[]={n,o[3],o[4],5120,119,7,17408};for(unsigned i=0;i<7;i++)Words(L::JointSummary)[1+i]=result[i];
  Fence();Words(L::JointSummary)[0]=L::Magic;Fence();previous=n;
  Words(L::Status)[4]=0;Words(L::Status)[5]=12;Words(L::Status)[6]=o[3];Words(L::Status)[7]=o[4];Fence();Words(L::Status)[1]=n;Fence();
 }
}
#endif
