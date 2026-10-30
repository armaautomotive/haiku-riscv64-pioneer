/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PRECISE_WORKER_LAYOUT_H
#define PIONEER_Q38_PRECISE_WORKER_LAYOUT_H
#include "PioneerQ38GlobalWorkerLayout.h"
// DISTINCT proposed QGW2/QPA1 DATA, never an old QGW1 producer grant.
namespace PioneerQ38PreciseWorkerLayout {
namespace Base=PioneerQ38GlobalWorkerLayout;
using namespace Base;
enum {Magic=0x32574751,Fault=0x46574752,JobWitness=0x324f4751,
 ActivationMagic=0x31415051,ActivationOperation=3,Chunks=68,ChunkValues=256,
 Descriptor=0x74200,GateOffset=0x400000,UpOffset=0x411000,
 NativeOffset=TT_Q38_PROJECTION_NATIVE_OFFSET,LinesOffset=TT_Q38_PROJECTION_LINES_OFFSET};
static inline unsigned BootWord(unsigned i){
 return i==40?unsigned(JobWitness):i==46?unsigned(ActivationMagic):i==47?unsigned(Chunks)
  :i==48?unsigned(ChunkValues):i>=46?0U:Base::BootWord(i);
}
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) && defined(__riscv)
// Shared ROOT-only cold grammar avoids repeated large stack temporaries
// and fits the existing ROOT text/Conv split. Default worker bytes stay fixed.
#define PIONEER_Q38_PRECISE_GRAMMAR_FN inline __attribute__((noinline,section(".text.q38_precise_shape")))
#else
#define PIONEER_Q38_PRECISE_GRAMMAR_FN static inline
#endif
PIONEER_Q38_PRECISE_GRAMMAR_FN bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++){
  if(i>=40 && i<49 && b[i]!=BootWord(i))return false;
  old[i]=i>=46 && i<49?0U:i==40?unsigned(Base::JobWitness):b[i];
 }
 old[0]=Base::Magic;return Base::BootShape(old);
}
PIONEER_Q38_PRECISE_GRAMMAR_FN bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || !BootShape(b) || j[0]!=Magic)return false;
 uint32_t oldBoot[64],oldJob[64];
 for(unsigned i=0;i<64;i++){oldBoot[i]=i>=46 && i<49?0U:i==40?unsigned(Base::JobWitness):b[i];oldJob[i]=j[i];}
 oldBoot[0]=oldJob[0]=Base::Magic;
 if(j[2]==ActivationOperation)oldJob[27]=0; // QGW2 input-token field, not old reserved DATA
 if(j[2]==GlobalMatrix){if(j[31]!=JobWitness)return false;oldJob[31]=Base::JobWitness;}
 if(!Base::JobShape(oldJob,oldBoot,previous,densePhase))return false;
 if(j[2]==ActivationOperation){
  if(b[3]>Chunks || j[16]!=ActivationMagic || j[17]!=256 || j[18]!=576 || j[19]!=j[7] || j[20]!=12 || j[27]>=248320)return false;
  for(unsigned i=21;i<64;i++)if(i!=27 && j[i])return false;
 }
 return true;
}
#undef PIONEER_Q38_PRECISE_GRAMMAR_FN
static inline bool DescriptorShape(const uint32_t* d){
 if(!d || d[0]!=ActivationMagic || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=Chunks
  || !d[6] || d[6]>=0x7fffffff || d[7]!=d[5]+1 || d[8]!=PioneerQ38ProjectionRoute(d[3])
  || d[9]!=Peer(d[7]) || d[10]!=GateOffset+d[5]*1024 || d[11]!=UpOffset+d[5]*1024
  || d[12]!=NativeOffset+d[5]*512 || d[13]!=LinesOffset+d[5]*128 || d[14]!=12 || d[15]!=Chunks
  || !d[16] || d[40]>=8192 || d[41]>=248320 || d[42]>=64 || d[43]!=4)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=44;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool ActivationJob(const uint32_t* j,const uint32_t* d,const uint32_t* source,unsigned worker,unsigned self){
 if(!j || !source || !DescriptorShape(d) || !worker || worker>Chunks || d[7]!=worker || d[9]!=self
  || j[0]!=ActivationMagic || j[1]!=256 || j[2]!=576 || j[3]!=d[6] || j[4]!=12
  || j[10]!=TT_Q38_FFN_PERSISTENT_MARKER || j[11]!=worker || !j[12] || j[12]>=0x7fffffff
  || j[12]>j[3] || j[13]!=ActivationOperation || j[14] || j[15])return false;
 for(unsigned i=5;i<10;i++)if(j[i])return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=source[i])return false;
 return true;
}
static inline bool ScaleLine(const uint32_t* line,const uint32_t* d){
 if(!line || !DescriptorShape(d))return false;
 unsigned magnitude=line[0]&0x7fffffff;
 if(line[0]==0x80000000U || (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)))return false;
 for(unsigned i=1;i<8;i++)if(line[i])return false;
 const uint32_t fields[]={ActivationMagic,d[6],d[5],12,17408,d[42],4,d[40],d[41]};
 for(unsigned i=0;i<9;i++)if(line[8+i]!=fields[i])return false;
 for(unsigned i=0;i<8;i++)if(line[17+i]!=d[32+i])return false;
 return line[25]==Chunks && line[26]==d[7] && line[27]==d[9] && line[28]==d[16]
  && !line[29] && !line[30] && line[31]==1;
}
static inline bool ActivationProof(const uint32_t* p,const uint32_t* j,const uint32_t* d,
 const uint32_t* source,unsigned worker,unsigned self,const uint32_t* before,const uint32_t* after){
 if(!p || !before || !after || !ActivationJob(j,d,source,worker,self) || before[2]!=before[6] || after[2]!=after[6])return false;
 const unsigned delta[]={2,32,4,2,2,10,4,2};
 for(unsigned i=0;i<8;i++)if(p[i]!=before[i] || p[8+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 if(p[16]!=256 || p[17]!=j[12] || p[18]!=worker || p[19]!=ActivationMagic || p[30]!=source[0] || p[31]!=self
  || p[48]!=3 || p[49]!=d[6] || p[50]!=d[5] || p[51]!=d[42] || p[52]!=d[40] || p[53]!=d[41]
  || p[62]!=ActivationMagic || p[63]!=17408)return false;
 if(p[20]!=0x90e00 || p[21] || p[22]!=self || p[23]!=d[4]+d[13] || p[24] || p[25]!=d[8]
  || p[26] || p[27]!=0x212 || p[28]!=128 || p[29])return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=source[i])return false;
 for(unsigned i=0;i<8;i++)if(p[54+i]!=d[32+i])return false;
 return true; // DATA observation ONLY;actual current producer/cohort proof remains separate
}
}
#endif
