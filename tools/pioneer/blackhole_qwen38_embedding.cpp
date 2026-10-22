/* SPDX-License-Identifier: MIT */
// New OFFLINE QPE1 root:original tiled Q4_K -> on-card F32 embedding.
// No host payload,CPU learned product,projection/native selection or grant.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerAttentionMath.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingShape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace Embedding {
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
// Exactly20 source tiles,then TWO bounded output writes. No arbitrary
// address/size/mode;all22 commands derived from validated fixed geometry.
static void Packet(unsigned index,const uint32_t* d){
 if(index>=22 || !Seal(d) || !Quiet())scalar_fault_park();
 bool write=index>=20;unsigned offset=write?(index-20)*16384:index*2304;
 unsigned local=write?0x80000+offset:0x90000,remote=write?d[4]+offset:d[11]+offset;
 unsigned bytes=write?(index==20?16384:4096):2304,route=write?d[8]:d[12];
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++){
  expected[i]=*Words(0xffb20000+Counter(i))+delta[i];if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Seal(d) || !Quiet())scalar_fault_park();Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;Fence();return;}
 }
 scalar_fault_park(); // UNKNOWN => NO next packet/retry/restore.
}
static unsigned Byte(unsigned index,unsigned lane){unsigned at=index*16+lane;return (Words(0x90000)[at/4]>>((at%4)*8))&255;}
static bool Decode(unsigned block,unsigned lane){
 float d=PioneerAttentionHalf(uint16_t(Byte(0,lane)|(Byte(1,lane)<<8)));
 float minimum=PioneerAttentionHalf(uint16_t(Byte(2,lane)|(Byte(3,lane)<<8)));
 if((PioneerAttentionBits(d)&0x7fffffff)>=0x7f800000 || (PioneerAttentionBits(minimum)&0x7fffffff)>=0x7f800000)return false;
 for(unsigned group=0;group<8;group++){
  unsigned scale,min;
  if(group<4){scale=Byte(4+group,lane)&63;min=Byte(8+group,lane)&63;}
  else{scale=(Byte(8+group,lane)&15)|((Byte(group,lane)>>6)<<4);min=(Byte(8+group,lane)>>4)|((Byte(4+group,lane)>>6)<<4);}
  float multiplier=d*float(scale),offset=minimum*float(min);
  for(unsigned column=0;column<32;column++){
   unsigned packed=Byte(16+(group/2)*32+column,lane),quant=(group&1)?packed>>4:packed&15;
   float value=multiplier*float(quant)-offset;uint32_t bits=PioneerAttentionBits(value);
   if((bits&0x7fffffff)>=0x7f800000)return false;
   Words(0x80000)[block*256+group*32+column]=bits;
  }
 }
 return true;
}
}
extern "C" unsigned scalar_main(){
 using namespace Embedding;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38EmbeddingShape(d))return 0;
 if(!Seal(d) || !Quiet())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=20480;s[6]=d[6];s[7]=TT_Q38_EMBEDDING_MARKER;s[24]=d[3];s[25]=d[4];s[26]=d[5];s[27]=d[10];
 s[28]=d[11];s[29]=d[13];s[30]=d[40];s[31]=129;
 for(unsigned i=0;i<8;i++){s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && s[8+i]!=s[10])scalar_fault_park();}
 // ALL20 blocks/5120 values must be finite BEFORE ANY output write.
 // Local partial scratch on refusal is NOT a resident input publication.
 for(unsigned block=0;block<20;block++){Packet(block,d);if(!Decode(block,d[13]))scalar_fault_park();}
 Packet(20,d);Packet(21,d);if(!Seal(d) || !Quiet())scalar_fault_park();
 const unsigned delta[]={20,720,22,20,2,320,22,2};
 for(unsigned i=0;i<8;i++){
  s[16+i]=*Words(0xffb20000+Counter(i));if(uint32_t(s[16+i]-s[8+i])!=delta[i])scalar_fault_park();
  if(i==6 && s[16+i]!=s[18])scalar_fault_park();
 }
 s[4]=5120;Fence();return 5120;
}
