/* SPDX-License-Identifier: MIT */
// Offline BFP4 endpoint-only test; no worker CPU, device or model.
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#define PIONEER_Q38_BFP4_WORKER_DIAGNOSTIC 1
#include "pioneer_q38_native_bfp4_peer_bridge.h"
#include "../PioneerQ38Bfp4WorkerCompute.h"
#include "../PioneerBlackholeBlockFloat.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
constexpr unsigned TestBands=8;
struct Endpoints {
 std::vector<uint32_t> root=std::vector<uint32_t>(0x180000/4),peer=std::vector<uint32_t>(0x180000/4);
 std::vector<uint8_t> weights=std::vector<uint8_t>(TestBands*160*576);
 uint32_t csr[0x2000/4]={};unsigned reads=0,events=0;
 Q38NativePeerTestBytes Bytes(){Q38NativePeerTestBytes b;b.context=this;
  b.word=[](void* p,unsigned worker,uint32_t address)->uint32_t&{auto& e=*static_cast<Endpoints*>(p);
   assert(!(address&3) && address<0x180000 && worker<2);return (worker?e.peer:e.root)[address/4];};
  b.noc=[](void* p,unsigned worker,unsigned offset){auto& e=*static_cast<Endpoints*>(p);assert(worker==1 && !(offset&3) && offset<0x2000);return e.csr[offset/4];};
  b.read=[](void* p,unsigned route,uint32_t address,uint8_t* out,unsigned n){auto& e=*static_cast<Endpoints*>(p);
   if(route!=786 || address<0x04000000 || !out || !n || n>16384 || address-0x04000000+n>e.weights.size())return false;
   std::memcpy(out,e.weights.data()+(address-0x04000000),n);e.reads++;return true;};
  b.events=[](void* p,unsigned worker,const uint32_t* d,const uint32_t* cmd,unsigned mask,bool changed,bool pending){auto& e=*static_cast<Endpoints*>(p);
   assert(worker==1 && d[2]==d[6]);const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(i!=6)e.csr[offsets[i]/4]+=d[i];
   for(unsigned i=0;i<10;i++)if(mask&(1U<<i))e.csr[i]=cmd[i];if(changed)e.csr[0x40/4]=pending;e.events++;
  };return b;
 }
};
struct LoaderIO {Q38NativeBfp4PeerBridge& bridge;unsigned Read(unsigned a){return bridge.Read(a);}void Write(unsigned a,unsigned v){bridge.Write(a,v);}void Fence(){}};
struct TensorIO {Q38NativeBfp4PeerBridge& bridge;
 void Op(unsigned v){bridge.Write(0xffe40000,v);}void Config(unsigned i,unsigned v){bridge.Write(0xffef0000+i*4,v);}
 void Drain(){bridge.Write(0xffe40000,0xa2101816);bridge.Write(0xffe40000,0x4500017e);assert(bridge.Read(0xffe000fc)==1);bridge.Write(0xffe000fc,0);}
};
int main(){
 using W=PioneerQ38Bfp4WorkerJob;W::Job job={{W::Magic,W::Version,5120,0x04000000,TestBands,786,1,W::Existing::Peer(1),
  PioneerQ38Bf16Wire::Magic,W::RootInput,W::RootRoute,2,W::Output,0,0,0}};assert(W::Shape(job));
 Endpoints endpoint;const unsigned counters[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};uint32_t initial[8];
 for(unsigned i=0;i<8;i++)if(i!=6)endpoint.csr[counters[i]/4]=UINT32_MAX-80-i*13;
 for(unsigned i=0;i<8;i++)initial[i]=endpoint.csr[counters[i]/4];
 for(unsigned i=0;i<2560;i++)endpoint.root[W::RootInput/4+i]=0x3f803f80;
 for(unsigned tile=0;tile<160;tile++){
  for(unsigned band=0;band<TestBands;band++){
   float values[1024];for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++)
    values[row*32+col]=float((band&1?-1:1)*(int((row*3+tile*7+col*5)%7)-3))*0.125f;
   assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,values,1024,endpoint.weights.data()+(band*160+tile)*576,576));
  }
 }
 const auto bytes=endpoint.Bytes();Q38NativeBfp4PeerBridge bridge(job.words,1,bytes);LoaderIO io{bridge};
 for(unsigned i=0;i<8;i++)assert(bridge.Read(0xffb20000+counters[i])==initial[i]);
 TensorIO tensor{bridge};
 for(unsigned band=0;band<TestBands;band++){
  PioneerQ38Bfp4WorkerLoader loader;
  assert(loader.Run(io,job,band) && endpoint.reads && endpoint.events);
  assert(PioneerQ38Bfp4WorkerCompute::Compute(tensor,5120) && bridge.Clean() && bridge.MatrixInstructions()==(band+1)*1280);
  for(unsigned row=0;row<32;row++){
   const unsigned dst=row<16?32:40,col=row%16;const uint32_t bits=bridge.Read(0xffbd8000+dst*64+col*4);
   float actual;std::memcpy(&actual,&bits,sizeof(actual));int sum=0;
   for(unsigned tile=0;tile<160;tile++)for(unsigned c=0;c<32;c++)sum+=int((row*3+tile*7+c*5)%7)-3;
   assert(actual==float((band&1?-1:1)*sum)*0.125f);
  }
 }
 for(unsigned i=0;i<8;i++)assert(bridge.CounterDelta(i)==uint32_t(endpoint.csr[counters[i]/4]-initial[i]));
 for(unsigned i=0;i<10;i++)assert(bridge.LastCommand(i)==endpoint.csr[i]);
 puts("PASS host BFP4 queued-read/tensor bridge/eight alternating bands/32 synthetic exact rows/counter wrap; NO worker CPU, MODEL, DEVICE OR TPS");
}
