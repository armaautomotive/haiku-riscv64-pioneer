/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ROOT_CYCLE_IMAGE_MODEL_ONLY
#include "pioneer_q38_root_cycle_image_test.cpp"
// Independent fixed proof fixture. Only the FOUR-tile DRAM packet shape
// changes. ALL sources/nonces/physical PCs/NoC/DMA/outputs remain observed.
struct Batch4ProofFabric:DispatchMachine {
 using DispatchMachine::DispatchMachine;
 std::array<uint32_t,64> Proof(unsigned worker)const override {
  auto p=DispatchMachine::Proof(worker);const auto& d=peers.at(worker-1).descriptor;
  if(d[14]) {
   unsigned batch=d[1]==8?4:2,blocks=d[5]>>(d[1]==8?5:8),reads=d[7]*blocks/batch+(d[5]*2+16383)/16384+1;
   assert(!(blocks%batch));
   for(unsigned c:{0U,2U,3U,6U})p[8+c]=p[c]+reads;
   p[20]-=(batch-1)*d[2];p[23]=0x98000;p[28]*=batch;
  }
  return p;
 }
};
int main(int argc,char** argv) {
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned n:{1U,2U,7U,119U})for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}) {
  CycleRoot<DispatchMachine> old(argv[1],n,type,k);CycleRoot<Batch4ProofFabric> next(argv[2],n,type,k);
  assert(old.RunRoot() && next.RunRoot());old.CheckCycles();next.CheckCycles();old.VerifyCompactHost();next.VerifyCompactHost();
  assert(old.host==next.host && old.packets==next.packets && old.dmaPackets==next.dmaPackets && old.nocReads==next.nocReads && old.gathers==next.gathers);
  for(unsigned field=0;field<57;field++)assert(old.memory.at(0x7f800+field*4)==next.memory.at(0x7f800+field*4));
  old.Configure(2);next.Configure(2);assert(old.RunRoot() && next.RunRoot());
  for(unsigned field=0;field<64;field++)assert(old.memory.at(0x7f800+field*4)==next.memory.at(0x7f800+field*4));
  old.active=next.active=119;old.Configure(3,true);next.Configure(3,true);assert(old.RunRoot() && next.RunRoot());
 }
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}) {
  // Both crossed pairs must refuse,not silently accept the other wire.
  CycleRoot<DispatchMachine> wrongNew(argv[2],7,type,k);assert(!wrongNew.RunRoot() && wrongNew.unknown && !wrongNew.dmaPackets);wrongNew.VerifyFaultInert();
  CycleRoot<Batch4ProofFabric> wrongOld(argv[1],7,type,k);assert(!wrongOld.RunRoot() && wrongOld.unknown && !wrongOld.dmaPackets);wrongOld.VerifyFaultInert();
 }
 for(unsigned field=0;field<64;field++) {
  CycleRoot<Batch4ProofFabric> m(argv[2],7);m.corruptWorker=7;m.corruptProof=field;
  assert(!m.RunRoot() && m.unknown && !m.dmaPackets && !m.memory.at(0x7f8e4));m.VerifyFaultInert();
 }
 for(unsigned worker:{1U,3U,7U}) {
  CycleRoot<Batch4ProofFabric> m(argv[2],7);m.badPcWorker=worker;
  assert(!m.RunRoot() && m.unknown && !m.dmaPackets);m.VerifyFaultInert();
 }
 for(unsigned packet:{1U,20U,40U}) {
  CycleRoot<Batch4ProofFabric> m(argv[2],7);m.faultAt=packet;
  assert(!m.RunRoot() && m.unknown && m.pending && m.packets==packet);m.VerifyFaultInert();
 }
 for(unsigned packet:{1U,4U,8U}) {
  CycleRoot<Batch4ProofFabric> m(argv[2],119);m.failDma=packet;
  assert(!m.RunRoot() && m.unknown && m.pending && m.dmaPackets==packet && !m.memory.at(0x7f8e4));m.VerifyFaultInert();
 }
 for(unsigned type:{8U,12U,14U}) {
  CycleRoot<CooperativeDispatch> m(argv[2],argv[3],2,type,5120);assert(m.RunRoot());m.CheckCycles();
  for(auto& p:m.actual)assert(p->CorrectRows() && p->copyReads==p->copyWrites && !p->dotProbeReads);
 }
 puts("PASS OFFLINE ACTUAL batch4 root:108 Work/checked/STOP shape cases,ALL57 safety fields/output/root NoC/DMA equivalent,18 crossed-wire pair refusals,ALL64 current peer-proof faults/physical PC/NoC/first-middle-last DMA failures inert;actual coalesced Q8/Q4/Q6 peer arithmetic exact;NO private controller/source/native/speed qualification");
}
