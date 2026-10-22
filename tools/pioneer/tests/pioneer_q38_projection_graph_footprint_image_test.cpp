/* SPDX-License-Identifier: MIT */
// ACTUAL ROOT instructions/packet endpoints against fixed snapshot DATA.
// Synthetic peers/transport,NOT actual native mappings or an IO authority.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_PROJECTION_PARENT_MACHINE_ONLY
#include "pioneer_q38_projection_graph_image_test.cpp"
struct FootprintParent:ProjectionParentMachine {
 uint64_t cpuStores=0,localPacketViews=0,peerPacketViews=0;
 using ProjectionParentMachine::ProjectionParentMachine;
 void Store(uint32_t a,uint32_t value)override{
  if(a<0xff000000){
   assert(PioneerQ38ProjectionGraphSnapshotContains(0,a) && !PioneerQ38ProjectionGraphProgramImmutable(0,a));cpuStores++;
  }
  ProjectionParentMachine::Store(a,value);
 }
 void Packet()override{
  bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0];
  unsigned bytes=command[8],route=write?command[5]:command[2];assert(bytes && !(bytes&3) && local<=UINT32_MAX-bytes);
  assert(PioneerQ38ProjectionGraphSnapshotContains(0,local) && PioneerQ38ProjectionGraphSnapshotContains(0,local+bytes-4));
  assert(!PioneerQ38ProjectionGraphProgramAddress(0,local) && !PioneerQ38ProjectionGraphProgramAddress(0,local+bytes-4));localPacketViews++;
  if(route!=fixture.graph->Manifest()[8] && remote<0xff000000){
   unsigned w=Worker(route);assert(remote<=UINT32_MAX-bytes && PioneerQ38ProjectionGraphSnapshotContains(w,remote)
    && PioneerQ38ProjectionGraphSnapshotContains(w,remote+bytes-4));
   assert(!PioneerQ38ProjectionGraphProgramAddress(w,remote) && !PioneerQ38ProjectionGraphProgramAddress(w,remote+bytes-4));peerPacketViews++;
  }
  ProjectionParentMachine::Packet();
 }
};
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned examples=0;uint64_t stores=0,local=0,peer=0;
 const unsigned cases[][2]={{0,10},{3,6},{0,5},{0,13},{0,2},{0,4},{3,10},{64,0},{64,1}};
 for(const auto& c:cases){
  auto m=std::make_unique<FootprintParent>(argv[1],c[0],c[1],examples%7);assert(m->Run());m->Verify();
  assert(m->cpuStores && m->localPacketViews && m->peerPacketViews);stores+=m->cpuStores;local+=m->localPacketViews;peer+=m->peerPacketViews;examples++;
 }
 {auto m=std::make_unique<FootprintParent>(argv[1],0,10,0,true);assert(m->Run());m->Verify();
  stores+=m->cpuStores;local+=m->localPacketViews;peer+=m->peerPacketViews;examples++;}
 printf("PASS fixed complete L1 snapshot DATA covers ACTUAL ROOT examples%u/CPUstores%llu/localNoCviews%llu/peerL1NoCviews%llu;ALL widths/types/small3+64/119/BOTH9wave globals/STOP,no code endpoints;SYNTHETIC_PEERS_NOT_NATIVE_MAPPING_CSR_TENSIX_OWNERSHIP_OR_SPEED\n",
  examples,(unsigned long long)stores,(unsigned long long)local,(unsigned long long)peer);
}
