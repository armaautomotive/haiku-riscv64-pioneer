/* SPDX-License-Identifier: MIT */
#ifdef _KERNEL_MODE
#error Offline failure observation is not a driver or native execution path
#endif
#ifndef PIONEER_Q38_INSTRUCTION_FAULT_H
#define PIONEER_Q38_INSTRUCTION_FAULT_H
#include <cstdint>
#include <cstdio>
#include <cstring>
// Read the host interpreter's existing memory map ONLY. Never call Load,
// Get, Step, a modeled CSR or any mutating/device helper while diagnosing.
namespace PioneerQ38InstructionFault {
static inline uint32_t Bits(float value){uint32_t b;memcpy(&b,&value,4);return b;}
static inline float Float(uint32_t bits){float v;memcpy(&v,&bits,4);return v;}
static inline bool Domain(uint32_t bits){unsigned a=bits&0x7fffffff;return a<=0x477fe000 && (!a || a>=0x26800000);}
template<class Machine> static bool Word(const Machine& m,unsigned address,uint32_t& value){
 const auto found=m.memory.find(address);if(found==m.memory.end())return false;value=found->second;return true;
}
template<class Machine> static void Vector(const Machine& m,const char* name,unsigned address,FILE* out){
 unsigned missing=0,nonfinite=0,large=0,tiny=0,zero=0,min=0x7f800000,max=0,first=5120,firstBits=0;
 for(unsigned i=0;i<5120;i++){
  uint32_t b=0;if(!Word(m,address+i*4,b)){missing++;continue;}unsigned a=b&0x7fffffff;
  if(a>=0x7f800000)nonfinite++;
  else{if(a<min)min=a;if(a>max)max=a;if(!a)zero++;else if(a<0x26800000)tiny++;if(a>0x477fe000)large++;}
  if(first==5120 && !Domain(b)){first=i;firstBits=b;}
 }
 fprintf(out,"FAULT_VECTOR %s address%x missing%u nonfinite%u above65504%u below2^-50%u zero%u minAbsBits%08x maxAbsBits%08x firstInvalid%u bits%08x\n",
  name,address,missing,nonfinite,large,tiny,zero,min,max,first,firstBits);
}
template<class Machine> static void Dump(const Machine& m,unsigned previousPC,unsigned projected,unsigned residual,unsigned final,unsigned original,FILE* out=stderr){
 fprintf(out,"ACTUAL_FAULT_SNAPSHOT pc%08x previousPC%08x ra%08x sp%08x s11%08x layer%u completed%u packets%u;READ_ONLY host map,NOT completion/native/TPS proof\n",
  m.pc,previousPC,m.regs[1],m.regs[2],m.regs[27],m.layer,m.completed,m.packets);
 for(unsigned i=0;i<32;i+=8){fprintf(out,"FAULT_REGS x%u",i);for(unsigned n=0;n<8;n++)fprintf(out," %08x",m.regs[i+n]);fputc('\n',out);}
 fprintf(out,"FAULT_COMMAND");for(unsigned value:m.command)fprintf(out," %08x",value);fputc('\n',out);
 fprintf(out,"FAULT_COUNTERS");for(unsigned value:m.counts)fprintf(out," %08x",value);fputc('\n',out);
 Vector(m,"projected",projected,out);Vector(m,"postResidual",residual,out);Vector(m,"final",final,out);Vector(m,"original",original,out);
 unsigned missing=0,badA=0,badB=0,badSum=0,shown=0;
 for(unsigned i=0;i<5120;i++){
  uint32_t a=0,b=0;if(!Word(m,projected+i*4,a) || !Word(m,residual+i*4,b)){missing++;continue;}
  uint32_t sum=Bits(Float(a)+Float(b));badA+=!Domain(a);badB+=!Domain(b);badSum+=!Domain(sum);
  if((!Domain(a) || !Domain(b) || !Domain(sum)) && shown++<4)
   fprintf(out,"FAULT_FINAL_DOMAIN index%u a%08x b%08x sum%08x domain%u/%u/%u\n",i,a,b,sum,unsigned(Domain(a)),unsigned(Domain(b)),unsigned(Domain(sum)));
 }
 fprintf(out,"FAULT_FINAL_DOMAIN_TOTAL missing%u invalidProjected%u invalidResidual%u invalidSum%u;classification only,NOT a claim of the fault callsite\n",missing,badA,badB,badSum);
 fflush(out);
}
}
#endif
