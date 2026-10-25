/* SPDX-License-Identifier: MIT */
// Reuse the actual full arithmetic machine, not invented success receipts.
#define PIONEER_Q38_RESIDENT_OUTPUT_COHORT_MACHINE_ONLY
#include "pioneer_q38_resident_output_cohort_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputProof.h"
static unsigned refusals=0;
template<class Check> static void Mutate(uint32_t* data,unsigned n,Check check){
 for(unsigned i=0;i<n;i++){data[i]^=1;assert(!check());data[i]^=1;assert(check());refusals++;}
}
struct CompletionSnapshot {
 uint32_t rootBefore[8],before[119][8];
 explicit CompletionSnapshot(OutputCohortFabric& f){
  for(unsigned i=0;i<8;i++)rootBefore[i]=f.counts[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<8;i++)before[w][i]=f.peers[w]->Count(i);
 }
};
static void VerifyCompletion(OutputCohortFabric& f,const CompletionSnapshot& snapshot,bool mutation){
 uint32_t stats[64],matrix[64];
 for(unsigned i=0;i<64;i++){stats[i]=f.Mem(TT_Q38_COHORT_STATS+i*4);matrix[i]=f.Mem(0x7f800+i*4);}
 auto root=[&](){return PioneerQ38ResidentOutputRootReceipt(f.manifest.data(),f.post.data(),snapshot.rootBefore,f.counts,stats,matrix,f.command);};
 if(!root()){fprintf(stderr,"ROOT_RECEIPT stats");for(unsigned v:stats)fprintf(stderr," %u",v);fprintf(stderr,"\nmatrix");for(unsigned v:matrix)fprintf(stderr," %u",v);fprintf(stderr,"\n");}
 assert(root());
 uint32_t status[]={f.Mem(0x17ffc0),f.Mem(0x17ffc4),f.Mem(0x17ffc8)},pc[]={f.pc,f.pc},reset=0x46800;
 auto physicalRoot=[&](){return PioneerQ38ResidentOutputRootPhysical(status,pc,reset);};assert(physicalRoot());
 if(mutation){
  Mutate(status,3,physicalRoot);Mutate(pc,2,physicalRoot);reset=0x47800;assert(!physicalRoot());reset=0x46800;refusals++;
  for(unsigned first:{0x72038U,0x7203cU})for(unsigned second:{0x72038U,0x7203cU}){
   pc[0]=first;pc[1]=second;assert(physicalRoot());
  }
  for(unsigned bad:{0x72034U,0x72040U,0x7205cU,0x72060U,0x7207cU,0x72080U})for(unsigned slot=0;slot<2;slot++){
   pc[0]=pc[1]=f.pc;pc[slot]=bad;assert(!physicalRoot());refusals++;
  }pc[0]=pc[1]=f.pc;
  // Cycle values are diagnostics; their framing/nonzero requirement is
  // tested separately. No claim that arbitrary timing bit changes fail.
  Mutate(stats,64,root);Mutate(matrix,57,root);Mutate(f.command,10,root);
  matrix[57]^=1;assert(!root());matrix[57]^=1;refusals++;
  for(unsigned phase=0;phase<3;phase++){
   auto lo=matrix[58+phase*2],hi=matrix[59+phase*2];matrix[58+phase*2]=matrix[59+phase*2]=0;
   assert(!root());matrix[58+phase*2]=lo;matrix[59+phase*2]=hi;refusals++;
  }
 }
 for(unsigned w=1;w<120;w++){
  auto& peer=*f.peers[w-1];uint32_t after[8],command[10],receipt[64],matrixBefore[8];
  uint32_t first[32]={},recurrent[32]={},gated[32]={},clock[16]={};
  for(unsigned i=0;i<8;i++){after[i]=peer.Count(i);matrixBefore[i]=f.Mem(TT_Q38_COHORT_COUNTERS+(w-1)*32+i*4);}
  for(unsigned i=0;i<10;i++)command[i]=peer.command[i];
  for(unsigned i=0;i<64;i++)receipt[i]=peer.memory.at(0x83f00+i*4);
  if(w<=96){
   for(unsigned i=0;i<32;i++){
    first[i]=peer.memory.at(TT_Q38_GDN_INPUT_SUMMARY+i*4);recurrent[i]=peer.memory.at(TT_Q38_RGDN_SUMMARY+i*4);gated[i]=peer.memory.at(TT_Q38_GDN_GATED_SUMMARY+i*4);
   }
   for(unsigned i=0;i<16;i++)clock[i]=peer.memory.at(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4);
  }
  auto check=[&](){return PioneerQ38ResidentOutputPeerReceipt(f.manifest.data(),f.post.data(),w,snapshot.before[w-1],after,command,receipt,matrixBefore,first,recurrent,gated,clock);};
  if(!check())fprintf(stderr,"PEER_RECEIPT_FAILURE worker%u\n",w);assert(check());
  uint32_t boot[8],wait[16],pcs[]={peer.pc,peer.pc};
  for(unsigned i=0;i<8;i++)boot[i]=peer.memory.at(0x74040+i*4);
  for(unsigned i=0;i<16;i++)wait[i]=peer.memory.at(0x74140+i*4);
  unsigned doorbell=peer.memory.at(0x74100),peerReset=0x46800;
  auto physical=[&](bool stop){return PioneerQ38ResidentOutputPeerPhysical(f.manifest.data(),w,peer.source,boot,wait,doorbell,pcs,peerReset,stop);};
  assert(physical(false));
  if(mutation && (w==1 || w==24 || w==25 || w==96 || w==97 || w==119)){
   Mutate(receipt,64,check);Mutate(after,8,check);Mutate(command,10,check);Mutate(matrixBefore,8,check);
   if(w<=96){Mutate(first,32,check);Mutate(recurrent,32,check);Mutate(gated,32,check);
    // Valid elapsed times may vary. Mutate the bound identity fields,
    // then reject zero/non-monotonic/expired timestamps independently.
    for(unsigned i:{0U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U}){clock[i]^=1;assert(!check());clock[i]^=1;refusals++;}
    uint32_t old=clock[3];clock[3]=clock[1];uint32_t high=clock[4];clock[4]=clock[2];assert(!check());clock[3]=old;clock[4]=high;refusals++;
   }
   auto waiting=[&](){return physical(false);};Mutate(boot,8,waiting);Mutate(wait,16,waiting);Mutate(peer.source,16,waiting);
   doorbell++;assert(!waiting());doorbell--;peerReset=0x47800;assert(!waiting());peerReset=0x46800;refusals+=2;
  }
  // Actual emitted STOP on the SAME peer and tensor state, never a reset
  // or manufactured terminal. STOP must preserve commands/counter lineage.
  const unsigned sequence=PioneerQ38ResidentOutputSequence(f.manifest.data(),w);
  assert(peer.Stop(sequence+1));doorbell=peer.memory.at(0x74100);pcs[0]=pcs[1]=peer.pc;
  for(unsigned i=0;i<16;i++)wait[i]=peer.memory.at(0x74140+i*4);
  assert(physical(true) && !physical(false) && check());
  for(unsigned i=0;i<8;i++)assert(after[i]==peer.Count(i));
  for(unsigned i=0;i<10;i++)assert(command[i]==peer.command[i]);
  if(mutation && w==119){auto stopped=[&](){return physical(true);};Mutate(wait,16,stopped);
   pcs[0]=TT_Q38_PEER_FAULT_PARK;assert(!stopped());pcs[0]=peer.pc;peerReset=0x47800;assert(!stopped());refusals+=2;}
 }
}
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 unsigned variant=unsigned(atoi(argv[3]));assert(variant<3);
 const unsigned layers[]={0,62,30},positions[]={0,8191,5},banks[]={0,6,3};
 OutputParts parts(layers[variant],positions[variant]);OutputCohortFabric f(argv[1],argv[2],parts,banks[variant],variant&1);
 auto snapshot=std::make_unique<CompletionSnapshot>(f);assert(f.Run());f.Verify();f.Inert();
 // Execute the ACTUAL emitted next-fetch instruction too. It must return
 // to the safe architectural park without any NoC read or packet.
 unsigned packets=f.packets,reads=f.nocReads;assert(f.pc==0x72038);f.pc=0x7203c;f.StepRoot();
 assert(f.pc==0x72038 && f.packets==packets && f.nocReads==reads);
 VerifyCompletion(f,*snapshot,variant==0);
 printf("PASS complete resident-output independent ROOT+ALL119 pre-boot-to-final counter lineage/96 actual GDN summary+clock chains/ALL5120 outputs/ACTUAL119 STOP terminals/NEW dual safe park72038+7203c versus disjoint fault72060/metadataRefusals%u variant%u;NO CPU products/injected success/reset/restore/native grant/throughput claim\n",refusals,variant);
}
