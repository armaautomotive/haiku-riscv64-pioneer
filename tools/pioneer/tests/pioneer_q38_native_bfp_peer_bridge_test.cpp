/* SPDX-License-Identifier: MIT */
// Offline endpoint-only test. No worker CPU, model, driver or card runs here.
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#define PIONEER_Q38_BFP8_WORKER_DIAGNOSTIC 1
#include "pioneer_q38_native_bfp_peer_bridge.h"
#include "../PioneerQ38Bfp8WorkerCompute.h"
#include "../PioneerBlackholeBlockFloat.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

struct Endpoints {
 std::vector<uint32_t> root=std::vector<uint32_t>(0x180000/4),peer=std::vector<uint32_t>(0x180000/4);
 std::vector<uint8_t> weights=std::vector<uint8_t>(174080);
 uint32_t csr[0x2000/4]={};unsigned reads=0,events=0;
 Q38NativePeerTestBytes Bytes(){Q38NativePeerTestBytes b;b.context=this;
  b.word=[](void* p,unsigned worker,uint32_t address)->uint32_t&{auto& e=*static_cast<Endpoints*>(p);
   assert(!(address&3) && address<0x180000 && worker<2);return (worker?e.peer:e.root)[address/4];};
  b.noc=[](void* p,unsigned worker,unsigned offset){auto& e=*static_cast<Endpoints*>(p);assert(worker==1 && !(offset&3) && offset<0x2000);return e.csr[offset/4];};
  b.read=[](void* p,unsigned route,uint32_t address,uint8_t* out,unsigned n){auto& e=*static_cast<Endpoints*>(p);
   if(route!=786 || address<0x04000000 || !out || !n || n>16384 || address-0x04000000+n>174080)return false;
   std::memcpy(out,e.weights.data()+(address-0x04000000),n);e.reads++;return true;};
  b.events=[](void* p,unsigned worker,const uint32_t* d,const uint32_t* cmd,unsigned mask,bool changed,bool pending){auto& e=*static_cast<Endpoints*>(p);
   assert(worker==1 && d[2]==d[6]);const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(i!=6)e.csr[offsets[i]/4]+=d[i];
   for(unsigned i=0;i<10;i++)if(mask&(1U<<i))e.csr[i]=cmd[i];if(changed)e.csr[0x40/4]=pending;e.events++;
  };return b;
 }
};
struct LoaderIO {
 Q38NativeBfpPeerBridge& bridge;
 unsigned Read(unsigned a){return bridge.Read(a);}void Write(unsigned a,unsigned v){bridge.Write(a,v);}void Fence(){}
};
struct TensorIO {
 Q38NativeBfpPeerBridge& bridge;
 void Op(unsigned v){bridge.Write(0xffe40000,v);}void Config(unsigned i,unsigned v){bridge.Write(0xffef0000+i*4,v);}
 void Drain(){bridge.Write(0xffe40000,0xa2101816);bridge.Write(0xffe40000,0x4500017e);
  assert(bridge.Read(0xffe000fc)==1);bridge.Write(0xffe000fc,0);}
};
int main(){
 using W=PioneerQ38Bfp8WorkerJob;W::Job job={{W::Magic,W::Version,5120,0x04000000,1,786,1,W::Existing::Peer(1),
  PioneerQ38Bf16Wire::Magic,W::RootInput,W::RootRoute,2,W::Output,0,0,0}};assert(W::Shape(job));
 Endpoints endpoint;const unsigned counters[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};uint32_t initial[8];
 for(unsigned i=0;i<8;i++)endpoint.csr[counters[i]/4]=UINT32_MAX-80;
 for(unsigned i=0;i<8;i++)initial[i]=endpoint.csr[counters[i]/4];
 for(unsigned i=0;i<2560;i++)endpoint.root[(W::RootInput/4)+i]=0x3f803f80;
 for(unsigned tile=0;tile<160;tile++){
  float values[1024];for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++)
   values[row*32+col]=float(int((row*7+tile*3+col*5)%17)-8)*0.0625f;
  assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,values,1024,endpoint.weights.data()+tile*1088,1088));
 }
 const auto bytes=endpoint.Bytes();Q38NativeBfpPeerBridge bridge(job.words,1,bytes);LoaderIO io{bridge};
 PioneerQ38Bfp8WorkerLoader loader;assert(loader.Run(io,job,0) && endpoint.reads && endpoint.events);
 TensorIO tensor{bridge};assert(PioneerQ38Bfp8WorkerCompute::Compute(tensor,5120) && bridge.Clean() && bridge.MatrixInstructions()>0);
 for(unsigned i=0;i<8;i++)assert(bridge.CounterDelta(i)==uint32_t(endpoint.csr[counters[i]/4]-initial[i]));
 for(unsigned i=0;i<10;i++)assert(bridge.LastCommand(i)==endpoint.csr[i]);
 for(unsigned row=0;row<32;row++){
  const unsigned dst=row<16?32:40,col=row%16;const uint32_t bits=bridge.Read(0xffbd8000+dst*64+col*4);
  float actual;std::memcpy(&actual,&bits,sizeof(actual));int sum=0;
  for(unsigned tile=0;tile<160;tile++)for(unsigned c=0;c<32;c++)sum+=int((row*7+tile*3+c*5)%17)-8;
  assert(actual==float(sum)*0.0625f);
 }
 puts("PASS host BFP8 queued-read/tensor bridge on nonzero synthetic band/32 exact values; NO worker CPU, MODEL, DEVICE OR TPS");
}
