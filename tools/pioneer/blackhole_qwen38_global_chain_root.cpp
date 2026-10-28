/* SPDX-License-Identifier: MIT */
// ONE on-card control flow, not a host loop across standalone controllers.
#include "PioneerQ38GlobalChainImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerAttentionMath.h"
#include "PioneerQ38GlobalChainLayout.h"
#include "PioneerQ38GlobalHeadLayout.h"
#ifdef PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK
#include "PioneerQ38GlobalFeedbackLayout.h"
extern "C" bool q38_global_chain_feedback_decode(unsigned,unsigned);
#endif
extern "C" unsigned q38_return_recurrent_layer();
extern "C" unsigned q38_return_dense_layer();
extern "C" bool q38_global_chain_learned_rms(const float*,const float*,float*);
extern "C" bool q38_global_chain_quantize14(const float*,void*);
extern "C" void q38_global_chain_stopped() __attribute__((noreturn));
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace GlobalChain {
namespace L=PioneerQ38GlobalChainLayout;namespace R=L::R;namespace D=L::D;
struct Private {uint32_t boot[64],outer[128],state[64],phase;};
static_assert(sizeof(Private)==1028,"distinct owned parent,not a transplanted component type");
enum {Bank=0,Workspace=1,Layer=2,Completed=3,Profile=6,Counts=8};
static volatile uint32_t* Words(unsigned address){return reinterpret_cast<volatile uint32_t*>(address);}
static void Fence(){asm volatile("fence":::"memory");}
static __attribute__((noinline)) Private* Owner(){
 Private* p;asm volatile("mv %0,s11":"=r"(p));
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x70000-sizeof(*p))scalar_fault_park();
 return p;
}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Scope(bool counters=true){
 const auto* p=Owner();
 if(!p->phase || p->phase>4 || p->state[Bank]>=7 || p->state[Profile]!=64U
  || p->state[Layer]>=p->state[Profile] || p->state[Completed]>p->state[Profile] || !Quiet())scalar_fault_park();
 if(!L::LayerFrame(p->outer,p->state[Layer],p->state[Bank],p->state[Workspace],p->boot)
  || (p->phase==4?p->state[Completed]!=64 || p->state[Layer]!=63:p->phase==1?(p->state[Completed]?p->state[Completed]!=p->state[Layer]+1:p->state[Layer]!=0)
   :p->state[Completed]!=p->state[Layer]))scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(R::Boot)[i]!=p->boot[i])scalar_fault_park();
 for(unsigned i=0;i<128;i++)if(Words(R::Outer)[i]!=p->outer[i])scalar_fault_park();
 if(*Words(R::Doorbell)!=p->outer[48])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=p->boot[16+i])scalar_fault_park();
 if(counters)for(unsigned i=0;i<8;i++)if(*Words(0xffb20000+Counter(i))!=p->state[Counts+i])scalar_fault_park();
}
static void Transfer(unsigned remote,unsigned bytes,bool write){
 auto* p=Owner();unsigned workspace=p->state[Workspace];
 if(!bytes || bytes>16384 || ((remote|bytes)&63) || remote<workspace || remote>workspace+L::WorkspaceBytes-bytes)scalar_fault_park();
 Scope();uint32_t expected[8];
 for(unsigned i=0;i<8;i++)expected[i]=p->state[Counts+i]+(i==(write?5U:1U)?bytes/64:i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U);
 unsigned route=PioneerQ38ProjectionRoute(p->state[Bank]);
 const unsigned command[]={write?unsigned(R::Scratch):remote,0,write?129U:route,write?remote:unsigned(R::Scratch),0,write?route:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];if(done)break;
 }
 if(!done)scalar_fault_park();
 Scope(false);
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)p->state[Counts+i]=expected[i];
}
static void Stage(unsigned from,unsigned to,unsigned bytes,bool gddr){
 for(unsigned at=0;at<bytes;at+=16384){
  unsigned count=bytes-at;if(count>16384)count=16384;Transfer(from+at,count,false);
  if(gddr)Transfer(to+at,count,true);
  else for(unsigned i=0;i<count/4;i++)Words(to+at+i*4)[0]=Words(R::Scratch)[i];
 }
 Scope();
}
static void Metadata(unsigned layer,unsigned package){
 const auto* o=Owner()->outer;
 if(!L::Dense(layer)){
  Stage(package+L::P::RecipesOffset,o[57],L::P::RecipeBytes,true);
  Stage(package+L::P::ConvOffset,o[58],L::P::ConvBytes,true);
  Stage(package+L::P::GdnOffset,o[59],L::P::GdnBytes,true);
 }else{
  Stage(package+L::DenseRecipes,D::Recipes,4*D::RecipeStride,false);
  Stage(package+L::DenseFfn,o[68],3*D::RecipeStride,true);
  Stage(package+L::DenseFrames,D::Frames,96*256,false);
 }
}
#include "blackhole_qwen38_global_head_control.inc"
}
extern "C" void q38_global_chain_main(){
 using namespace GlobalChain;Private owned;uint32_t next[128];
 for(unsigned i=0;i<64;i++){owned.boot[i]=Words(R::Boot)[i];owned.state[i]=0;}
 for(unsigned i=0;i<128;i++)owned.outer[i]=Words(R::Outer)[i];
 owned.phase=1;owned.state[Bank]=owned.outer[7];owned.state[Workspace]=owned.outer[9];owned.state[Profile]=64U;
 if(!L::LayerFrame(owned.outer,0,owned.state[Bank],owned.state[Workspace],owned.boot) || *Words(R::Doorbell)!=1 || !Quiet())scalar_fault_park();
 asm volatile("mv s11,%0"::"r"(&owned):"s11","memory");Scope(false);
 for(unsigned i=0;i<8;i++)owned.state[Counts+i]=*Words(0xffb20000+Counter(i));
 if(owned.state[Counts+2]!=owned.state[Counts+6])scalar_fault_park();
 Words(R::Status)[3]=L::Magic;for(unsigned i=0;i<64;i++)Words(L::Progress)[i]=0;
#ifdef PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK
 for(unsigned i=0;i<64;i++)Words(PioneerQ38GlobalFeedbackLayout::Receipt)[i]=0;
#endif
 Words(L::Progress)[0]=L::Magic;Words(L::Progress)[1]=owned.state[Profile];
 for(unsigned layer=0;layer<64U;layer++){
  if(owned.state[Completed]!=layer || owned.phase!=1)scalar_fault_park();
  uint32_t package=0;if(!L::Address(owned.state[Workspace],layer,package))scalar_fault_park();
  Transfer(package,512,false);for(unsigned i=0;i<128;i++)next[i]=Words(R::Scratch)[i];
  if(!L::LayerFrame(next,layer,owned.state[Bank],owned.state[Workspace],owned.boot))scalar_fault_park();
  if(!layer)for(unsigned i=0;i<128;i++)if(next[i]!=owned.outer[i])scalar_fault_park();
  // A completed returning body is the ONLY predecessor. ROOT itself changes
  // the next frame and doorbell;there is no host metadata/product handoff.
  for(unsigned i=0;i<128;i++)Words(R::Outer)[i]=owned.outer[i]=next[i];
  owned.state[Layer]=layer;owned.phase=2;Fence();*Words(R::Doorbell)=layer+1;Fence();
  Words(R::Status)[5]=0x100+layer;Metadata(layer,package);Scope();owned.phase=3;
  const unsigned returned=L::Dense(layer)?q38_return_dense_layer():q38_return_recurrent_layer();
  // No caller-ready/unknown baseline recapture. This control point is reached
  // only by an actual successful pinned component call with all its private
  // source/metadata/quiet/worker proofs complete under the exclusive cohort.
  if(returned!=5120 || Owner()!=&owned || owned.phase!=3)scalar_fault_park();
  Scope(false);
  for(unsigned i=0;i<8;i++)owned.state[Counts+i]=*Words(0xffb20000+Counter(i));
  if(owned.state[Counts+2]!=owned.state[Counts+6])scalar_fault_park();
  owned.state[Completed]=layer+1;owned.phase=1;
  Words(L::Progress)[2]=layer+1;Words(L::Progress)[3]=layer;Fence();
 }
 if(owned.state[Completed]!=64 || owned.phase!=1 || owned.state[Layer]!=63)scalar_fault_park();
 owned.phase=4;Scope();RunHead();
 scalar_fault_park(); // own global STOP/candidate diagnostic never returns
}
