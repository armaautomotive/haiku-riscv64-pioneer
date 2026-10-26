/* SPDX-License-Identifier: MIT */
// OFFLINE returning op6: actual gated GDDR -> native Q8_0 + fresh ancestry.
// All-layer DATA grammar, not a native producer or full-model launch grant.
#include "PioneerQ38ResidentGdnOutputImageRole.h"
#include "PioneerQ38MixedCapsuleHook.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38ProjectionInput.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnOutputShape.h"
extern "C" void block_fault_park() __attribute__((noreturn));
namespace ResidentGdnOutput {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static __attribute__((noinline,optimize("Os"))) bool Current(const uint32_t* d,const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self){
 PIONEER_Q38_MIXED_SEAL();
 if(*Words(0x74100)!=job[12])return false;
 for(unsigned i=0;i<16;i++)if(Words(0x74000)[i]!=job[i] || *Words(0xffb20000+Source(i))!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(Words(0x74040)[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?worker:i==2?self:i==3?source[0]:0))return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]!=d[i])return false;
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static __attribute__((noinline)) void Command(unsigned p,const uint32_t* d,uint32_t* command){
 if(p>=3)block_fault_park();
 bool write=p!=0;
 static const unsigned local[] __attribute__((section(".q38_projection_data")))={0x90000,0x90c00,0x90e00};
 static const unsigned size[] __attribute__((section(".q38_projection_data")))={1024,512,128};
 static const unsigned field[] __attribute__((section(".q38_projection_data")))={10,12,13};
 unsigned bytes=size[p],here=local[p],remote=d[4]+d[field[p]];
 command[0]=write?here:remote;command[1]=0;command[2]=write?d[9]:d[8];command[3]=write?remote:here;
 command[4]=0;command[5]=write?d[8]:d[9];command[6]=0;command[7]=write?0x212U:0;command[8]=bytes;command[9]=0;
}
static __attribute__((noinline)) void Packet(unsigned p,const uint32_t* d,const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self,uint32_t* tracked){
 if(p>=3 || !Current(d,job,source,worker,self))block_fault_park();
 uint32_t command[10],expected[8];
 if(p){
  Command(p-1,d,command);
  for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 }
 Command(p,d,command);bool write=p!=0;unsigned bytes=command[8];
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 // No unexplained counter movement may be absorbed as the next baseline.
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])block_fault_park();
  expected[i]=tracked[i]+delta[i];if(i==6 && expected[6]!=expected[2])block_fault_park();
 }
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){
   if(!Current(d,job,source,worker,self))block_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   return;
  }
 }
 block_fault_park(); // UNKNOWN: no following packet/reset/restore/retry.
}
}
extern "C" __attribute__((noinline,optimize("Os"))) void q38_resident_persistent_gdn_output(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self){
 using namespace ResidentGdnOutput;uint32_t d[64];
 for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38ResidentGdnOutputJob(job,d,source,worker,self) || !Current(d,job,source,worker,self))block_fault_park();
 uint32_t before[8],tracked[8];
 for(unsigned i=0;i<8;i++){
  tracked[i]=before[i]=*Words(0xffb20000+Counter(i));if(i==6 && before[6]!=before[2])block_fault_park();
 }
 Words(0x74140)[0]=job[12];Words(0x74140)[4]=6;Fence();
 for(unsigned i=0;i<256;i++)Words(0x90000)[i]=0x7fc90000+i;
 for(unsigned i=0;i<256;i++)if(Words(0x90000)[i]!=0x7fc90000+i)block_fault_park();
 Packet(0,d,job,source,worker,self,tracked);
 if(!PioneerQ38ProjectionQuantize(8,reinterpret_cast<const float*>(0x90000),256,reinterpret_cast<PioneerQ38ProjectionInput*>(0x90c00)))block_fault_park();
 volatile uint32_t* line=Words(0x90e00);
 volatile uint32_t fields[9]; // Explicit WORD stores: freestanding, no memcpy.
 fields[0]=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER;fields[1]=d[6];fields[2]=d[5];fields[3]=8;fields[4]=6144;
 fields[5]=d[42];fields[6]=13;fields[7]=d[40];fields[8]=d[41];
 for(unsigned i=0;i<9;i++)line[8+i]=fields[i];
 for(unsigned i=0;i<8;i++)line[17+i]=d[32+i];
 line[25]=24;line[26]=worker;line[27]=self;line[28]=source[0];line[29]=TT_Q38_GDN_RENDEZVOUS_MARKER;line[30]=d[45];line[31]=1;
 Packet(1,d,job,source,worker,self,tracked);Packet(2,d,job,source,worker,self,tracked);
 if(!Current(d,job,source,worker,self))block_fault_park();
 uint32_t after[8];static const unsigned delta[] __attribute__((section(".q38_projection_data")))={1,16,3,1,2,10,3,2};
 for(unsigned i=0;i<8;i++){
  after[i]=*Words(0xffb20000+Counter(i));
  if(after[i]!=tracked[i] || uint32_t(after[i]-before[i])!=delta[i] || (i==6 && after[6]!=after[2]))block_fault_park();
 }
 volatile uint32_t* proof=Words(0x83f00);
 for(unsigned i=0;i<8;i++){proof[i]=before[i];proof[8+i]=after[i];}
 proof[16]=256;proof[17]=job[12];proof[18]=worker;proof[19]=TT_Q38_FFN_PERSISTENT_MARKER;
 uint32_t last[10];Command(2,d,last);
 for(unsigned i=0;i<10;i++){
  uint32_t value=Words(0xffb20000)[i];if(value!=last[i])block_fault_park();proof[20+i]=value;
 }
 proof[30]=source[0];proof[31]=self;for(unsigned i=0;i<16;i++)proof[32+i]=source[i];
 proof[48]=6;proof[49]=d[6];proof[50]=d[5];proof[51]=d[42]|13U<<8|8U<<16;proof[52]=d[40];proof[53]=d[41];
 for(unsigned i=0;i<8;i++)proof[54+i]=d[32+i];
 proof[62]=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER;proof[63]=6144;Fence();
 Words(0x74140)[4]=0;Words(0x74140)[1]=job[12];Fence();
}
