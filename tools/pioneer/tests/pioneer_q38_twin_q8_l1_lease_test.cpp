/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TwinQ8L1Lease.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeHostWriteProbe.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>

namespace L=PioneerQ38TwinQ8L1Lease;
namespace S=L::S;
using L::Tile;
using Words=std::vector<uint32_t>;

static S::Binding Fixture(){
 S::Binding b={};b.address=0x04000000;
 const uint32_t fixed[]={S::Magic,1,64,S::Worker,S::Peer,0x10001,8192,64,1,2,3,4,5,6,7,8};
 for(unsigned i=0;i<16;i++)b.boot[i]=fixed[i];
 b.boot[16]=b.boot[5];
 const uint32_t ext[]={0x334f4751,1,5120,248320,12,1,0x31415051,68,256,0x33504d51,129,0x118000,256,0x83c00,64,0x384d5351};
 for(unsigned i=0;i<16;i++)b.boot[i+40]=ext[i];
 return b;
}
struct ROM {
 std::array<Words,10> code;
 unsigned fault=10;
 ROM(){
  const unsigned bytes[]={9532,8392,10332,16456,9888,1292,5020,8060,35944,4};
  for(unsigned i=0;i<10;i++)for(unsigned j=0;j<bytes[i]/4;j++)code[i].push_back(0x29b7c1abU^(i*8192+j));
 }
 bool Section(S::Arm a,unsigned i,const uint32_t*& data,unsigned& size)const{
  if(a!=S::Arm::Native || i==fault)return false;
  data=code.at(i).data();size=unsigned(code.at(i).size());return true; // mock words, not matched ROM
 }
};
struct IO {
 std::array<Words,2> memory,original;
 unsigned reads=0,writes=0,holds=0,attempts=0,lastWrite=0;
 unsigned failRead=0,failWrite=0,badRead=0;
 bool stop=false,knownHeld=true,tripped=false;
 IO(){
  for(unsigned t=0;t<2;t++){
   memory[t].resize(0x180000/4);
   for(unsigned i=0;i<memory[t].size();i++)memory[t][i]=i%257?0xbeaeefa7U*(i+1U+t):UINT32_MAX;
  }
  original=memory;
 }
 unsigned Calls()const{return reads+writes+holds+attempts;}
 bool Held(Tile t){assert(!tripped && (t==Tile::Root || t==Tile::Worker));holds++;return knownHeld;}
 bool ClosedWorkerHeld(){assert(!tripped);attempts++;return stop && knownHeld;}
 bool ReadHeld(Tile t,unsigned at,uint32_t& out){
  assert(!tripped && knownHeld && L::Owns(t,at));reads++;
  if(reads==failRead){tripped=true;return false;}
  out=memory.at(unsigned(t)).at(at/4);
  if(reads==badRead){out^=1;tripped=true;}
  return true;
 }
 bool WriteHeld(Tile t,unsigned at,uint32_t value){
  assert(!tripped && knownHeld && L::Owns(t,at));writes++;
  if(writes==failWrite){tripped=true;return false;}
  lastWrite=at;memory.at(unsigned(t)).at(at/4)=value;return true;
 }
};

static void Shape(){
 unsigned owned[2]={};
 for(unsigned at=0;at<0x180000;at++)for(unsigned i=0;i<2;i++){
  const auto t=i?Tile::Worker:Tile::Root;
  if(L::Owns(t,at)){assert(!(at&3));owned[i]++;}
  if(L::RuntimeRead(t,at))assert(L::Owns(t,at));
  if(L::RuntimeWrite(t,at))assert(L::RuntimeRead(t,at));
 }
 assert(owned[0]==L::RootWords && owned[1]==L::WorkerWords);
 assert(!L::RuntimeWrite(Tile::Worker,L::Entry) && !L::RuntimeRead(Tile::Worker,L::Entry));
 assert(!L::RuntimeWrite(Tile::Worker,S::Proof) && !L::RuntimeWrite(Tile::Worker,S::Status));
 assert(L::RuntimeWrite(Tile::Worker,S::Bell) && L::RuntimeWrite(Tile::Root,S::Mail+252));
 assert(!L::RuntimeRead(Tile::Root,S::Mail+256) && !L::RuntimeRead(Tile::Root,L::RootScale+640));
}

