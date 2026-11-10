/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwdKernelCompileRole.h"
#include "PioneerQ38QgwdNativeInspectionRole.h"
#if defined(PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION) && (PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION != 1 \
 || (!defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) && !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) \
  && !(defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) && defined(PIONEER_Q38_QGWD_ROOT_IMAGE))) \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error Dense Q8 BF16 producer is isolated offline inspection only
#endif
#ifndef PIONEER_Q38_BF16_PRODUCER_LAYOUT_H
#define PIONEER_Q38_BF16_PRODUCER_LAYOUT_H
#include "PioneerQ38GlobalWorkerLayout.h"
#include "PioneerQ38Bf16Wire.h"
#ifdef PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION
enum {PioneerQ38Bf16ProducerDenseQ8Role=1};
#else
enum {PioneerQ38Bf16ProducerDenseQ8Role=0};
#endif
// NEW typed wire DATA,not an old integer-scale producer or native grant.
namespace PioneerQ38Bf16ProducerLayout {
enum : unsigned {Magic=0x31504251,Format=PioneerQ38Bf16Wire::Magic,Witness=0x31574251,
 Descriptor=0x74200,GateOffset=0x400000,UpOffset=0x411000,NativeOffset=0x20000,LinesOffset=0x30000,
 ProjectionInput=0xe0000,InputRoot=129}; // QBP1/QBI1/QBW1
static inline bool DenseQ8Role(const uint32_t* d){
#ifdef PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION
 return d && d[42]<64 && (d[42]&3)==3 && d[43]>=5 && d[43]<=7;
#else
 (void)d;return false;
#endif
}
static inline unsigned Width(const uint32_t* d){return d[43]==4?17408U:5120U;}
static inline unsigned Reads(const uint32_t* d){return d[43]==4?2U:1U;}
static inline bool DescriptorShape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=67108864 || d[3]>=7 || (d[4]&4095)
  || d[4]<0x04000000 || d[4]>0xc0000000 || (d[43]!=2 && d[43]!=3 && d[43]!=4 && !DenseQ8Role(d))
  || d[42]>=64 || d[40]>=8192 || d[41]>=248320)return false;
 const unsigned width=Width(d),chunks=width/256;
 if(d[5]>=chunks || !d[6] || d[6]>=0x7fffffff || d[7]!=d[5]+1
  || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=PioneerQ38GlobalWorkerLayout::Peer(d[7])
  || d[10]!=(d[43]==4?unsigned(GateOffset):unsigned(ProjectionInput))+d[5]*1024
  || d[11]!=(d[43]==4?unsigned(UpOffset)+d[5]*1024:width)
  || d[12]!=NativeOffset+d[5]*512 || d[13]!=LinesOffset+d[5]*128
  || d[14]!=Format || d[15]!=chunks || !d[16])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=44;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool Job(const uint32_t* j,const uint32_t* d,const uint32_t* source,unsigned worker,unsigned self){
 if(!j || !source || !DescriptorShape(d) || worker!=d[7] || self!=d[9]
  || j[0]!=Magic || j[1]!=256 || j[2]!=512 || j[3]!=d[6] || j[4]!=12
  || j[10]!=Witness || j[11]!=worker || !j[12] || j[12]>=0x7fffffff || j[12]>j[3]
  || j[13]!=(d[43]==4?3U:5U) || j[14] || j[15])return false;
 for(unsigned i=5;i<10;i++)if(j[i])return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=source[i])return false;
 return true;
}
static inline unsigned Delta(const uint32_t* d,unsigned i){
 const unsigned reads=Reads(d);return i==0 || i==3?reads:i==1?reads*16:i==2 || i==6?reads+2:i==4 || i==7?2U:10U;
}
static inline bool Line(const uint32_t* line,const uint32_t* d,unsigned request){
 if(!line || !DescriptorShape(d) || !request || request>d[6])return false;
 for(unsigned i=0;i<8;i++)if(line[i])return false; // BF16 has NO integer scales.
 const unsigned fields[]={Magic,d[6],d[5],Format,Width(d),d[42],d[43],d[40],d[41]};
 for(unsigned i=0;i<9;i++)if(line[8+i]!=fields[i])return false;
 for(unsigned i=0;i<8;i++)if(line[17+i]!=d[32+i])return false;
 return line[25]==d[15] && line[26]==d[7] && line[27]==d[9] && line[28]==d[16]
  && line[29]==request && line[30]==1 && line[31]==1;
}
static inline bool Proof(const uint32_t* p,const uint32_t* j,const uint32_t* d,const uint32_t* source,
 unsigned worker,unsigned self,const uint32_t* before,const uint32_t* after){
 if(!p || !before || !after || !Job(j,d,source,worker,self) || before[2]!=before[6] || after[2]!=after[6])return false;
 for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i] || uint32_t(after[i]-before[i])!=Delta(d,i))return false;
 if(p[16]!=256 || p[17]!=j[12] || p[18]!=worker || p[19]!=Magic || p[30]!=source[0] || p[31]!=self
  || p[48]!=j[13] || p[49]!=d[6] || p[50]!=d[5] || p[51]!=(d[42]|(d[43]<<8))
  || p[52]!=d[40] || p[53]!=d[41] || p[62]!=Format || p[63]!=Width(d))return false;
 const unsigned command[]={0x90e00,0,self,d[4]+d[13],0,d[8],0,0x212,128,0};
 for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(p[54+i]!=d[32+i])return false;
 return true; // DATA consistency only;actual producer/root ownership external.
}
}
#endif
#ifdef PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION
static_assert(PioneerQ38Bf16ProducerDenseQ8Role==1,"late dense Q8 producer selection");
#else
static_assert(PioneerQ38Bf16ProducerDenseQ8Role==0,"late dense Q8 producer removal");
#endif
