/* SPDX-License-Identifier: MIT */
// Returning BF16 producer capsule bound to the actual worker private seal.
#include "PioneerQ38Bf16WorkerImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_BF16_ACTIVATION_IMAGE 1
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "PioneerQ38Bf16ActivationChunk.h"
#include "PioneerQ38Bf16ProducerLayout.h"
extern "C" void block_fault_park() __attribute__((noreturn));
extern "C" void q38_mixed_seal();
namespace Bf16Producer {
namespace P=PioneerQ38Bf16ProducerLayout;
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence":::"memory");}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static void Seal(const uint32_t* d){
 q38_mixed_seal();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])block_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(P::Descriptor)[i]!=d[i])block_fault_park();
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))block_fault_park();
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)block_fault_park();
}
static __attribute__((noinline)) void Packet(unsigned p,const uint32_t* d){
 const unsigned reads=P::Reads(d);if(p>=reads+2)block_fault_park();Seal(d);
 const bool write=p>=reads,trailer=p==reads+1;
 const unsigned here=write?(trailer?0x90e00:0x90c00):(p?0x90400:0x90000);
 const unsigned size=write?(trailer?128U:512U):1024U,offset=write?d[trailer?13:12]:d[10+p];
 const bool rootInput=!write && reads==1;
 const unsigned remote=rootInput?offset:d[4]+offset,sourceRoute=rootInput?unsigned(P::InputRoot):d[8];
 uint32_t expected[8];const unsigned delta[]={write?0U:1U,write?0U:size/64,1,write?0U:1U,write?1U:0U,write?size/64:0U,1,write?1U:0U};
 for(unsigned i=0;i<8;i++){expected[i]=*Words(0xffb20000+Counter(i))+delta[i];if(i==6 && expected[6]!=expected[2])block_fault_park();}
 const unsigned command[]={write?here:remote,0,write?d[9]:sourceRoute,write?remote:here,0,write?d[8]:d[9],0,write?0x212U:0,size,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  const unsigned busy=*Words(0xffb20040);if(busy>1)block_fault_park();bool done=!busy;
  for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){Seal(d);for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();Fence();return;}
 }
 block_fault_park(); // UNKNOWN,zero subsequent packet/reset/retry/cleanup.
}
}
extern "C" void q38_bf16_persistent_producer(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self){
 using namespace Bf16Producer;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(P::Descriptor)[i];
 if(!P::Job(job,d,source,worker,self) || *Words(0x74100)!=job[12])block_fault_park();
 Seal(d);
 uint32_t before[8];for(unsigned i=0;i<8;i++){before[i]=*Words(0xffb20000+Counter(i));if(i==6 && before[6]!=before[2])block_fault_park();}
 const unsigned reads=P::Reads(d);for(unsigned p=0;p<reads;p++)Packet(p,d);
 if(reads==1){if(!PioneerQ38Bf16Input::Pack(reinterpret_cast<const uint32_t*>(0x90000),256,reinterpret_cast<uint32_t*>(0x90c00),128))block_fault_park();}
 else if(!PioneerQ38Bf16ActivationChunk(reinterpret_cast<const float*>(0x90000),reinterpret_cast<const float*>(0x90400),256,
  reinterpret_cast<float*>(0x90800),reinterpret_cast<uint32_t*>(0x90c00)))block_fault_park();
 auto* line=Words(0x90e00);for(unsigned i=0;i<32;i++)line[i]=0;
 const unsigned fields[]={P::Magic,d[6],d[5],P::Format,P::Width(d),d[42],d[43],d[40],d[41]};
 for(unsigned i=0;i<9;i++)line[8+i]=fields[i];
 for(unsigned i=0;i<8;i++)line[17+i]=d[32+i];
 line[25]=d[15];line[26]=worker;line[27]=self;line[28]=source[0];line[29]=job[12];line[30]=1;line[31]=1;
 Packet(reads,d);Packet(reads+1,d);Seal(d);
 if(*Words(0x74100)!=job[12])block_fault_park();
 for(unsigned i=0;i<16;i++)if(Words(0x74000)[i]!=job[i])block_fault_park();
 for(unsigned i=0;i<8;i++)if(Words(0x74040)[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?worker:i==2?self:i==3?source[0]:0))block_fault_park();
 auto* proof=Words(0x83f00);for(unsigned i=0;i<8;i++){
  const unsigned after=*Words(0xffb20000+Counter(i));if(uint32_t(after-before[i])!=P::Delta(d,i))block_fault_park();proof[i]=before[i];proof[8+i]=after;
 }
 proof[16]=256;proof[17]=job[12];proof[18]=worker;proof[19]=P::Magic;
 for(unsigned i=0;i<10;i++)proof[20+i]=Words(0xffb20000)[i];
 proof[30]=source[0];proof[31]=self;
 for(unsigned i=0;i<16;i++)proof[32+i]=source[i];
 proof[48]=job[13];proof[49]=d[6];proof[50]=d[5];proof[51]=d[42]|(d[43]<<8);proof[52]=d[40];proof[53]=d[41];
 for(unsigned i=0;i<8;i++)proof[54+i]=d[32+i];
 proof[62]=P::Format;proof[63]=P::Width(d);Fence();
}
