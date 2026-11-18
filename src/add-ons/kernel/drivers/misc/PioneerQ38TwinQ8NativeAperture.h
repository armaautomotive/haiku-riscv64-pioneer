/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE_H
#define PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE_H
#if !defined(_KERNEL_MODE) || !defined(PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE) \
 || PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE != 1 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT) \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026111700010221ULL
#error Twin Q8 aperture requires its exact private Haiku diagnostic role
#endif
#include "PioneerQ38TwinQ8L1Lease.h"

// Native mapping only. Include after PioneerSessionIO. The enclosing
// native owner must hold the same exclusive CPU/FD/PCI/GDDR owners on every
// operation and keep this object alive until clean END or power recovery.
// Mapping does not grant NIU writes, reset, device execution or cleanup.
namespace PioneerQ38TwinQ8NativeAperture {
namespace L=PioneerQ38TwinQ8L1Lease;
namespace S=L::S;
using Tile=L::Tile;

template<class Owner> class Access {
 Owner& owner;
 PioneerSessionIO& io;
 const uint64 bar;
 area_id areas[4]={-1,-1,-1,-1};
 volatile uint32* views[4]={};
 enum class State { New, Mapping, Held, Running, StoppedHeld, Closed, Retained };
 State state=State::New;
 unsigned selected=UINT32_MAX;

 bool Fail(){state=State::Retained;return false;}
 static bool Valid(Tile tile){return tile==Tile::Root || tile==Tile::Worker;}
 static unsigned Worker(Tile tile){return tile==Tile::Root?0U:unsigned(S::Worker);}
 bool Scope(){
  return state!=State::New && state!=State::Retained && state!=State::Closed
   && owner.Scope() && io.profile.bar0_host_start==bar;
 }
 bool CheckSelection(Tile tile){
  if(!Valid(tile) || !Scope() || io.runtimeCore!=Worker(tile) || !io.Q38PanelMappingStable())return false;
  const unsigned w=Worker(tile);
  for(unsigned slot=0;slot<2;slot++)for(unsigned word=0;word<3;word++){
   if(!Scope() || io.DramTlbRead(slot,word)!=BlackholeDramFixedTlbWord(slot?0xffa00000U:0U,
     PioneerWorkerX(w),PioneerWorkerY(w),false,word))return false;
  }
  return true;
 }
 bool Select(Tile tile,bool verify=false){
  if(!Valid(tile) || !Scope())return false;
  const unsigned w=Worker(tile);
  if(selected!=w){
   if(!io.SelectRuntimeCore(w) || !CheckSelection(tile))return Fail();
   selected=w;
  }else if(io.runtimeCore!=w || !io.Q38PanelMappingStable() || (verify && !CheckSelection(tile)))return Fail();
  return true; // same-owner access under the caller's locked lifetime
 }
 bool ResetIs(uint32 value){return Scope() && io.ResetRead()==value && Scope() && io.ResetRead()==value;}
 bool RawRead(unsigned address,uint32_t& output){
  if(!Scope())return false;
  if(address==L::Entry)output=io.RuntimeCodeRead(0);
  else if(address<0x40000)output=PioneerRead32(views[0]+(address-0x20000)/4);
  else if(address<0x60000)output=PioneerRead32(views[1]+(address-0x40000)/4);
  else if(address<0x6e000)output=PioneerRead32(views[2]+(address-0x60000)/4);
  else if(address<0xac000)output=io.Q38PanelRead(address);
  else if(address>=0x17ffc0)output=io.ResidentL1Read(0x1fc0/4+(address-0x17ffc0)/4);
  else output=PioneerRead32(views[3]+(address-0xac000)/4);
  return true; // original 0xffffffff is DATA, not an error
 }
 bool RawWrite(unsigned address,uint32_t value){
  if(!Scope())return false;
  if(address==L::Entry)io.RuntimeCodeWrite(0,value);
  else if(address<0x40000)PioneerWrite32(views[0]+(address-0x20000)/4,value);
  else if(address<0x60000)PioneerWrite32(views[1]+(address-0x40000)/4,value);
  else if(address<0x6e000)PioneerWrite32(views[2]+(address-0x60000)/4,value);
  else if(address<0xac000)io.Q38PanelWrite(address,value);
  else if(address>=0x17ffc0)io.ResidentL1Write(0x1fc0/4+(address-0x17ffc0)/4,value);
  else PioneerWrite32(views[3]+(address-0xac000)/4,value);
  return true;
 }
 bool Live(Tile tile){
  return state==State::Running && Select(tile) && ResetIs(tile==Tile::Root?0x47800:0x46800);
 }
public:
 Access(Owner& o,PioneerSessionIO& native):owner(o),io(native),bar(native.profile.bar0_host_start){}
 Access(const Access&)=delete;Access& operator=(const Access&)=delete;
 void Retain(){state=State::Retained;} // memory only, including partially mapped aliases
 bool MapHeld(){
  if(state!=State::New || !owner.Scope() || bar>UINT64_MAX-UINT64_C(0x19180000)
   || !io.fleetEnabled || !io.runtimeMapped || !io.runtimeEntry || !io.runtimePc || !io.scratchData
   || io.payloadBytes!=8192 || TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000)return false;
  state=State::Mapping;
  if(!Scope() || !io.SelectRuntimeCore(0) || !io.MapQ38Panel() || !CheckSelection(Tile::Root) || !ResetIs(0x47800)
   || !Scope() || !io.SelectRuntimeCore(S::Worker) || !io.MapQ38Panel() || !CheckSelection(Tile::Worker)
   || !ResetIs(0x47800))return Fail();
  selected=S::Worker;
  constexpr uint32_t offsets[]={0x19020000,0x19040000,0x19060000,0x190ac000};
  constexpr uint32_t bytes[]={0x20000,0x20000,0xe000,0xd4000};
  for(unsigned i=0;i<4;i++){
   if(!Scope())return Fail();
   void* ptr=nullptr;
   areas[i]=map_physical_memory("P100A private Q8 comparison L1",bar+offsets[i],bytes[i],
    B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&ptr);
   if(areas[i]<B_OK || !ptr)return Fail();
   views[i]=static_cast<volatile uint32*>(ptr);
  }
  if(!CheckSelection(Tile::Worker) || !ResetIs(0x47800))return Fail();
  state=State::Held;return true;
 }
 bool Held(Tile t){
  return (state==State::Held || state==State::StoppedHeld) && Select(t,true) && ResetIs(0x47800);
 }
 bool ReadHeld(Tile t,unsigned at,uint32_t& out){
  return (state==State::Held || state==State::StoppedHeld) && L::Owns(t,at) && Select(t) && RawRead(at,out);
 }
 bool WriteHeld(Tile t,unsigned at,uint32_t value){
  return (state==State::Held || state==State::StoppedHeld) && L::Owns(t,at) && Select(t) && RawWrite(at,value);
 }
 // Call exactly once before the outer owner's actual reset release.
 bool NoteRelease(){
  if(state!=State::Held || !Held(Tile::Worker))return false;
  state=State::Running;return true;
 }
 bool Read(Tile t,unsigned at,uint32_t& out){
  return L::RuntimeRead(t,at) && Live(t) && RawRead(at,out);
 }
 bool Write(Tile t,unsigned at,uint32_t value){
  return L::RuntimeWrite(t,at) && Live(t) && RawWrite(at,value);
 }
 bool Count(Tile t,unsigned offset,uint32_t& out){
  if(!L::CounterRead(t,offset) || !Live(t) || !Scope())return false;
  out=io.ResidentNocRead(offset);return true;
 }
 // Outer owner verified actual STOP and held reset before calling. No
 // access to aliases or power controls is added after an uncertain STOP.
 bool ClosedWorkerHeld(){
  if(state!=State::Running || !owner.Scope() || !owner.WorkerStopProved()
   || !Select(Tile::Worker,true) || !ResetIs(0x47800))return Fail();
  state=State::StoppedHeld;return true;
 }
 bool ReleaseRestoredViews(const L::Lease& lease){
  if(state!=State::StoppedHeld || lease.State()!=L::Phase::Closed || !Held(Tile::Worker) || !Held(Tile::Root))return false;
  for(unsigned i=4;i>0;i--){
   if(!Scope() || areas[i-1]<B_OK || !views[i-1] || delete_area(areas[i-1])!=B_OK)return Fail();
   areas[i-1]=-1;views[i-1]=nullptr;
  }
  state=State::Closed;return true;
 }
};
}
#endif
