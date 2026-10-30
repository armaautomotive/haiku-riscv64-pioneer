/* SPDX-License-Identifier: MIT */
// Isolated emitted ABI contracts with SYNTHETIC metadata. No worker history,
// model product,completion/native owner or real ROOT execution is supplied.
#define PIONEER_Q38_LOCAL_SOURCE_INSTRUCTION_MACHINE_ONLY
#define PIONEER_Q38_LOCAL_SOURCE_PRECISE_ABI_TEST
#include "pioneer_q38_local_source_instruction_test.cpp"
struct AdmissionMachine:LocalSourceMachine {
 AdmissionMachine(const AuditImage& image,unsigned kind,unsigned worker,bool warm):LocalSourceMachine(image,kind,worker){
  if(!warm)return;
  memory[P::Nonces+worker*4]=1;peer[G::Doorbell]=1;
  peer[G::Status]=peer[G::Status+4]=1;
  std::array<uint32_t,64> boot={},job={},proof={};
  for(unsigned i=0;i<64;i++)boot[i]=peer.at(G::Boot+i*4);
  const unsigned h[]={G::Magic,1,1,worker+1,G::Peer(worker+1),0,0,2025};
  for(unsigned i=0;i<8;i++){job[i]=h[i];job[8+i]=boot[8+i];}
  const unsigned m[]={0x31483851,8,576,0,0,5120,0x04000000,1,PioneerQ38ProjectionRoute(0),1};
  for(unsigned i=0;i<10;i++)job[16+i]=m[i];job[26]=1;
  assert(G::JobShape(job.data(),boot.data(),0,0));
  proof[17]=1;proof[18]=worker+1;proof[19]=0x32504651;proof[30]=boot[16];proof[31]=G::Peer(worker+1);
  for(unsigned i=0;i<16;i++)proof[32+i]=boot[16+i];
  for(unsigned i=0;i<8;i++)proof[8+i]=peer.at(0xffb20000U+Counter(i));
  for(unsigned i=0;i<64;i++){peer[G::Job+i*4]=job[i];peer[0x83f00+i*4]=proof[i];}
 } // helper ABI data ONLY,not an initialized owner or actual closed job history
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
 AuditImage baseline(argv[1],argv[3]),candidate(argv[2],argv[4]);
 const auto oldEntry=Entries(argv[3]),newEntry=Entries(argv[4]);unsigned coldFaults=0,deferred=0,unseenPeers=0,unseenFaults=0;
 for(unsigned kind=0;kind<2;kind++){
  for(unsigned worker:{0U,95U,118U})for(bool warm:{false,true}){
   auto a=std::make_unique<AdmissionMachine>(baseline,kind,worker,warm),b=std::make_unique<AdmissionMachine>(candidate,kind,worker,warm);
   a->Call(oldEntry[2+kind]);b->Call(newEntry[2+kind]);assert(a->Run() && b->Run() && !a->peerWrites && !b->peerWrites);
   assert(a->sourceReads.size()==16 && b->sourceReads.size()==(warm?0U:16U)
    && a->packets.size()==b->packets.size()+(warm?16U:0U));
   if(!warm)assert(a->packets==b->packets && a->sourceReads==b->sourceReads);
   printf("PASS emitted admission%u worker%u/%s packets%zu->%zu/instructions%u->%u/stack%u/%u;SYNTHETIC ABI metadata,NOT ROOT/model/physical/TPS\n",
    kind,worker,warm?"warm":"cold",a->packets.size(),b->packets.size(),a->steps-2048,b->steps-2048,
    0x70000-a->minimumStack,0x70000-b->minimumStack);
  }
  // A peer's closed-looking sequence is NOT the ROOT's prior audit history.
  // Wait imports that sequence;the candidate must latch its own zero BEFORE
  // Wait and retain ALL16 source observations. These remain synthetic ABI
  // inputs,not a manufactured worker history or a native admission grant.
  for(unsigned worker:{0U,95U,118U}){
   auto a=std::make_unique<AdmissionMachine>(baseline,kind,worker,true),b=std::make_unique<AdmissionMachine>(candidate,kind,worker,true);
   a->memory[P::Nonces+worker*4]=b->memory[P::Nonces+worker*4]=0;
   a->Call(oldEntry[2+kind]);b->Call(newEntry[2+kind]);
   assert(a->Run() && b->Run() && !a->peerWrites && !b->peerWrites
    && a->packets==b->packets && a->sourceReads==b->sourceReads && b->sourceReads.size()==16
    && a->memory.at(P::Nonces+worker*4)==1 && b->memory.at(P::Nonces+worker*4)==1);unseenPeers++;
  }
  for(unsigned word=0;word<16;word++)for(bool warm:{false,true}){
   auto a=std::make_unique<AdmissionMachine>(baseline,kind,0,warm),b=std::make_unique<AdmissionMachine>(candidate,kind,0,warm);
   const unsigned at=0xffb20000U+Source(word);a->peer.at(at)^=1;b->peer.at(at)^=1;
   a->Call(oldEntry[2+kind]);b->Call(newEntry[2+kind]);assert(!a->Run() && !a->peerWrites);
   if(warm){assert(b->Run() && b->sourceReads.empty() && !b->peerWrites);deferred++;}
   else{assert(!b->Run() && a->packets==b->packets && a->sourceReads==b->sourceReads && !b->peerWrites);coldFaults++;}
  }
  for(unsigned word=0;word<16;word++){
   auto a=std::make_unique<AdmissionMachine>(baseline,kind,0,true),b=std::make_unique<AdmissionMachine>(candidate,kind,0,true);
   a->memory[P::Nonces]=b->memory[P::Nonces]=0;
   const unsigned at=0xffb20000U+Source(word);a->peer.at(at)^=1;b->peer.at(at)^=1;
   a->Call(oldEntry[2+kind]);b->Call(newEntry[2+kind]);
   assert(!a->Run() && !b->Run() && !a->peerWrites && !b->peerWrites
    && a->packets==b->packets && a->sourceReads==b->sourceReads && b->sourceReads.size()==word+1
    && a->memory.at(P::Nonces)==1 && b->memory.at(P::Nonces)==1);unseenFaults++;
  }
 }
 printf("PASS emitted QGW2 admission/12 cold+warm controls/%u unseen-peer controls retain ALL16 audits/%u retained cold source failures/%u unseen-peer source failures despite imported sequence/%u EXPLICIT deferred warm source observations/NO release packets/inert2048 each;actual worker-local faults and connected ROOT+model+physical remain SEPARATE\n",unseenPeers,coldFaults,unseenFaults,deferred);
}
