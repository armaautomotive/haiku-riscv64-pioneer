/* SPDX-License-Identifier: MIT */
// Distinct OFFLINE QRR2:nonzero recurrent-layer result+raw -> learned RMS.
// CPU DATA/shape is NOT genuine producer ownership or a native launch grant.
#if !defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE) || PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE != 1 || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || defined(PIONEER_Q38_NEXT_GDN_NATIVE) || defined(PIONEER_Q38_NEXT_CONV_NATIVE) || defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || defined(PIONEER_Q38_NEXT_RMS_NATIVE) || defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_IMAGE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(_KERNEL_MODE)
#error Separate exact OFFLINE QRR2 image only,never native/CPU/Work/old image
#endif
#if defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_DECODER_RUNTIME) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE) || defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT)
#error Native/client roles never enter the QRR2 device image
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38ResidualRms.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38NextResidualRmsShape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace NextResidualRms {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Seal(const uint32_t* d){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]!=d[i])return false;
 return true;
}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Packet(unsigned index,const uint32_t* d){
 if(index>=10 || !Seal(d) || !Quiet())scalar_fault_park();
 bool write=index>=6,learned=index>=4 && index<6,raw=index>=2 && index<4,normalized=index>=8;
 unsigned offset=(index&1)*16384,bytes=index&1?4096:16384;
 unsigned local=(write?(normalized?0xa0000:0x98000):learned?0x90000:raw?0x88000:0x80000)+offset;
 unsigned remote=(learned?d[11]:d[4])+(learned?0U:write?(normalized?0U:unsigned(TT_Q38_RMS_BACKUP)):raw?unsigned(TT_Q38_RMS_BACKUP):unsigned(TT_Q38_NEXT_GDN_OUTPUT_RESULT))+offset;
 unsigned route=learned?d[12]:d[8];
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++){expected[i]=*Words(0xffb20000+Counter(i))+delta[i];if(i==6 && expected[6]!=expected[2])scalar_fault_park();}
 const uint32_t command[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Seal(d) || !Quiet())scalar_fault_park();Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;Fence();return;}
 }
 scalar_fault_park();
}
}
extern "C" unsigned scalar_main(){
 using namespace NextResidualRms;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38NextResidualRmsShape(d))return 0;
 if(!Seal(d) || !Quiet())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=20480;s[6]=d[6];s[7]=TT_Q38_NEXT_RESIDUAL_RMS_MARKER;s[24]=d[3];s[25]=d[4];s[26]=d[41];s[27]=d[5];
 s[28]=d[10];s[29]=d[11];s[30]=d[40];s[31]=TT_Q38_RMS_BACKUP;
 for(unsigned i=0;i<8;i++){s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && s[8+i]!=s[10])scalar_fault_park();}
 for(unsigned i=0;i<6;i++)Packet(i,d);
 if(!PioneerQ38ResidualRms(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x88000),
  reinterpret_cast<const float*>(0x90000),reinterpret_cast<float*>(0x98000),reinterpret_cast<float*>(0xa0000)))scalar_fault_park();
 // ALL sum/domain/norm checks BEFORE FIRST GDDR write. No host numeric
 // result,CPU attention/validation round trip,or modified old QRR1/QRN1 grammar.
 for(unsigned i=6;i<10;i++)Packet(i,d);
 if(!Seal(d) || !Quiet())scalar_fault_park();
 const unsigned delta[]={6,960,10,6,4,640,10,4};
 for(unsigned i=0;i<8;i++){s[16+i]=*Words(0xffb20000+Counter(i));if(uint32_t(s[16+i]-s[8+i])!=delta[i])scalar_fault_park();if(i==6 && s[16+i]!=s[18])scalar_fault_park();}
 s[4]=5120;Fence();return 5120;
}
