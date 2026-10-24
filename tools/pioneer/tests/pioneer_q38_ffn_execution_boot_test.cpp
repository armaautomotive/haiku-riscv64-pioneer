/* SPDX-License-Identifier: MIT */
// Small actual QFP2 boot regression for the NEW byte-execution bridge.
// No native owner,completed producer,weights or hardware access is granted.
#include "pioneer_q38_ffn_residual_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnResidualGraphImages.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <map>
struct BootFabric:ProjectionExecutionFabric {
 std::map<std::pair<unsigned,uint32_t>,uint32_t> writes;
 std::array<std::array<uint32_t,10>,120> commands={};
 std::array<std::array<uint32_t,8>,120> counts={};
 std::vector<uint32_t> workspace=std::vector<uint32_t>(524288,0x7fc00001);
 unsigned transfers=0,localWrites=0;
 static unsigned Route(unsigned w){const unsigned x[]={1,2,3,4,5,6,7,10,11,12,13,14};return x[w/10]|((w%10+2)<<6);}
 BootFabric(){for(unsigned w=0;w<120;w++){commands[w][5]=Route(w);for(unsigned i=0;i<8;i++)counts[w][i]=UINT32_MAX-15-i;counts[w][6]=counts[w][2];}}
 uint32_t ReadLocal(unsigned w,uint32_t a)override{
  assert(w<120 && PioneerQ38ProjectionGraphSnapshotContains(w,a));auto found=writes.find({w,a});if(found!=writes.end())return found->second;
  if(PioneerQ38ProjectionGraphProgramAddress(w,a))return PioneerQ38FfnGraphProgramAddress(w,a)?PioneerQ38FfnResidualProgramWord(w,a):0;
  if(a>=0x74000 && a<0x74180){unsigned i=(a-0x74000)/4;
   const unsigned b[]={0x32504651,w,Route(w),0x16005,0,0,0,0};return i>=16 && i<24?b[i-16]:i>=80?0xdeadbeef:0;
  }
  return 0xa5a50000^a^w;
 }
 void WriteLocal(unsigned w,uint32_t a,uint32_t v)override{assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));writes[{w,a}]=v;localWrites++;}
 uint32_t NoC(unsigned w,unsigned a)override{
  const unsigned source[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};
  for(unsigned i=0;i<16;i++)if(a==source[i])return i?0x12340000+i:0x16005;
  const unsigned off[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)if(a==off[i])return counts[w][i];
  if(a<40 && !(a&3))return commands[w][a/4];assert(false);return 0;
 }
 void SetControls(unsigned w,const uint32_t* cmd,const uint32_t* c)override{
  for(unsigned i=0;i<10;i++)commands[w][i]=cmd[i];
  for(unsigned i=0;i<8;i++){if(c[i]!=counts[w][i])transfers++;counts[w][i]=c[i];}
 }
 uint32_t Reset(unsigned)override{return 0x46800;}
 std::vector<uint32_t>& Gddr()override{return workspace;}
 ProjectionExecutionLearnedBytes* LearnedBytes()override{assert(false && "boot must not request learned bytes");return nullptr;}
};
int main(int argc,char** argv){
 assert(argc==3);BootFabric f;auto original=f.workspace;FfnResidualExecutionMachine vm(argv[1],argv[2],f);
 for(unsigned w:{1U,64U,119U}){
  assert(f.ReadLocal(w,0x7414c)==0xdeadbeef);auto counters=f.counts[w];vm.Start(w);
  assert(f.ReadLocal(w,0x7414c)==0xdeadbeef); // constructor cannot forge boot
  for(unsigned i=0;i<32 && f.ReadLocal(w,0x7414c)==0xdeadbeef;i++)vm.Advance(w);
  assert(f.ReadLocal(w,0x7414c)==0x32504651 && !f.ReadLocal(w,0x74140) && !f.ReadLocal(w,0x74144)
   && vm.Pc(w)==0x7204c && f.counts[w]==counters);
 }
 const auto bootWrites=f.localWrites;const auto bootBytes=f.writes;
 for(unsigned i=0;i<32;i++)for(unsigned w:{1U,64U,119U})vm.Advance(w);
 assert(f.localWrites==bootWrites && f.writes==bootBytes); // idle WAIT cannot invent endpoint writes
 assert(!f.transfers && f.workspace==original && !vm.Matrices() && !vm.Producers() && !vm.Stops());
 puts("PASS actual installed QFP2 bridge3freshBOOTs/poison markers unchanged until emitted instructions/WAIT+private sequence0/96idle polls ZERO endpoint writes/NoC counters+GDDR unchanged/no learned bytes or synthetic ready;NO native/physical/TPS authority");
}
