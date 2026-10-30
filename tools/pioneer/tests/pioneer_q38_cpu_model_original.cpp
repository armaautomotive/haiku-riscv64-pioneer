/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_model_reference.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "pioneer_q38_state_snapshot_data.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
static void Word(FILE* f,uint32_t word){
 uint8_t bytes[4];for(unsigned i=0;i<4;i++)bytes[i]=uint8_t(word>>(i*8));assert(fwrite(bytes,1,4,f)==4);
}
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!strcmp(argv[3],"--state-snapshots") || !strcmp(argv[3],"--eos-state-snapshots"))));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool eos=argc==4 && !strcmp(argv[3],"--eos-state-snapshots"),snapshots=argc==4 && !eos;
 unsigned records=0;std::string stateDirectory=std::string(argv[2])+(eos?"/eos-state-cpu":"/state-cpu");
 puts("START CPU-REFERENCE ONLY/full original SHA/all851 tensors/row-major/libm/retained GDN+Conv+KV;NO accelerator or device-product uploads/TPS authority");
 WholeModelOriginalBytes original;assert(original.Open(argv[1]) && original.Ready());
 Q38CpuModelReference reference;assert(reference.Bind(original) && reference.Ready());
 unsigned selected=0;
 for(unsigned position=0;position<(eos?2U:3U);position++){
  Q38CpuTokenTrace trace;unsigned token=position==0?248319:position==1?17:selected;
  assert(reference.Step(token,trace) && reference.Ready() && original.Ready());selected=trace.selected;
  assert(trace.position==position && trace.input==token && trace.hidden.size()==65*5120 && trace.logits.size()==248320);
  if(eos)assert(selected==(position?17U:70253U)); // independent control,not supplied model products
  std::string path=std::string(argv[2])+"/token-"+std::to_string(position)+".bin";
  FILE* f=fopen(path.c_str(),"wbx");assert(f); // newly created runner-owned output only
  const uint32_t header[]={0x31524651,1,position,token,trace.selected,trace.runnerUp,trace.best,trace.second,65,5120,248320,0,0,0,0,0};
  for(auto word:header)Word(f,word);for(auto word:trace.hidden)Word(f,word);for(auto word:trace.logits)Word(f,word);Word(f,0x454e4451);
  assert(!fflush(f) && !ferror(f) && !fclose(f));
  float best,second;memcpy(&best,&trace.best,4);memcpy(&second,&trace.second,4);
  printf("CPU_REFERENCE_TOKEN position%u/input%u/greedy%u/runnerUp%u/best%.9g/second%.9g/65hidden+all248320logits SAVED;NOT accelerator match or inference throughput\n",position,token,trace.selected,trace.runnerUp,best,second);
  if(snapshots && position>=1)for(unsigned layer=0;layer<64;layer++)for(unsigned kind=0;kind<4;kind++){
   const unsigned count=Q38StateSnapshotData::Count(position+1,layer,kind);if(!count)continue;
   std::vector<uint32_t> words;assert(reference.Snapshot(position+1,layer,kind,words) && words.size()==count);
   assert(Q38StateSnapshotData::Save(stateDirectory.c_str(),false,position+1,layer,kind,words.data(),words.size()));records++;
  }
  if(eos && position==1)for(unsigned layer=0;layer<64;layer++)for(unsigned kind=0;kind<4;kind++){
   const unsigned count=Q38StateSnapshotData::EosCount(layer,kind);if(!count)continue;
   std::vector<uint32_t> words;assert(reference.SnapshotEos(layer,kind,words) && words.size()==count);
   assert(Q38StateSnapshotData::SaveEos(stateDirectory.c_str(),false,layer,kind,words.data(),words.size()));records++;
  }
 }
 if(snapshots){assert(records==176 && reference.Ready() && Q38StateSnapshotData::Finish(stateDirectory.c_str(),false,records));
  puts("PASS CPU-PRIVATE176 state snapshots/GDN after2+3/Conv after3/packed half K+V all3positions;NOT a native match or execution authority");}
 if(eos){assert(records==128 && reference.Ready() && Q38StateSnapshotData::FinishEos(stateDirectory.c_str(),false,records));
  puts("PASS CPU-PRIVATE128 EOS state records/ALL GDN+Conv+packed half K+V after EXACTLY2 forward steps;NO third step,device EOS execution or native match claimed");}
 printf("PASS CPU-ONLY original27B%u-position reference computed;MUST compare independently with complete emitted/device generations;NOT native/physical/Work/TPS qualification\n",eos?2:3);
}
