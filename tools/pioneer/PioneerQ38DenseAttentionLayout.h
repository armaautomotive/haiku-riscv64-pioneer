/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_ATTENTION_LAYOUT_H
#define PIONEER_Q38_DENSE_ATTENTION_LAYOUT_H
// NEW offline streaming layout ONLY. No old native/Work owner reserves it.
namespace PioneerQ38DenseAttentionLayout {
enum {Magic=0x31414451,Fault=0x46414451,Header=0x74600,Summary=0x74800,
 Query=0x80000,Gate=0x80400,HalfQuery=0x80800,NewK=0x80c00,NewV=0x80e00,
 Scores=0x90000,Accumulate=0x98000,PreGate=0x99000,Output=0x99400,Chunk=0xa0000};
static inline unsigned Group(unsigned head){return head<6?0:head<12?1:head<18?2:3;}
static inline unsigned Route(unsigned bank){const unsigned r[]={786,978,1170,785,977,1169,1361};return bank<7?r[bank]:~0U;}
static inline unsigned Peer(unsigned head){unsigned w=head+1,column=1;while(w>=10){w-=10;column++;}return column|((w+2)<<6);}
static inline bool Shape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=64 || d[3]>=64 || d[3]%4!=3 || d[4]>=8192 || d[5]>=24
  || d[6]!=Group(d[5]) || d[7]!=d[4]+1 || (d[8]&4095) || d[8]<0x04000000 || d[8]>0xc3c00000
  || (d[9]&4095) || d[9]<0x04000000 || d[9]>0xc3c00000 || d[10]>=7 || d[11]>=7
  || d[12]!=Route(d[10]) || d[13]!=Route(d[11]) || d[14]!=Peer(d[5]) || d[15]!=d[5]+1 || !d[16]
  || !d[40] || d[40]>=0x80000000 || d[41]!=256 || d[42]!=0x3d800000 || d[43]!=32 || d[44]!=8192
  || d[45]!=16384 || d[46]!=2*((d[4]+31)>>5) || d[47]!=d[4]*16)return false;
 if(d[10]==d[11] && d[8]<d[9]+4194304 && d[9]<d[8]+4194304)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=48;i<64;i++)if(d[i])return false;
 return true; // geometry DATA,not canonical/authenticated prefix ownership
}
}
#endif
