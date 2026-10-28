/* SPDX-License-Identifier: MIT */
// Experimental persistent-token component ONLY. Current rows remain beyond
// the committed frontier until the real whole-token commit. A partial append
// faults the context; this routine supplies no rollback or native capability.
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND) || PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_LAYER_COMPONENT_DENSE) || PIONEER_Q38_LAYER_COMPONENT_DENSE != 1
#error Exact distinct returning dense token component required
#endif
namespace Joined {
static __attribute__((noinline,section(".conv.tokenkv"))) void AppendTokenKv(L::PrivateSeal* p){
 if(p->phase!=5 || p->outer[4]>=8192)scalar_fault_park();
 MetadataSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
 uint32_t tracked[8],expected[8],value[128];
 for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 if(tracked[2]!=tracked[6])scalar_fault_park();
 for(unsigned group=0;group<4;group++)for(unsigned which=0;which<2;which++){
  const auto* f=reinterpret_cast<const uint32_t*>(L::DenseCopies+group*24*256);
  if(!L::Child(p->frame,f,group,0))scalar_fault_park();
  FrameSeal(group*24);MetadataSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
  unsigned local=(which?L::NewV:L::NewK)+group*512;
  unsigned shadow=L::InputShadow+(which?26624:24576)+group*512;
  unsigned remote=f[8+which]+p->outer[4]*512,route=f[12+which];
  for(unsigned i=0;i<128;i++){
   value[i]=Words(local)[i];if(value[i]!=Words(shadow)[i])scalar_fault_park();Words(L::Scratch)[i]=value[i];
  }
  for(unsigned i=0;i<8;i++){
   if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
   expected[i]=tracked[i]+(i==5?8U:i==2 || i==4 || i==6 || i==7?1U:0U);
  }
  const unsigned command[]={L::Scratch,0,129,remote,0,route,0,0x212,512,0};
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
  Read(remote,route,L::Scratch,512,tracked);
  for(unsigned i=0;i<128;i++)
   if(Words(L::Scratch)[i]!=value[i] || Words(local)[i]!=value[i] || Words(shadow)[i]!=value[i])scalar_fault_park();
 }
 MetadataSeal();for(unsigned w=0;w<96;w++)FrameSeal(w);
 q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
}
}