static void Counters(){
 unsigned roots=0,workers=0;
 const auto root=[&](unsigned at,uint32_t& out){
  assert(at==0x2f0 || at==0x2e0 || at==0x2e8 || at==0x2c4);
  // Tag221 used this accessor, which returned an error sentinel for
  // all four addresses. The private ROOT view reads the real register.
  assert(!BlackholeHostWriteReadRegister(at));roots++;
  out=at==0x2f0?UINT32_MAX:at;return true;
 };
 const auto worker=[&](unsigned at,uint32_t& out){
  assert(BlackholeHostWriteReadRegister(at));workers++;out=at;return true;
 };
 for(unsigned at=0;at<0x1000;at++){
  const bool needRoot=at==0x2f0 || at==0x2e0 || at==0x2e8 || at==0x2c4;
  const bool needWorker=at==0x208 || at==0x20c || at==0x210 || at==0x214 || at==0x204 || at==0x220 || at==0x228;
  for(auto tile:{Tile::Root,Tile::Worker,static_cast<Tile>(3)}){
   uint32_t v=0xfeedbeef;
   const bool permitted=tile==Tile::Root?needRoot:tile==Tile::Worker && needWorker;
   assert(L::ReadCounter(tile,at,v,root,worker)==permitted);
   assert(v==(!permitted?0xfeedbeefU:at==0x2f0?UINT32_MAX:at));
  }
 }
 assert(roots==4 && workers==7);
 uint32_t v=0;
 assert(!L::ReadCounter(Tile::Root,0x2f0,v,[](unsigned,uint32_t&){return false;},worker) && !v && workers==7);
}

int main(){
 Shape();Counters();const auto b=Fixture();ROM rom;
 for(unsigned which=0;which<9;which++){
  Words saved(L::SavedWords);IO io;L::Lease lease(saved.data(),unsigned(saved.size()));
  if(which==1)io.failRead=30;
  if(which==2)io.badRead=L::SavedWords+19;
  const bool capture=lease.Capture(io);
  if(which==1 || which==2){
   assert(!capture && !io.writes && lease.State()==L::Phase::Retained && io.memory==io.original);
  }else{
   assert(capture && io.reads==2*L::SavedWords && !io.writes);
   if(which==3)rom.fault=9;
   if(which==4)io.failWrite=1;
   if(which==5)io.badRead=io.reads+12;
   const bool installed=lease.Install(io,rom,S::Arm::Native,b);rom.fault=10;
   if(which==3)assert(!installed && !io.writes && lease.State()==L::Phase::Held && io.memory==io.original);
   else if(which==4 || which==5)assert(!installed && io.tripped && lease.State()==L::Phase::Retained);
   else{
    assert(installed && io.lastWrite==L::Entry && lease.State()==L::Phase::Ready);
    assert(io.memory[0]==io.original[0] && io.memory[1][S::Boot/4+55]==0x384d5351
     && io.memory[1][S::Output/4]==0x7fc00000 && io.memory[1][S::Bell/4]==0);
    const auto calls=io.Calls();assert(!lease.Restore(io) && calls==io.Calls());
    assert(lease.NoteRelease() && !lease.NoteRelease());
    io.stop=which!=6;
    if(which==7)io.failWrite=io.writes+21091;
    if(which==8)io.badRead=io.reads+23;
    // Model a legal ROOT input and worker scratch mutation before STOP.
    io.memory[0][L::RootInput/4]^=1;io.memory[0][S::Mail/4]^=1;io.memory[1][S::Proof/4]^=1;
    const auto before=io.writes;
    const bool restored=lease.Restore(io);
    if(which==0)assert(restored && lease.State()==L::Phase::Closed && io.memory==io.original
     && io.writes-before==L::SavedWords);
    else{
     assert(!restored && lease.State()==L::Phase::Retained && io.attempts==1);
     if(which==6)assert(io.writes==before);
     if(which==8)assert(io.writes-before==L::SavedWords && lease.Detail().seen!=lease.Detail().expected);
    }
   }
  }
  const auto calls=io.Calls();
  assert(!lease.Capture(io) && !lease.Restore(io) && calls==io.Calls());
 }
 puts("PASS Q8 two-participant journal: 113385 words, 8 faults, exact master/slave read policy; no device");
}
