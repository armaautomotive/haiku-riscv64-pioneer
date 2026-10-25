/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_OUTPUT_SHAPE_H
#define PIONEER_Q38_RESIDENT_GDN_OUTPUT_SHAPE_H
#include "PioneerQ38GdnOutputInputShape.h"
// QOL1 DATA ONLY. Unlike the older QOI1 helper, this binds every recurrent
// layer and token position. It is NOT source/model/producer/commit authority.
enum { TT_Q38_RESIDENT_GDN_OUTPUT_MARKER=0x314c4f51 };
static inline bool PioneerQ38ResidentGdnOutputShape(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=24 || !d[6] || d[6]>=0x7fffffff
  || d[7]!=d[5]+1 || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=PioneerQ38ProjectionPeer(d[5])
  || d[10]!=TT_Q38_GDN_GATED_OUTPUT+d[5]*1024 || d[11]!=6144
  || d[12]!=TT_Q38_PROJECTION_NATIVE_OFFSET+d[5]*512 || d[13]!=TT_Q38_PROJECTION_LINES_OFFSET+d[5]*128
  || d[14]!=8 || d[15]!=24 || !d[16] || d[40]>=8192 || !d[41] || d[41]>=0x7ffffffd || d[6]!=d[41]+1
  || d[42]>=64 || d[42]%4==3 || d[43]!=13 || d[44]!=TT_Q38_GDN_RENDEZVOUS_MARKER || d[45]>=248320)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=46;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38ResidentGdnOutputJob(const uint32_t* j,const uint32_t* d,const uint32_t* source,unsigned worker,unsigned self){
 if(!j || !source || !PioneerQ38ResidentGdnOutputShape(d) || !worker || worker>24 || worker!=d[7] || self!=d[9]
  || j[0]!=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER || j[1]!=256 || j[2]!=576 || j[3]!=d[6]
  || j[10]!=TT_Q38_FFN_PERSISTENT_MARKER || j[11]!=worker || !j[12] || j[12]>=0x7fffffff || j[12]>j[3]
  || j[13]!=6 || j[14] || j[15])return false;
 for(unsigned i=4;i<10;i++)if(j[i])return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=source[i])return false;
 return true;
}
static inline bool PioneerQ38ResidentGdnOutputLine(const uint32_t* line,const uint32_t* d){
 if(!line || !PioneerQ38ResidentGdnOutputShape(d))return false;
 for(unsigned i=0;i<8;i++){
  uint32_t v=line[i],m=v&0x7fffffff;
  if((v&0x80000000) || (m && (m<0x2f800000 || m>0x47800000)))return false;
 }
 const uint32_t fields[]={TT_Q38_RESIDENT_GDN_OUTPUT_MARKER,d[6],d[5],8,6144,d[42],13,d[40],d[41]};
 for(unsigned i=0;i<9;i++)if(line[8+i]!=fields[i])return false;
 for(unsigned i=0;i<8;i++)if(line[17+i]!=d[32+i])return false;
 return line[25]==24 && line[26]==d[7] && line[27]==d[9] && line[28]==d[16]
  && line[29]==TT_Q38_GDN_RENDEZVOUS_MARKER && line[30]==d[45] && line[31]==1;
}
static inline bool PioneerQ38ResidentGdnOutputProof(const uint32_t* p,const uint32_t* j,const uint32_t* d,
 const uint32_t* source,unsigned worker,unsigned self,const uint32_t* before,const uint32_t* after){
 if(!p || !before || !after || !PioneerQ38ResidentGdnOutputJob(j,d,source,worker,self) || before[2]!=before[6] || after[2]!=after[6])return false;
 const unsigned delta[]={1,16,3,1,2,10,3,2};
 for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 if(p[16]!=256 || p[17]!=j[12] || p[18]!=worker || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER || p[30]!=source[0] || p[31]!=self
  || p[48]!=6 || p[49]!=d[6] || p[50]!=d[5] || p[51]!=(d[42]|13U<<8|8U<<16) || p[52]!=d[40] || p[53]!=d[41]
  || p[62]!=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER || p[63]!=6144)return false;
 if(p[20]!=0x90e00 || p[21] || p[22]!=d[9] || p[23]!=d[4]+d[13] || p[24] || p[25]!=d[8]
  || p[26] || p[27]!=0x212 || p[28]!=128 || p[29])return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(p[54+i]!=d[32+i])return false;
 return true;
}
#endif
