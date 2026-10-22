/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_INPUT_SHAPE_H
#define PIONEER_Q38_PROJECTION_INPUT_SHAPE_H
#include "PioneerQ38FfnPersistentShape.h"
// NEW private peer op5 DATA. Same persistent matrix/FFN family,not an old
// image extension grant. Native caller MUST pin the COMPLETE new peer/root
// images and retain exact canonical learned/input/mutable/source owners.
enum {TT_Q38_PROJECTION_INPUT_MARKER=0x31445051,TT_Q38_PROJECTION_INPUT_OPERATION=5,
 TT_Q38_PROJECTION_NATIVE_OFFSET=131072,TT_Q38_PROJECTION_LINES_OFFSET=196608};
static inline uint32_t PioneerQ38ProjectionRoute(unsigned bank){
 static const uint32_t route[]
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
 __attribute__((section(".q38_projection_data")))
#endif
 ={786,978,1170,785,977,1169,1361};
 return bank<7?route[bank]:~uint32_t(0);
}
static inline uint32_t PioneerQ38ProjectionPeer(unsigned chunk){
 if(chunk>=68)return ~uint32_t(0);
 unsigned row=chunk+1,column=1;while(row>=10){row-=10;column++;}
 return column|((row+2)<<6); // first68 logical workers use columns1..7
}
static inline bool PioneerQ38ProjectionGeometry(unsigned layer,unsigned role,unsigned& type,unsigned& inputs,unsigned& outputs){
 unsigned t=0,k=5120,n=0;
 if(layer==64){if(role>1)return false;t=role?14:12;n=248320;}
 else if(layer>=64)return false;
 else if(role>=2 && role<=4){t=12;k=role==4?17408:5120;n=role==4?5120:17408;}
 else if(layer%4==3){
  if(role>=5 && role<=7){t=8;n=role==5?12288:1024;}
  else if(role==10){t=14;k=6144;n=5120;}else return false;
 }else{
  if(role==5 || role==6){t=8;n=role==5?6144:10240;}
  else if(role==10 || role==11){t=8;n=48;}
  else if(role==13){t=8;k=6144;n=5120;}else return false;
 }
 type=t;inputs=k;outputs=n;return true; // fixed semantic geometry ONLY
}
static inline bool PioneerQ38ProjectionInputShape(const uint32_t* d){
 if(!d)return false;
 unsigned type=0,inputs=0,outputs=0;
 if(!PioneerQ38ProjectionGeometry(d[42],d[43],type,inputs,outputs)
  || d[0]!=TT_Q38_PROJECTION_INPUT_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=inputs/256
  || !d[6] || d[6]>=0x7fffffff || d[7]!=d[5]+1 || d[8]!=PioneerQ38ProjectionRoute(d[3])
  || d[9]!=PioneerQ38ProjectionPeer(d[5]) || d[10]!=d[5]*1024 || d[11]!=inputs
  || d[12]!=TT_Q38_PROJECTION_NATIVE_OFFSET+d[5]*512 || d[13]!=TT_Q38_PROJECTION_LINES_OFFSET+d[5]*128
  || d[14]!=type || d[15]!=inputs/256 || !d[16])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=44;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38ProjectionInputJob(const uint32_t* j,const uint32_t* d,const uint32_t* source,unsigned worker,unsigned self){
 if(!j || !source || !PioneerQ38ProjectionInputShape(d) || !worker || worker>68 || worker!=d[7] || self!=d[9]
  || j[0]!=TT_Q38_PROJECTION_INPUT_MARKER || j[1]!=256 || j[2]!=576 || j[3]!=d[6]
  || j[10]!=TT_Q38_FFN_PERSISTENT_MARKER || j[11]!=worker || !j[12] || j[12]>=0x7fffffff || j[12]>j[3]
  || j[13]!=TT_Q38_PROJECTION_INPUT_OPERATION || j[14] || j[15])return false;
 for(unsigned i=4;i<10;i++)if(j[i])return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=source[i])return false;
 return true;
}
static inline bool PioneerQ38ProjectionInputLine(const uint32_t* line,const uint32_t* d){
 if(!line || !PioneerQ38ProjectionInputShape(d))return false;
 for(unsigned i=0;i<8;i++){
  uint32_t v=line[i],m=v&0x7fffffff;
  if(i>=(d[14]==8?8U:1U)?v!=0:(v==0x80000000U || (d[14]==8 && (v&0x80000000U))
   || (m && (m<0x2f800000 || m>0x47800000))))return false;
 }
 const uint32_t fields[]={TT_Q38_PROJECTION_INPUT_MARKER,d[6],d[5],d[14],d[11],d[42],d[43],d[40],d[41]};
 for(unsigned i=0;i<9;i++)if(line[8+i]!=fields[i])return false;
 for(unsigned i=0;i<8;i++)if(line[17+i]!=d[32+i])return false;
 return line[25]==d[15] && line[26]==d[7] && line[27]==d[9] && line[28]==d[16] && !line[29] && !line[30] && line[31]==1;
}
static inline bool PioneerQ38ProjectionInputProof(const uint32_t* p,const uint32_t* j,const uint32_t* d,
 const uint32_t* source,unsigned worker,unsigned self,const uint32_t* before,const uint32_t* after){
 if(!p || !before || !after || !PioneerQ38ProjectionInputJob(j,d,source,worker,self) || before[2]!=before[6] || after[2]!=after[6])return false;
 const unsigned delta[]={1,16,3,1,2,10,3,2};
 for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 if(p[16]!=256 || p[17]!=j[12] || p[18]!=worker || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER || p[30]!=source[0] || p[31]!=self
  || p[48]!=5 || p[49]!=d[6] || p[50]!=d[5] || p[51]!=(d[42]|(d[43]<<8)|(d[14]<<16)) || p[52]!=d[40] || p[53]!=d[41]
  || p[62]!=TT_Q38_PROJECTION_INPUT_MARKER || p[63]!=d[11])return false;
 const uint32_t command[]={0x90e00,0,d[9],d[4]+d[13],0,d[8],0,0x212,128,0};
 for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(p[54+i]!=d[32+i])return false;
 return true; // DATA only,not a physical/native producer completion grant
}
#endif
