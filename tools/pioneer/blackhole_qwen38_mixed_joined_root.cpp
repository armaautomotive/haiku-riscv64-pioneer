/* SPDX-License-Identifier: MIT */
// NEW single persistent ROOT. No native/source/init/commit grant.
#include "blackhole_qwen38_mixed_projection_root.cpp"
#include "PioneerQ38DensePrepare.h"
extern "C" unsigned mixed_joined_wait(unsigned);
namespace Joined {
using namespace DensePartitionRoot;
static L::PrivateSeal* Private(){const L::PrivateSeal* p;asm volatile("mv %0,s11":"=r"(p));return const_cast<L::PrivateSeal*>(p);}
static void MetadataSeal(){
 for(unsigned slot=0;slot<4;slot++)for(unsigned i=0;i<L::RecipeBytes/4;i++)
  if(Words(L::Recipes+slot*L::RecipeStride)[i]!=Words(L::RecipeCopies+slot*L::RecipeStride)[i])scalar_fault_park();
 for(unsigned i=0;i<96*64;i++)if(Words(L::Frames)[i]!=Words(L::DenseCopies)[i])scalar_fault_park();
 for(unsigned i=0;i<5120;i++)if(Words(L::Raw)[i]!=Words(L::RawCopy)[i])scalar_fault_park();
}
static void DenseHeader(const uint32_t* o,uint32_t* d){
 for(unsigned i=0;i<64;i++)d[i]=0;
 const unsigned head[]={PioneerQ38MixedDenseRootLayout::Magic,1,64,o[3],o[4],129,o[6]+3,96,4,24,256,3,L::Frames,L::Shadow,L::Reports,L::Proofs};
 for(unsigned i=0;i<16;i++){d[i]=head[i];d[16+i]=o[16+i];}
 for(unsigned i=0;i<8;i++)d[32+i]=o[32+i];
 const unsigned fields[]={L::Input,L::Gate,L::Output,L::Accumulate,L::Scratch,12288,6144,7168,o[48],119,L::PeerBoots,L::PeerJobs,L::Nonces};
 for(unsigned i=0;i<13;i++)d[40+i]=fields[i];
 if(!L::Shape(d))scalar_fault_park();
}
static void Capture(const uint32_t* o){
 for(unsigned slot=0;slot<4;slot++){
  unsigned to=L::RecipeCopies+slot*L::RecipeStride;Copy(to,L::Recipes+slot*L::RecipeStride,L::RecipeBytes/4);
  const auto* d=reinterpret_cast<const uint32_t*>(to);const auto* h=d+64;const auto* parts=reinterpret_cast<const uint32_t(*)[8]>(d+80);
  const auto* rows=d+80+119*8;const auto* banks=rows+119;
  if(!PioneerQ38DenseProjectionRecipe(d,h,banks,parts,rows,false) || d[3]!=o[7] || d[4]!=o[9] || d[6]!=o[6]+slot
   || d[40]!=o[4] || d[41]!=o[52] || d[42]!=o[3] || d[43]!=(slot==3?10:5+slot))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(d[16+i]!=o[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(d[32+i]!=o[32+i])scalar_fault_park();
 }
 Copy(L::DenseCopies,L::Frames,96*64);uint32_t dense[64];DenseHeader(o,dense);
 for(unsigned group=0;group<4;group++)for(unsigned ordinal=0;ordinal<24;ordinal++){
  unsigned w=group*24+ordinal;
  const auto* f=reinterpret_cast<const uint32_t*>(L::DenseCopies+w*256);
  if(!L::Child(dense,f,group,ordinal))scalar_fault_park();
 }
 for(unsigned i=0;i<5120;i++){
  unsigned x=Words(L::Raw)[i];if((x&0x7fffffff)>0x477fe000)scalar_fault_park();Words(L::RawCopy)[i]=x;
 }
 MetadataSeal();
}
static void BindProjection(L::PrivateSeal* p,unsigned slot){
 MetadataSeal();const auto* s=Words(L::RecipeCopies+slot*L::RecipeStride);
 for(unsigned i=0;i<64;i++)p->frame[i]=s[i];
 auto* d=p->frame;
 d[0]=PioneerQ38MixedProjectionRootLayout::Magic;d[6]=p->outer[6]+(slot==3?4:slot);d[13]=L::Rows;
 d[48]=p->outer[48];d[49]=119;d[50]=L::Parts;d[51]=L::MatrixHeader;d[52]=L::Banks;
 d[53]=slot==3?L::Output:L::Input;d[54]=L::Projected;d[55]=L::PeerJobs;d[56]=L::Nonces;d[57]=L::PeerBoots;
 if(!PioneerQ38MixedProjectionRootLayout::Shape(d))scalar_fault_park();
 Copy(L::MatrixHeader,L::RecipeCopies+slot*L::RecipeStride+256,16);Words(L::MatrixHeader)[6]=d[6];
 Copy(L::Parts,L::RecipeCopies+slot*L::RecipeStride+320,119*8);
 Copy(L::Rows,L::RecipeCopies+slot*L::RecipeStride+4128,119);
 Copy(L::Banks,L::RecipeCopies+slot*L::RecipeStride+4604,8);
 for(unsigned i=0;i<64;i++)Words(L::Header)[i]=d[i];
 p->phase=slot==3?6:slot+1;Words(L::Status)[5]=p->phase;Fence();q38_mixed_root_seal();
}
static void Read(unsigned remote,unsigned route,unsigned destination,unsigned bytes,uint32_t* tracked){
 if(!bytes || bytes>16384 || ((remote|bytes)&63))scalar_fault_park();
 q38_mixed_root_seal();if(!Quiet())scalar_fault_park();uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  unsigned delta=i==1?bytes/64:i==0 || i==2 || i==3 || i==6?1:0;expected[i]=tracked[i]+delta;
 }
 const unsigned command[]={remote,0,route,L::Scratch,0,129,0,0,bytes,0};
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
 Copy(destination,L::Scratch,bytes/4);
}
static void Prepare(L::PrivateSeal* p){
 MetadataSeal();p->phase=4;Words(L::Status)[5]=4;uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 const auto* o=p->outer;
 Read(o[13],PioneerQ38ProjectionRoute(o[12]),0x83800,1024,tracked);
 Read(o[15],PioneerQ38ProjectionRoute(o[14]),0x83c00,1024,tracked);
 Read(o[40]+o[4]*256,o[41],0x84000,256,tracked);
 for(unsigned group=0;group<4;group++){
  Read(o[9]+TT_Q38_DENSE_QG+group*12288,o[8],0x80000,12288,tracked);
  Read(o[9]+TT_Q38_DENSE_K+group*1024,o[8],0x83000,1024,tracked);
  Read(o[9]+TT_Q38_DENSE_V+group*1024,o[8],0x83400,1024,tracked);
  if(!PioneerQ38DensePrepare::Run(Floats(0x80000),Floats(0x83000),Floats(0x83400),Floats(0x83800),Floats(0x83c00),Floats(0x84000),
   Floats(L::Input+group*6144),Floats(L::Gate+group*6144),reinterpret_cast<uint32_t*>(L::NewK+group*512),reinterpret_cast<uint32_t*>(L::NewV+group*512)))scalar_fault_park();
 }
 MetadataSeal();q38_mixed_root_seal();
}
static void BindDense(L::PrivateSeal* p){
 DenseHeader(p->outer,p->frame);Copy(L::Frames,L::DenseCopies,96*64);
 for(unsigned i=0;i<64;i++)Words(L::Header)[i]=p->frame[i];
 p->phase=5;Words(L::Status)[5]=5;Fence();q38_mixed_root_seal();
}
}
#if !defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) && !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
extern "C" void q38_mixed_root_seal(){
 using namespace DensePartitionRoot;const auto* p=Joined::Private();
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x70000-sizeof(*p))scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(L::Boot)[i]!=p->boot[i] || Words(L::Outer)[i]!=p->outer[i] || Words(L::Header)[i]!=p->frame[i])scalar_fault_park();
 if(!p->phase || p->phase>6 || *Words(L::Doorbell)!=p->outer[48])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=p->boot[16+i])scalar_fault_park();
}
extern "C" void mixed_joined_main(){
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
  unsigned n=mixed_joined_wait(previous);for(unsigned i=0;i<64;i++)seal.outer[i]=Words(L::Outer)[i];
  const auto* o=seal.outer;if(n!=previous+1 || n!=o[48] || !L::OuterShape(o))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(o[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(o[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  for(unsigned i=0;i<64;i++)Words(L::JointSummary)[i]=0;
  Words(L::Status)[0]=n;Words(L::Status)[4]=1;Joined::Capture(o);Copy(L::Input,L::RawCopy,5120);
  for(unsigned slot=0;slot<3;slot++){Joined::BindProjection(&seal,slot);if(Projection::Run(seal.frame)!=(slot?1024U:12288U))scalar_fault_park();}
  Joined::Prepare(&seal);Joined::BindDense(&seal);if(q38_joined_dense_frame()!=6144)scalar_fault_park();
  Joined::BindProjection(&seal,3);if(Projection::Run(seal.frame)!=5120)scalar_fault_park();
  Joined::MetadataSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
  for(unsigned i=0;i<8;i++){counters[i]=*Words(0xffb20000+Counter(i));Words(L::JointSummary)[8+i]=counters[i];}
  const unsigned result[]={n,o[3],o[4],5120,119,4,6144};for(unsigned i=0;i<7;i++)Words(L::JointSummary)[1+i]=result[i];
  Fence();Words(L::JointSummary)[0]=L::Magic;Fence();previous=n;
  Words(L::Status)[4]=0;Words(L::Status)[5]=6;Words(L::Status)[6]=o[3];Words(L::Status)[7]=o[4];Fence();Words(L::Status)[1]=n;Fence();
 }
}
#endif
