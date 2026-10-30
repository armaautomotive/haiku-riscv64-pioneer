/* SPDX-License-Identifier: MIT */
// Synthetic HOST DATA only. No model/reference,firmware,native or device IO.
#define PIONEER_Q38_TEST_OBSERVED_TRACE 1
#include "PioneerQ38EosStateDataBridge.h"
#include "pioneer_q38_state_snapshot_data.h"
#include <cassert>
#include <vector>
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);using namespace Q38StateSnapshotData;
 unsigned records=0,refused=0;
 for(unsigned layer=0;layer<65;layer++)for(unsigned kind=0;kind<5;kind++){
  const unsigned count=EosCount(layer,kind);
  const bool wanted=layer<64 && kind<4 && (layer%4==3?kind>=2:kind<2);
  assert(bool(count)==wanted);
  // Existing normal corpus restrictions/176-record shape are unchanged.
  assert(Count(2,layer,kind)==(wanted && kind==0?786432U:0U));
  assert(Count(3,layer,kind)==(wanted?kind==0?786432U:kind==1?30720U:1536U:0U));
  if(!wanted){uint32_t value=0;assert(!SaveEos(argv[3],false,layer,kind,&value,1));refused++;continue;}
  std::vector<uint32_t> values(count);
  for(unsigned i=0;i<count;i++)values[i]=kind<2?(i+layer)%3==0?0x3f800000:(i+layer)%3==1?0xbf000000:0:
   ((i+layer)%3==0?0x3c000000:(i+layer)%3==1?0x0000b800:0);
  assert(SaveEos(argv[1],false,layer,kind,values.data(),count));
  assert(SaveQ38ObservedEosStateData(argv[2],layer,kind,values.data(),count));records++;
  assert(!SaveQ38ObservedEosStateData(argv[2],layer,kind,values.data(),count));refused++;
  assert(!SaveEos(argv[3],false,layer,kind,nullptr,count));refused++;
  assert(!SaveEos(argv[3],false,layer,kind,values.data(),count-1));refused++;
  values[0]=kind<2?0x7f800000:0x7c000000;
  assert(!SaveEos(argv[3],false,layer,kind,values.data(),count));refused++;
  if(kind>=2){values[0]=0x00007c00;assert(!SaveEos(argv[3],false,layer,kind,values.data(),count));refused++;}
 }
 assert(records==128 && !FinishEos(argv[3],false,127) && !FinishEos(argv[3],false,129));refused+=2;
 assert(FinishEos(argv[1],false,records) && FinishQ38ObservedEosStateData(argv[2],records));
 assert(!FinishQ38ObservedEosStateData(argv[2],records));refused++;
 printf("PASS SYNTHETIC EOS128 state DATA records/all2-position shapes+distinct QCE1/QNE1+normal count unchanged/%u invalid shape,null,length,nonfinite,duplicate,marker refusals;NO model,actual EOS,STOP,native END,Work or TPS claim\n",refused);
}
