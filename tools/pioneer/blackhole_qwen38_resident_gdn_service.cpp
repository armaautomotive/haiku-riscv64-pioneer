/* SPDX-License-Identifier: MIT */
// OFFLINE returning QGT1 composite. No native registry or model grant.
#include "PioneerQ38ResidentGdnOperatorImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnServiceShape.h"
extern "C" unsigned q38_resident_gdn_once();
extern "C" void block_fault_park() __attribute__((noreturn));
namespace GdnService {
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static bool Current(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self,
 const uint32_t* d,const uint32_t* input,const uint32_t* state){
 if(*Words(0x74100)!=job[12])return false;
 for(unsigned i=0;i<16;i++)if(Words(0x74000)[i]!=job[i] || *Words(0xffb20000+Source(i))!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(Words(0x74040)[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?worker:i==2?self:i==3?source[0]:0))return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_GDN_GATED_DESCRIPTOR)[i]!=d[i]
  || Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i]!=input[i] || Words(TT_Q38_RGDN_DESCRIPTOR)[i]!=state[i])return false;
 return Quiet();
}
}
extern "C" void q38_resident_persistent_gdn(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self){
 using namespace GdnService;
 uint32_t d[64],input[64],state[64],before[8],after[8],first[32],recurrent[32],gated[32],clock[16];
 for(unsigned i=0;i<64;i++){
  d[i]=Words(TT_Q38_GDN_GATED_DESCRIPTOR)[i];input[i]=Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i];state[i]=Words(TT_Q38_RGDN_DESCRIPTOR)[i];
 }
 if(!PioneerQ38ResidentGdnServiceJob(job,d,input,state,source,worker,self))block_fault_park();
 // The future ROOT must clear ALL96 cells BEFORE issuing ANY peer op8.
 // Never clear an old seal here or turn stale metadata into readiness.
 for(unsigned i=0;i<16;i++)if(Words(TT_Q38_GDN_GATED_READY)[i])block_fault_park();
 if(!Current(job,source,worker,self,d,input,state))block_fault_park();
 for(unsigned i=0;i<8;i++){before[i]=*Words(0xffb20000+Counter(i));if(i==6 && before[i]!=before[2])block_fault_park();}
 Words(0x74140)[0]=job[12];Words(0x74140)[4]=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION;Fence();
 if(q38_resident_gdn_once()!=64 || !Current(job,source,worker,self,d,input,state))block_fault_park();
 for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<32;i++){
  first[i]=Words(TT_Q38_GDN_INPUT_SUMMARY)[i];recurrent[i]=Words(TT_Q38_RGDN_SUMMARY)[i];gated[i]=Words(TT_Q38_GDN_GATED_SUMMARY)[i];
 }
 for(unsigned i=0;i<16;i++)clock[i]=Words(TT_Q38_GDN_RENDEZVOUS_CLOCK)[i];
 if(!PioneerQ38ResidentGdnServiceSummaries(d,input,state,first,recurrent,gated,clock,before,after))block_fault_park();
 for(unsigned i=0;i<16;i++)if(Words(TT_Q38_GDN_GATED_READY)[i]!=PioneerQ38GdnRendezvousReadyWord(d,i,false))block_fault_park();
 const uint32_t command[]={0x8ea00,0,self,d[4]+d[43],0,d[8],0,0x212,256,0};
 for(unsigned i=0;i<10;i++)if(*Words(0xffb20000+i*4)!=command[i])block_fault_park();
 volatile uint32_t* p=Words(0x83f00);
 for(unsigned i=0;i<8;i++){p[i]=before[i];p[8+i]=after[i];}
 p[16]=64;p[17]=job[12];p[18]=worker;p[19]=TT_Q38_FFN_PERSISTENT_MARKER;
 for(unsigned i=0;i<10;i++)p[20+i]=command[i];
 p[30]=source[0];p[31]=self;for(unsigned i=0;i<16;i++)p[32+i]=source[i];
 p[48]=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION;p[49]=d[6];p[50]=d[5];p[51]=d[7];p[52]=d[40];p[53]=state[11];
 for(unsigned i=0;i<8;i++)p[54+i]=d[32+i];
 p[62]=TT_Q38_GDN_RENDEZVOUS_MARKER;p[63]=gated[5];Fence();
 Words(0x74140)[4]=0;Words(0x74140)[1]=job[12];Fence();
 // SAME sequence loop retains code and state;NO reset/CPU intermediate.
}
