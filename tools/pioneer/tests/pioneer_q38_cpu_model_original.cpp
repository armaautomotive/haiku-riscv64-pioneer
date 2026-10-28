/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_model_reference.h"
#include "pioneer_q38_whole_original_bytes.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
static void Word(FILE* f,uint32_t word){
 uint8_t bytes[4];for(unsigned i=0;i<4;i++)bytes[i]=uint8_t(word>>(i*8));assert(fwrite(bytes,1,4,f)==4);
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START CPU-REFERENCE ONLY/full original SHA/all851 tensors/row-major/libm/retained GDN+Conv+KV;NO accelerator or device-product uploads/TPS authority");
 WholeModelOriginalBytes original;assert(original.Open(argv[1]) && original.Ready());
 Q38CpuModelReference reference;assert(reference.Bind(original) && reference.Ready());
 unsigned selected=0;
 for(unsigned position=0;position<3;position++){
  Q38CpuTokenTrace trace;unsigned token=position==0?248319:position==1?17:selected;
  assert(reference.Step(token,trace) && reference.Ready() && original.Ready());selected=trace.selected;
  assert(trace.position==position && trace.input==token && trace.hidden.size()==65*5120 && trace.logits.size()==248320);
  std::string path=std::string(argv[2])+"/token-"+std::to_string(position)+".bin";
  FILE* f=fopen(path.c_str(),"wbx");assert(f); // newly created runner-owned output only
  const uint32_t header[]={0x31524651,1,position,token,trace.selected,trace.runnerUp,trace.best,trace.second,65,5120,248320,0,0,0,0,0};
  for(auto word:header)Word(f,word);for(auto word:trace.hidden)Word(f,word);for(auto word:trace.logits)Word(f,word);Word(f,0x454e4451);
  assert(!fflush(f) && !ferror(f) && !fclose(f));
  float best,second;memcpy(&best,&trace.best,4);memcpy(&second,&trace.second,4);
  printf("CPU_REFERENCE_TOKEN position%u/input%u/greedy%u/runnerUp%u/best%.9g/second%.9g/65hidden+all248320logits SAVED;NOT accelerator match or inference throughput\n",position,token,trace.selected,trace.runnerUp,best,second);
 }
 puts("PASS CPU-ONLY original27B3-position reference computed;MUST compare independently with complete emitted/device generations;NOT native/physical/Work/TPS qualification");
}
