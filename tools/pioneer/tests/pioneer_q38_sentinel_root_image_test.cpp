/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ROOT_CYCLE_IMAGE_MODEL_ONLY
#include "pioneer_q38_root_cycle_image_test.cpp"
// Independent progress follows fabricated ROOT instruction steps,NOT the
// number of times a worker is polled. It is NOT a hardware timing model.
struct TimedPollFabric:DispatchMachine {
 using DispatchMachine::DispatchMachine;
 uint64_t readyAt[119]={};unsigned pcReads[119]={},resetReads[119]={};
 unsigned lateWorker=0,badResetWorker=0;
 void Packet()override {
  bool write=command[7]==0x212;unsigned worker=Worker(write?command[5]:command[2]);
  auto remote=write?command[3]:command[0],local=write?command[0]:command[3];
  if(!write && remote==0x74140 && peers[worker-1].launched) {
   auto& p=peers[worker-1];p.polls=0;
   p.finished=steps>=readyAt[worker-1]?p.requested:p.requested-1;
  }
  DispatchMachine::Packet();
  if(write && remote==0x74100)readyAt[worker-1]=uint64_t(steps)+200000+peers[worker-1].descriptor[7]*10000+(worker==lateWorker?200000:0);
  if(!write && remote==0xffb13140)pcReads[worker-1]++;
  if(!write && remote==0xffb121b0){resetReads[worker-1]++;if(worker==badResetWorker)memory[local]=0x46801;}
 }
};
int main(int argc,char** argv) {
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned n:{1U,2U,7U,119U})for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}) {
  CycleRoot<DispatchMachine> old(argv[1],n,type,k),next(argv[2],n,type,k);
  assert(old.RunRoot() && next.RunRoot());old.CheckCycles();next.CheckCycles();old.VerifyCompactHost();next.VerifyCompactHost();
  assert(old.host==next.host && old.packets==next.packets && old.dmaPackets==next.dmaPackets
   && old.nocReads==next.nocReads && old.gathers==next.gathers);
  // In this poll-count fixture,the scheduling order legitimately changes
  // but EACH worker needs exactly the same number of status reads. All57
  // safety fields/packets/DMA must therefore remain bit-identical.
  for(unsigned field=0;field<57;field++)assert(old.memory.at(0x7f800+field*4)==next.memory.at(0x7f800+field*4));
  unsigned reads=unsigned(next.reads.size());
  old.Configure(2);next.Configure(2);assert(old.RunRoot() && next.RunRoot());
  assert(next.reads.size()==reads && old.host==next.host);
  for(unsigned field=0;field<64;field++)assert(old.memory.at(0x7f800+field*4)==next.memory.at(0x7f800+field*4));
  old.active=next.active=119;old.Configure(3,true);next.Configure(3,true);assert(old.RunRoot() && next.RunRoot());
  assert(next.reads.size()==reads);
  for(unsigned field=0;field<64;field++)assert(old.memory.at(0x7f800+field*4)==next.memory.at(0x7f800+field*4));
 }
 for(unsigned workers:{2U,7U,119U})for(unsigned late:{0U,1U}) {
  CycleRoot<TimedPollFabric> old(argv[1],workers),next(argv[2],workers);
  old.lateWorker=next.lateWorker=late;assert(old.RunRoot() && next.RunRoot());
  old.CheckCycles();next.CheckCycles();old.VerifyCompactHost();next.VerifyCompactHost();assert(old.host==next.host);
  for(unsigned w=0;w<workers;w++)assert(next.pcReads[w]==2 && next.resetReads[w]==1 && next.peers[w].finished==next.peers[w].requested);
  printf("FABRICATED_PROGRESS workers%u non_sentinel_late%u status_polls%u/%u;NOT_SILICON_OR_SPEED\n",workers,late,old.memory.at(0x7f828),next.memory.at(0x7f828));
 }
 for(unsigned field=0;field<64;field++) {
  CycleRoot<DispatchMachine> m(argv[2],7);m.corruptWorker=7;m.corruptProof=field;
  assert(!m.RunRoot() && m.unknown && !m.dmaPackets && !m.memory.at(0x7f8e4));m.VerifyFaultInert();
 }
 for(unsigned worker:{1U,3U,7U}) {
  CycleRoot<TimedPollFabric> m(argv[2],7);m.badPcWorker=worker;
  assert(!m.RunRoot() && m.unknown && !m.dmaPackets);m.VerifyFaultInert();
  CycleRoot<TimedPollFabric> reset(argv[2],7);reset.badResetWorker=worker;
  assert(!reset.RunRoot() && reset.unknown && !reset.dmaPackets);reset.VerifyFaultInert();
 }
 for(unsigned packet:{1U,20U,40U}) {
  CycleRoot<DispatchMachine> m(argv[2],7);m.faultAt=packet;
  assert(!m.RunRoot() && m.unknown && m.pending && m.packets==packet);m.VerifyFaultInert();
 }
 for(unsigned packet:{1U,4U,8U}) {
  CycleRoot<DispatchMachine> m(argv[2],119);m.failDma=packet;
  assert(!m.RunRoot() && m.unknown && m.pending && m.dmaPackets==packet && !m.memory.at(0x7f8e4));m.VerifyFaultInert();
 }
 for(bool large:{false,true}) {
  CycleRoot<DispatchMachine> m(argv[2],7,12,17408);m.rollover=true;m.large=large;
  assert(m.RunRoot());m.CheckCycles();assert(m.reads.size()==15);
 }
 for(bool backward:{false,true}) {
  CycleRoot<DispatchMachine> m(argv[2],7);m.unstable=!backward;m.backward=backward;
  assert(!m.RunRoot() && m.unknown && !m.memory[0x7f8e4]);m.VerifyFaultInert();
 }
 for(unsigned type:{8U,12U,14U}) {
  CycleRoot<CooperativeDispatch> m(argv[2],argv[3],2,type,5120);assert(m.RunRoot());m.CheckCycles();
  for(auto& peer:m.actual)assert(peer->CorrectRows() && !peer->dotProbeReads);
 }
 puts("PASS OFFLINE actual sentinel root:108 shape/worker/Work/checked/STOP equivalence cases,ALL57 safety fields and exact packet/DMA output in original fixture,independent progress with NON-sentinel late workers still audits ALL,two physical PCs/reset each,ALL64 proof corruptions/PC/reset/NoC/counter uncertainty inert,actual peer arithmetic exact;NO NATIVE OR SPEED QUALIFICATION");
}
