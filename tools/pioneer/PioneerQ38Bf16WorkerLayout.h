/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_WORKER_LAYOUT_H
#define PIONEER_Q38_BF16_WORKER_LAYOUT_H
#include "PioneerQ38Bf16ProducerLayout.h"
#include "PioneerQ38Q4FpuCachedLayout.h"
// DISTINCT QGW4 worker DATA. Old QGW1/QGW2/old integer Q4 are refused.
namespace PioneerQ38Bf16WorkerLayout {
namespace Base=PioneerQ38GlobalWorkerLayout;namespace Producer=PioneerQ38Bf16ProducerLayout;
using namespace Base;
enum : unsigned {Magic=0x34574751,Fault=0x46574754,JobWitness=0x344f4751};
static inline unsigned BootWord(unsigned i){return i==40?unsigned(JobWitness):i==46?unsigned(Producer::Magic):i==47?unsigned(Producer::Format):i==48?32U:Base::BootWord(i);}
#ifdef PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU
#define QGW4_GRAMMAR __attribute__((noinline,section(".bf16.root")))
#else
#define QGW4_GRAMMAR __attribute__((noinline))
#endif
static QGW4_GRAMMAR bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++){
  if((i==40 || (i>=46 && i<=48)) && b[i]!=BootWord(i))return false;
  old[i]=i==40?unsigned(Base::JobWitness):i>=46 && i<=48?0U:b[i];
 }
 old[0]=Base::Magic;return Base::BootShape(old);
}
static inline bool FpuJob(const uint32_t* j){return j && j[2]==1 && j[16]==PioneerQ38Q4FpuCachedLayout::Magic;}
// Keep temporary grammar copies out of the persistent caller's frame;
// they must not remain live throughout the activation's staging buffers.
static QGW4_GRAMMAR bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || !BootShape(b) || j[0]!=Magic)return false;
 if(FpuJob(j) || j[2]==3 || j[2]==5){
  if(densePhase || previous>=0x7ffffffe || j[1]!=previous+1 || !j[1] || j[1]>=0x7fffffff
   || j[3]!=b[3] || j[4]!=b[4] || j[5]>=64 || j[6]>=8192 || !j[7] || j[7]>=0x7fffffff)return false;
  for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
  if(FpuJob(j)){
   PioneerQ38Q4FpuCachedLayout::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
   if(!PioneerQ38Q4FpuCachedLayout::Shape(leaf) || leaf.words[6]!=b[3] || leaf.words[7]!=b[4] || leaf.words[11]!=j[1]
    || j[33]!=(leaf.words[2]==5120?17408U:5120U) || (j[32]&31) || j[32]>=j[33]
    || leaf.words[4]*16>j[33]-j[32] || j[35]!=12
    || (leaf.words[2]==17408?j[34]!=4:(j[34]!=2 && j[34]!=3)))return false;
   for(unsigned i=36;i<64;i++)if(j[i])return false;
   // Exact119-way row/bank/source coverage is ROOT's canonical paired
   // stripe plan,not a flat local partition that can cross bank boundaries.
   return true;
  }
  const unsigned width=j[2]==3?17408U:5120U;
  if(b[3]>width/256 || j[16]!=Producer::Magic || j[17]!=256 || j[18]!=512 || j[19]!=j[7] || j[20]!=12
   || j[27]>=248320 || (j[2]==3?j[28]!=4:(j[28]!=2 && j[28]!=3))
   || j[29]!=width || j[30]!=Producer::Format || j[31]!=Producer::Witness)return false;
  for(unsigned i=21;i<27;i++)if(j[i])return false;
  for(unsigned i=32;i<64;i++)if(j[i])return false;
  return true;
 }
 // Non-Q4 capsules retain their genuine old integer producer contract.
 // No old integer Q4 job is admitted by the BF16 profile.
 if(j[2]==1 && j[17]==12)return false;
 uint32_t oldBoot[64],oldJob[64];for(unsigned i=0;i<64;i++){
  oldBoot[i]=i==40?unsigned(Base::JobWitness):i>=46 && i<=48?0U:b[i];oldJob[i]=j[i];
 }
 oldBoot[0]=oldJob[0]=Base::Magic;if(j[2]==GlobalMatrix){if(j[31]!=JobWitness)return false;oldJob[31]=Base::JobWitness;}
 return Base::JobShape(oldJob,oldBoot,previous,densePhase);
}
#undef QGW4_GRAMMAR
}
#endif
