/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ROOT_CYCLE_IMAGE_MODEL_ONLY
#include "pioneer_q38_root_cycle_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}){
  printf("RAW_CACHE_ROOT type%u K%u\n",type,k);
  CycleRoot<CooperativeDispatch> m(argv[1],argv[2],2,type,k);
  for(unsigned trial=1;trial<=2;trial++){
   if(trial==2){for(auto& p:m.actual)p->NextPackets();m.Configure(2);m.DmaHeader();m.Bind();m.reads.clear();m.dmaPackets=0;std::fill(m.host.begin(),m.host.end(),0xdeadbeef);}
   assert(m.RunRoot());m.CheckCycles();m.VerifyCompactHost();
   for(auto& p:m.actual)assert(p->CorrectRows()&&!p->gate&&p->publications==unsigned(type==12?trial*p->groups*(k/32):0)
    &&!p->dotProbeReads&&!p->pending&&p->pendingAccumulator.empty());
  }
 }
 for(unsigned fault:{1U,5U,140U}){
  CycleRoot<CooperativeDispatch> m(argv[1],argv[2],2,12,17408);m.actual[1]->faultAt=fault;
  assert(!m.RunRoot()&&m.unknown&&m.actual[1]->pending&&!m.gathers&&!m.dmaPackets);
  unsigned requests=m.actual[1]->requests,publications=m.actual[1]->publications;
  m.VerifyFaultInert();assert(m.actual[1]->requests==requests&&m.actual[1]->publications==publications);
 }
 puts("PASS OFFLINE ACTUAL sentinel coordinator+raw-Dst cache peers:ALL9type/K twice,exact FP32+FP64/all original proof/PC/source/NoC/compactDMA observers;first/middle/last peer failures UNKNOWN/inert;NO kernel/native/speed qualification");
}
