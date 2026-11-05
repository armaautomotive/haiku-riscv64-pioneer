/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_Q4_MASK_H
#define PIONEER_Q38_PREPARED_Q4_MASK_H
#include "PioneerQ38PreparedQ4Layout.h"
// Exact DATA expansion only. NO quant decode,dot/affine,FP calculation or
// source submission. E owns the CURRENT completed bounded group payload.
struct PioneerQ38PreparedQ4Mask {
 using L=PioneerQ38PreparedQ4Layout;
 static unsigned Pop(unsigned v){v-=((v>>1)&0x55555555);v=(v&0x33333333)+((v>>2)&0x33333333);
  return (((v+(v>>4))&0x0f0f0f0f)*0x01010101)>>24;}
 template<class E> static bool Expand(E& e,unsigned index,const L::Window& group){
  const unsigned kind=index>>30,offset=index&L::OffsetMask;
  if(kind==2){if(offset)return false;for(unsigned i=0;i<128;i++)e.MaskStore(i,0);e.SealData();return true;}
  if(kind>1 || offset<group.start || offset-group.start>group.bytes)return false;
  const unsigned at=offset-group.start,available=group.bytes-at;
  if(!kind){
   if(available<512)return false;
   unsigned rows=0;
   for(unsigned i=0;i<128;i++){const unsigned value=e.PayloadWord(at+i*4);rows+=bool(value&65535)+bool(value>>16);e.MaskStore(i,value);}
   if(rows<240)return false;
   e.SealData();return true;
  }
  if(available<32)return false;
  unsigned bitmap[8],rows=0;
  for(unsigned i=0;i<8;i++){bitmap[i]=e.PayloadWord(at+i*4);rows+=Pop(bitmap[i]);}
  const unsigned bytes=32+rows*2,padded=(bytes+15)&~15U;
  if(!rows || bytes>=512 || padded>available)return false;
  unsigned cursor=32;
  for(unsigned pair=0;pair<128;pair++){
   unsigned word=0;
   for(unsigned half=0;half<2;half++){
    const unsigned row=pair*2+half;if(!(bitmap[row/32]&(1U<<(row&31))))continue;
    const unsigned value=(e.PayloadWord(at+(cursor&~3U))>>((cursor&2)*8))&65535;
    if(!value)return false;
    word|=value<<(half*16);cursor+=2;
   }
   e.MaskStore(pair,word);
  }
  if(cursor!=bytes)return false;
  for(unsigned i=bytes;i<padded;i++)if((e.PayloadWord(at+(i&~3U))>>((i&3)*8))&255)return false;
  e.SealData();return true;
 }
};
#endif
