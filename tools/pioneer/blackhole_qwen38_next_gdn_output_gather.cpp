/* SPDX-License-Identifier: MIT */
// OFFLINE distinct QOG2 ROOT gather;not selected by any native driver.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER)
#error Standalone offline QOG2 image never shares native/reference/Work roles
#endif
#if defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
#error Standalone QOG2 image never shares native producer or DEBUG roles
#endif
#include "PioneerQ38NextGdnOutputImageOnly.h"
#if !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE) || PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE != 1 || defined(PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE)
#error Exact standalone QOG2 helper role required
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38NextGdnOutputGatherShape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace NextGdnOutputGather {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool BaseSeal(const uint32_t* d){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]!=d[i])return false;
 return true;
}
static bool Seal(const uint32_t* d){
 if(!BaseSeal(d))return false;
 for(unsigned i=0;i<24*16;i++)if(Words(TT_Q38_NEXT_GDN_OUTPUT_GATHER_SOURCES)[i]!=Words(TT_Q38_NEXT_GDN_OUTPUT_GATHER_SHADOW)[i])return false;
 return true;
}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Packet(unsigned p,const uint32_t* d){
 if(p>=26 || !Seal(d) || !Quiet())scalar_fault_park();
 bool write=p==25;unsigned local=p<24?0x90e00:write?0xab000:0xa0000;
 unsigned remote=d[4]+(p<24?d[11]+p*128:write?d[12]:d[10]),bytes=p<24?128:write?768:12288;
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++){
  expected[i]=*Words(0xffb20000+Counter(i))+delta[i];if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:d[8],write?remote:local,0,write?d[8]:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){if(!Seal(d) || !Quiet())scalar_fault_park();Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;Fence();return;}
 }
 scalar_fault_park(); // UNKNOWN:no next packet/reset/restore/retry
}
}
extern "C" unsigned scalar_main(){
 using namespace NextGdnOutputGather;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38NextGdnOutputGatherShape(d))return 0;
 if(!BaseSeal(d) || !Quiet())scalar_fault_park();
 for(unsigned w=0;w<24;w++)if(!Words(TT_Q38_NEXT_GDN_OUTPUT_GATHER_SOURCES)[w*16])scalar_fault_park();
 for(unsigned i=0;i<24*16;i++)Words(TT_Q38_NEXT_GDN_OUTPUT_GATHER_SHADOW)[i]=Words(TT_Q38_NEXT_GDN_OUTPUT_GATHER_SOURCES)[i];
 Fence();if(!Seal(d) || !Quiet())scalar_fault_park();
 auto* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=12288;s[6]=d[6];s[7]=TT_Q38_NEXT_GDN_OUTPUT_GATHER_MARKER;s[24]=d[3];s[25]=d[4];s[26]=d[45];s[27]=d[41];
 s[28]=8;s[29]=768;s[30]=6144;s[31]=129;
 for(unsigned i=0;i<8;i++){s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && s[8+i]!=s[10])scalar_fault_park();}
 for(unsigned i=0;i<192;i++)Words(0xab000)[i]=0;
 for(unsigned chunk=0;chunk<24;chunk++){
  Packet(chunk,d);uint32_t line[32],peerSource[16];
  for(unsigned i=0;i<32;i++)line[i]=Words(0x90e00)[i];
  for(unsigned i=0;i<16;i++)peerSource[i]=Words(TT_Q38_NEXT_GDN_OUTPUT_GATHER_SHADOW)[chunk*16+i];
  if(!PioneerQ38NextGdnOutputGatherLine(line,d,chunk,peerSource))scalar_fault_park();
  for(unsigned i=0;i<8;i++)Words(0xab000)[chunk*8+i]=line[i];
 }
 // No operand read or compact sink BEFORE every line has fresh ancestry.
 Packet(24,d);Packet(25,d);if(!Seal(d) || !Quiet())scalar_fault_park();
 const unsigned delta[]={25,240,26,25,1,12,26,1};
 for(unsigned i=0;i<8;i++){
  s[16+i]=*Words(0xffb20000+Counter(i));if(uint32_t(s[16+i]-s[8+i])!=delta[i] || (i==6 && s[16+i]!=s[18]))scalar_fault_park();
 }
 s[4]=24;Fence();return 24;
}
