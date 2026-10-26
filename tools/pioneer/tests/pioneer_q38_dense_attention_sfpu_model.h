/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_ATTENTION_SFPU_MODEL_H
#define PIONEER_Q38_DENSE_ATTENTION_SFPU_MODEL_H
#include "../PioneerBlackholeFma.h"
// TEST ONLY: same32-lane partial-FMA semantics as the prior strict
// attention interpreter. Not expected arithmetic products or device input.
struct DenseSfpuModel {
 uint32_t lanes[11][32]={},dst[12][16]={},config[224]={},thread[68]={},ack=0;
 bool drained=true,waiting=false,dropAck=false,dropProduct=false,transpose=false;
 unsigned commands=0,products=0,drains=0;
 DenseSfpuModel(){for(unsigned i=0;i<32;i++)lanes[10][i]=PioneerAttentionBits(1.f);}
 static unsigned Lane(unsigned i){return (i>>3)*16+(i&7)*2;}
 static float F(uint32_t x){return PioneerAttentionFloat(x);}
 static uint32_t B(float x){return PioneerAttentionBits(x);}
 static float Flush(float x){unsigned b=B(x);return b&0x7f800000?x:F(b&0x80000000);}
 void Emit(uint32_t op){
  commands++;unsigned code=op>>24;
  if(code==0xb2){unsigned i=(op>>16)&255;assert(i<68);thread[i]=op&65535;}
  else if(op==0x3700000f){assert(!thread[0]);}
  else if(op==0x8a000002){}
  else if(op==0xa2101816){assert(!ack);waiting=true;}
  else if(op==0x4500017e){assert(waiting && !ack);if(!dropAck)ack=1;waiting=false;drained=true;drains++;return;}
  else if(code==0x70 || code==0x72){
   unsigned reg=(op>>20)&15,mode=(op>>16)&15,at=op&1023;
   assert(reg<8 && !(at&1) && !(op&0xfc00) && !thread[1] && !thread[36] && !config[3] && !config[6] && !config[7]);
   for(unsigned lane=0;lane<32;lane++){
    unsigned row=(at&~3U)+(lane>>3),col=(lane&7)*2+unsigned(bool(at&2));assert(row<12);
    if(code==0x72){assert(mode==3 && row>=8);dst[row][col]=B(Flush(F(lanes[reg][lane])));}
    else if(mode==11)lanes[reg][lane]=0;
    else {assert(mode==3 && row<8);lanes[reg][lane]=dst[row][col];}
   }
  }else if(code==0x71){
   unsigned reg=(op>>20)&15,mode=(op>>16)&15;assert(reg<8 && (mode==0 || mode==10));
   for(unsigned lane=0;lane<32;lane++)lanes[reg][lane]=mode?(lanes[reg][lane]&0xffff0000)|(op&65535):(op&65535)<<16;
  }else if(code==0x84 || code==0x85){
   unsigned a=(op>>16)&15,b=(op>>12)&15,c=(op>>8)&15,d=(op>>4)&15,mod=op&15;
   assert((a<8 || a==10) && (b<8 || b==10) && (c<8 || c==9) && d<8 && !(mod&~3));
   if(code==0x84)products++;
   for(unsigned lane=0;lane<32;lane++){
    float av=F(lanes[a][lane]),bv=F(lanes[b][lane]),cv=F(lanes[c][lane]);
    if(mod&1)av=-av;if(mod&2)cv=-cv;if(dropProduct && code==0x84)av=0;
    lanes[d][lane]=B(PioneerBlackholeFma(av,bv,cv));
   }
  }else if(op==0x7c000520){for(unsigned lane=0;lane<32;lane++)lanes[2][lane]=lanes[5][lane];}
  else {fprintf(stderr,"UNQUALIFIED dense SFPU %08x\n",op);assert(false);}
  drained=false;
 }
 uint32_t Load(uint32_t a){
  if(a==0xffe000fc)return ack;
  if(a>=0xffef0000 && a<0xffef0000+224*4){assert(drained);return config[(a-0xffef0000)/4];}
  assert(a>=0xffbd8000 && a<0xffbd8000+12*64 && drained && !config[3]);
  return dst[(a-0xffbd8000)/64][((a-0xffbd8000)/4)%16];
 }
 void Store(uint32_t a,uint32_t value){
  if(a==0xffe40000){Emit(value);return;}
  if(a==0xffe000fc){assert(!value);ack=0;return;}
  if(a>=0xffef0000 && a<0xffef0000+224*4){
   assert(drained);unsigned i=(a-0xffef0000)/4;assert(i==0 || i==1 || i==2 || i==3 || i==6 || i==7 || i==220);
   assert(value==(i==0?0x4231U:i==1?0x20220000U:0U));config[i]=value;return;
  }
  assert(a>=0xffbd8000 && a<0xffbd8000+8*64 && drained && !config[3]);
  unsigned row=(a-0xffbd8000)/64,col=((a-0xffbd8000)/4)%16;assert(!(col&1));
  if(transpose)row=(row/4)*4+(3-row%4);dst[row][col]=value;
 }
};
#endif
