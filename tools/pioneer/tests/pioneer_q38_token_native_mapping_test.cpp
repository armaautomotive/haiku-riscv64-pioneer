/* SPDX-License-Identifier: MIT */
// Verbatim full-token native mapping adapter. OS aliases and access scopes
// are simulated endpoint checks ONLY,not a native owner/launch/END grant.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenMemory.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenViewLease.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPciLease.h"
#define PIONEER_Q38_TOKEN_NATIVE_VIEW_ENDPOINT_TEST 1
#define PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE 1
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using area_id=int;
static constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
namespace L=PioneerQ38TokenL1;
struct Endpoint {
 static constexpr uint64 Bar=UINT64_C(0x900000000);
 static constexpr unsigned Words=0x180000/4;
 std::vector<uint32> alias=std::vector<uint32>(Words);
 std::array<std::vector<uint32>,120> core;
 unsigned selected=0,maps=0,mmio=0,direct=0;int fail=-1,nullView=-1;
 std::array<unsigned,4> mappedOffset={},mappedBytes={};
 uint32& At(uint32 address){assert(selected<120 && !(address&3) && address<0x180000);
  auto& data=core[selected];if(data.empty())data.resize(Words);return data.at(address/4);}
 uint32 Address(const volatile uint32* p)const{
  const auto a=reinterpret_cast<uintptr_t>(p),base=reinterpret_cast<uintptr_t>(alias.data());
  assert(a>=base && a-base<Words*4 && !((a-base)&3));return uint32(a-base);
 }
 bool Mapped(uint32 address)const{
  for(unsigned i=0;i<4;i++)if(mappedBytes[i] && address>=mappedOffset[i]
   && address-mappedOffset[i]<=mappedBytes[i]-4)return true;
  return false;
 }
};
static Endpoint* endpoint=nullptr;
static area_id map_physical_memory(const char*,uint64 physical,unsigned bytes,int placement,unsigned flags,void** view){
 assert(endpoint && endpoint->maps<4 && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
 const unsigned offsets[]={0x20000,0x40000,0x60000,0xac000},lengths[]={L::Precise?131072U:8192U,131072,57344,868352};
 const unsigned i=endpoint->maps++;assert(physical==Endpoint::Bar+0x19000000+offsets[i] && bytes==lengths[i]);
 if(int(i)==endpoint->fail)return -1;
 if(int(i)!=endpoint->nullView){endpoint->mappedOffset[i]=offsets[i];endpoint->mappedBytes[i]=bytes;}
 *view=int(i)==endpoint->nullView?nullptr:endpoint->alias.data()+offsets[i]/4;return int(i)+37;
}
static uint32 PioneerRead32(const volatile uint32* p){const auto address=endpoint->Address(p);assert(endpoint->Mapped(address));endpoint->mmio++;return endpoint->At(address);}
static void PioneerWrite32(volatile uint32* p,uint32 value){const auto address=endpoint->Address(p);assert(endpoint->Mapped(address));endpoint->mmio++;endpoint->At(address)=value;}
struct MappingIO:Endpoint {
 struct{uint64 bar0_host_start=Bar;}profile;
 unsigned runtimeCore=0,payloadBytes=8192;
 bool fleetEnabled=true,runtimeMapped=true,panelStable=true,mappingSource=true,accessSource=true;
 uint32 entry=0;uint32 scratch[2048]={};uint32* runtimeEntry=&entry;uint32* scratchData=scratch;
 const volatile uint32* runtimePc=&entry;
 bool Q38PanelMappingStable(){return panelStable;}
 void Select(unsigned w){assert(w<120);runtimeCore=selected=w;}
 uint32 RuntimeCodeRead(unsigned word){assert(!word);direct++;return At(0x6000);}
 void RuntimeCodeWrite(unsigned word,uint32 value){assert(!word);direct++;At(0x6000)=value;}
 uint32 Q38PanelRead(uint32 address){assert(address>=0x6e000 && address<0xac000);direct++;return At(address);}
 void Q38PanelWrite(uint32 address,uint32 value){assert(address>=0x6e000 && address<0xac000);direct++;At(address)=value;}
 uint32 ResidentL1Read(unsigned word){assert(word>=2032 && word<2040);direct++;return At(0x17e000+word*4);}
 void ResidentL1Write(unsigned word,uint32 value){assert(word>=2032 && word<2040);direct++;At(0x17e000+word*4)=value;}
 // This old L1-geometry test never exercises the newer restoration/CSR
 // adapters. Abort if one unexpectedly reaches a raw hardware endpoint.
 uint32 DramTlbRead(unsigned,unsigned){std::abort();}
 void DramTlbWrite(unsigned,unsigned,uint32){std::abort();}
 uint32 ResidentNocRead(uint32){std::abort();}
 void ResidentNocWrite(uint32,uint32){std::abort();}
 uint32 TokenFifoStatusWord(uint32){std::abort();}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeMapping.inc"
};
bool MappingIO::TokenMemoryMappingSourceScope(){return mappingSource;}
bool MappingIO::TokenMemoryAccessScope(){return accessSource;}
static uint32 Value(unsigned worker,unsigned word){return word%17?uint32(0x9e3779b9U*(word+1)^0x1234567U*(worker+1)):UINT32_MAX;}
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned words=0,refused=0;
 {
  auto io=std::make_unique<MappingIO>();endpoint=io.get();uint32 out=0x12345678;
  assert(!io->TokenMemoryReadChecked(0,0x60000,out) && out==0x12345678 && !io->TokenMemoryWrite(0,0x60000,0)
   && !io->maps && !io->mmio && !io->direct);
  assert(io->MapTokenMemory() && io->maps==4 && !io->mmio && !io->direct);
  assert(io->mappedBytes[0]==(L::Precise?131072U:8192U) && io->Mapped(0x20000) && io->Mapped(0x21ffc)
   && io->Mapped(0x30000)==bool(L::Precise) && io->Mapped(0x3fffc)==bool(L::Precise));
  assert(!io->MapTokenMemory() && io->maps==4);refused++;
  for(unsigned worker=0;worker<120;worker++){
   io->Select(worker);assert(io->TokenMemoryMappingStable(worker));
   for(unsigned i=0;i<L::SnapshotWords(worker);i++){
    const uint32 address=L::SnapshotAddress(worker,i),value=Value(worker,i);
    assert(io->TokenMemoryWrite(worker,address,value) && io->TokenMemoryReadChecked(worker,address,out) && out==value);words++;
   }
   for(uint32 address:{0U,0x6001U,0x22000U,0x23ffcU,0x17f800U,0x17ffbcU,0x17ffe0U,0x180000U,UINT32_MAX}){
    unsigned n=io->mmio+io->direct;out=0x12345678;
    assert(!io->TokenMemoryReadChecked(worker,address,out) && out==0x12345678 && !io->TokenMemoryWrite(worker,address,0)
     && io->mmio+io->direct==n);refused++;
   }
   if(!worker)for(uint32 address:{0x20000U,0x3fffcU,0x60008U,0x63ffcU,0x6c000U,0x6dffcU,0x70000U,0x73ffcU}){
    auto n=io->mmio+io->direct;assert(!io->TokenMemoryReadChecked(worker,address,out) && !io->TokenMemoryWrite(worker,address,0) && io->mmio+io->direct==n);refused++;
   }
   else for(uint32 address:{0x44000U,0x5fffcU,0xac000U,0xb7ffcU,0xc1000U}){
    auto n=io->mmio+io->direct;assert(!io->TokenMemoryReadChecked(worker,address,out) && !io->TokenMemoryWrite(worker,address,0) && io->mmio+io->direct==n);refused++;
   }
  }
  assert(words==L::SavedWords);
  // Same virtual apertures MUST still select independent participant bytes.
  for(unsigned worker=0;worker<120;worker++){io->Select(worker);
   for(unsigned i=0;i<L::SnapshotWords(worker);i++){out=0;assert(io->TokenMemoryReadChecked(worker,L::SnapshotAddress(worker,i),out) && out==Value(worker,i));}
  }
  io->Select(0);for(unsigned kind=0;kind<11;kind++){
   auto bar=io->profile.bar0_host_start;auto* entry=io->runtimeEntry;auto* scratch=io->scratchData;
   auto* pc=io->runtimePc;
   if(!kind)io->runtimeCore=1;if(kind==1)io->fleetEnabled=false;if(kind==2)io->runtimeMapped=false;if(kind==3)io->panelStable=false;
   if(kind==4)io->profile.bar0_host_start++;if(kind==5)io->runtimeEntry=nullptr;if(kind==6)io->scratchData=nullptr;
   if(kind==7)io->payloadBytes--;if(kind==8)io->accessSource=false;if(kind==9)io->q38TokenMemoryArea[3]=-1;
   if(kind==10)io->runtimePc=nullptr;
   auto n=io->mmio+io->direct;out=0x12345678;
   assert(!io->TokenMemoryReadChecked(0,0x60004,out) && out==0x12345678 && !io->TokenMemoryWrite(0,0x60004,0) && io->mmio+io->direct==n);refused++;
   io->runtimeCore=0;io->fleetEnabled=io->runtimeMapped=io->panelStable=io->accessSource=true;io->profile.bar0_host_start=bar;
   io->runtimeEntry=entry;io->runtimePc=pc;io->scratchData=scratch;io->payloadBytes=8192;io->q38TokenMemoryArea[3]=40;
  }
 }
 for(unsigned kind=0;kind<12;kind++){
  auto io=std::make_unique<MappingIO>();endpoint=io.get();
  if(!kind)io->mappingSource=false;if(kind==1)io->runtimeCore=1;if(kind==2)io->fleetEnabled=false;if(kind==3)io->runtimeMapped=false;
  if(kind==4)io->panelStable=false;if(kind==5)io->runtimeEntry=nullptr;if(kind==6)io->scratchData=nullptr;if(kind==7)io->payloadBytes--;
  if(kind==8)io->profile.bar0_host_start=UINT64_MAX-0x1917ffff;if(kind==9)io->q38TokenMemoryArea[0]=1;
  if(kind==10)io->q38TokenMemoryView[1]=io->alias.data();if(kind==11)io->profile.bar0_host_start=UINT64_MAX;
  assert(!io->MapTokenMemory() && !io->maps && !io->mmio && !io->direct && !io->q38TokenMemoryMapAttempted);refused++;
 }
 for(unsigned kind=0;kind<8;kind++){
  auto io=std::make_unique<MappingIO>();endpoint=io.get();if(kind<4)io->fail=int(kind);else io->nullView=int(kind-4);
  assert(!io->MapTokenMemory() && io->q38TokenMemoryMapAttempted);const unsigned maps=io->maps;
  assert(maps==kind%4+1 && !io->MapTokenMemory() && io->maps==maps && !io->mmio && !io->direct);
  uint32 out=19;assert(!io->TokenMemoryReadChecked(0,0x60000,out) && out==19 && !io->TokenMemoryWrite(0,0x60000,0) && !io->mmio && !io->direct);refused++;
 }
 printf("PASS ACTUAL full-token mapping adapter/ALL120 independent simulated apertures/ALL%u saved words write+two reads/root60000+60004/4 exact VM aliases/%u inert refusals/UINT32_MAX DATA/partial aliases retained NO retry+unmap;OS+scope endpoints SIMULATED,NOT native factory/source/physical/launch/END/speed qualification\n",words,refused);
}
