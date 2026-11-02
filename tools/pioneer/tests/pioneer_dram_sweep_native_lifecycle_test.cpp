/* SPDX-License-Identifier: MIT */
// Execute the ACTUAL Native.inc scope/mapping/closure methods against explicit
// KernelExport/PCI endpoints. This is NOT native hardware or a model/ARC run.
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <map>
#include <new>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramReadSweepOwner.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePioneerMmioProbe.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;
using int32=int32_t;using status_t=int32_t;using area_id=int32_t;
using thread_id=int32_t;using team_id=int32_t;using sem_id=int32_t;
enum { B_OK=0,B_NOT_ALLOWED=-1,B_BAD_VALUE=-2,B_BUSY=-3,B_NO_MEMORY=-4,B_DEV_NOT_READY=-5,
 B_KERNEL_READ_AREA=16,B_KERNEL_WRITE_AREA=32,B_KERNEL_EXECUTE_AREA=64,
 B_READ_AREA=1,B_WRITE_AREA=2,B_EXECUTE_AREA=4,B_UNCACHED_MEMORY=1<<28,
 B_ANY_KERNEL_ADDRESS=0,B_PAGE_SIZE=4096 };
#define TT_PCI_PROBE_DEVICE "test-only/no-public-device"
#define B_DEVICE_MANAGER_MODULE_NAME "test-only/device-manager"
struct module_info {};
struct thread_info {thread_id thread=10;team_id team=20;};
struct area_info {area_id area;void* address;size_t size;uint32 protection;};
struct pci_info {uint16 vendor_id=0x1e52,device_id=0xb140;uint8 bus=1,device=0,function=0;
 struct {struct {uint16 subsystem_vendor_id=0x1e52,subsystem_id=0x0043;} h0;} u;};
struct device_manager_info {};
struct PioneerStoreContext {};
struct PioneerProbeCookie {uint32 magic=0x50313030;team_id team=20;PioneerStoreContext* context=NULL;};
struct pioneer_driver_reload_guard_info {uint32 version=1;};
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=false;
static PioneerProbeCookie* sRuntimeOwner=NULL;
static uint64 sReloadGeneration=7;
static uint32 sOpenCookies=1;
static sem_id sLock=42;
static pci_info sCard;
static pioneer_driver_reload_guard_info guard;
static pioneer_driver_reload_guard_info* sReloadGuard=&guard;
static device_manager_info manager;
static device_manager_info* sDeviceManager=&manager;
static char sCookie;
static thread_id currentThread=10;
static team_id currentTeam=20;
static int32 semCount=0;
static unsigned ioCalls=0,deletes=0,dropDelete=0,unreserves=0;
static unsigned pciWrites=0,dropPciWrite=0,pciReads=0,badPciRead=0;
static bool failUnreserve=false,failProfile=false;
static unsigned effectiveUid=0,copyCalls=0,failCopy=0;
static std::map<area_id,area_info> views;
static std::map<uint64,uint32> config;
static void Touch(){assert(!sColdRecoveryRequired && "endpoint activity after retention");ioCalls++;}
static uint64 Key(uint8 bus,uint16 offset){return (uint64(bus)<<32)|offset;}
static uint32 ConfigRead(uint8 bus,uint8,uint8,uint16 offset,uint8){
 Touch();const uint32 value=config[Key(bus,offset)];return ++pciReads==badPciRead?value^1:value;
}
static void ConfigWrite(uint8 bus,uint8,uint8,uint16 offset,uint8,uint32 value){
 Touch();if(++pciWrites!=dropPciWrite)config[Key(bus,offset)]=value;
}
static status_t Reserve(uint8,uint8,uint8,const char*,void*){Touch();return B_OK;}
static status_t Unreserve(uint8,uint8,uint8,const char*,void*){Touch();unreserves++;return failUnreserve?B_BUSY:B_OK;}
struct pci_module_info {
 uint32 (*read_pci_config)(uint8,uint8,uint8,uint16,uint8)=ConfigRead;
 void (*write_pci_config)(uint8,uint8,uint8,uint16,uint8,uint32)=ConfigWrite;
 status_t (*reserve_device)(uint8,uint8,uint8,const char*,void*)=Reserve;
 status_t (*unreserve_device)(uint8,uint8,uint8,const char*,void*)=Unreserve;
};
static pci_module_info pci;
static pci_module_info* sPCI=&pci;
static thread_id find_thread(const char*){return currentThread;}
static status_t get_thread_info(thread_id id,thread_info* t){assert(id==currentThread);t->thread=id;t->team=currentTeam;return B_OK;}
static status_t get_sem_count(sem_id id,int32* count){assert(id==sLock);*count=semCount;return B_OK;}
static status_t AcquireProbeLock(){if(sColdRecoveryRequired)return B_DEV_NOT_READY;semCount=0;return B_OK;}
static status_t release_sem(sem_id id){assert(id==sLock);semCount=1;return B_OK;}
static int geteuid(){return int(effectiveUid);}
static status_t user_memcpy(void* to,const void* from,size_t length){
 if(++copyCalls==failCopy)return B_NOT_ALLOWED;memcpy(to,from,length);return B_OK;
}
static status_t get_module(const char*,module_info**){assert(false);return B_NOT_ALLOWED;}
static status_t GetPioneerPciProfile(device_manager_info*,const pci_info&,tt_pioneer_profile&){
 assert(failProfile);return B_NOT_ALLOWED;
}
static area_id map_physical_memory(const char*,uint64,size_t bytes,int,uint32 protection,void** pointer){
 Touch();*pointer=calloc(1,bytes);assert(*pointer);
 const area_id id=views.empty()?100:views.rbegin()->first+1;
 views[id]={id,*pointer,bytes,protection};return id;
}
static status_t get_area_info(area_id id,area_info* info){
 Touch();if(!views.count(id))return B_BAD_VALUE;*info=views.at(id);return B_OK;
}
static status_t delete_area(area_id id){
 Touch();assert(views.count(id));if(++deletes==dropDelete)return B_BUSY;
 free(views.at(id).address);views.erase(id);return B_OK;
}
static uint32 PioneerRead32(const volatile uint32* p){Touch();assert(p);return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 value){Touch();assert(p);*p=value;}
static void spin(unsigned){Touch();}
struct PioneerMmioIO {
 const tt_pioneer_profile& profile;area_id area=-1;const volatile uint32* nodeID=NULL;
 explicit PioneerMmioIO(const tt_pioneer_profile& p):profile(p){}
 const tt_pioneer_profile& Profile(){return profile;}
 uint32 CardRead(uint16 a,uint8 n){return ConfigRead(1,0,0,a,n);}
 void CardWrite(uint16 a,uint8 n,uint32 v){ConfigWrite(1,0,0,a,n,v);}
 uint32 RootRead(uint16 a,uint8 n){return ConfigRead(0,0,0,a,n);}
 void RootWrite(uint16 a,uint8 n,uint32 v){ConfigWrite(0,0,0,a,n,v);}
 uint64 Size(unsigned){return 0;}
 bool Quiet(){return true;}
 uint32 ReadNodeId(){return PioneerRead32(nodeID);}
};
static void* NewView(area_id& id,unsigned bytes=4096,bool write=false){
 void* p=NULL;id=map_physical_memory("fixture",0,bytes,0,B_KERNEL_READ_AREA|(write?B_KERNEL_WRITE_AREA:0),&p);return p;
}
struct PioneerTensixIO:PioneerMmioIO {
 area_id tlbArea=-1,scratchArea=-1,csmArea=-1,resetArea=-1;
 volatile uint32* tlb=NULL;const volatile uint32 *scratch=NULL,*csm=NULL,*reset=NULL;
 explicit PioneerTensixIO(const tt_pioneer_profile& p):PioneerMmioIO(p){}
 bool Map(){
  nodeID=(const volatile uint32*)((uint8*)NewView(area)+0x44);
  tlb=(volatile uint32*)((uint8*)NewView(tlbArea,4096,true)+0x96c);
  scratch=(const volatile uint32*)NewView(scratchArea);
  csm=(const volatile uint32*)NewView(csmArea,0x20000);
  reset=(const volatile uint32*)((uint8*)NewView(resetArea)+0x1b0);return true;
 }
 uint32 ResetRead(){return PioneerRead32(reset);}
 bool AfterNodeId(uint32){assert(false && "NO fabricated ARC/telemetry/Tensix proof");return false;}
};
struct PioneerDramSamplesIO:PioneerTensixIO {
 area_id dramArea=-1,dataArea=-1;const volatile uint32* dramNiu=NULL;volatile uint32* data=NULL;
 explicit PioneerDramSamplesIO(const tt_pioneer_profile& p):PioneerTensixIO(p){}
 uint32 SourceRead(unsigned){Touch();return 0;}
 uint32 DramIdRead(unsigned i){return PioneerRead32(dramNiu+BlackholeDramIdOffset(i)/4);}
 uint32 DramTlbRead(unsigned s,unsigned i){return PioneerRead32(tlb-3+s*3+i);}
 void DramTlbWrite(unsigned s,unsigned i,uint32 v){PioneerWrite32(tlb-3+s*3+i,v);}
};
struct PioneerComputeNocIO:PioneerDramSamplesIO {
 area_id nocArea=-1;const volatile uint32* nocRegisters=NULL;
 PioneerComputeNocIO(const tt_pioneer_profile& p,tt_arc_scratch_result&,tt_arc_telemetry_result&,
  tt_tensix_reset_result&,tt_dram_samples_result&,tt_compute_noc_result&,unsigned):PioneerDramSamplesIO(p){}
 bool Map(){PioneerTensixIO::Map();dramNiu=(const volatile uint32*)NewView(dramArea);
  nocRegisters=(const volatile uint32*)NewView(nocArea,8192);return true;}
 uint32 ComputeNocRead(uint32 a){return PioneerRead32(nocRegisters+a/4);}
 bool AfterNodeId(uint32){assert(false && "NO fabricated prerequisite proof");return false;}
};
struct PioneerArcQueryIO:PioneerMmioIO {
 area_id tlbArea=-1,scratchArea=-1,csmArea=-1,queueWriteArea=-1,msiArea=-1;
 volatile uint32 *tlb=NULL,*queueWrite=NULL,*msi=NULL;const volatile uint32 *scratch=NULL,*csm=NULL;
 PioneerArcQueryIO(const tt_pioneer_profile& p,tt_arc_scratch_result&,tt_arc_telemetry_result&,tt_arc_queue_result&):PioneerMmioIO(p){}
 bool Map(){
  nodeID=(const volatile uint32*)((uint8*)NewView(area)+0x44);
  tlb=(volatile uint32*)((uint8*)NewView(tlbArea,4096,true)+0x96c);
  scratch=(const volatile uint32*)NewView(scratchArea);csm=(const volatile uint32*)NewView(csmArea,0x20000);
  queueWrite=(volatile uint32*)NewView(queueWriteArea,TT_ARC_QUEUE_CSM_BYTES,true);
  msi=(volatile uint32*)NewView(msiArea,4096,true);return true;
 }
 uint32 TlbRead(unsigned i){return PioneerRead32(tlb+i);}
 void TlbWrite(unsigned i,uint32 v){PioneerWrite32(tlb+i,v);}
 uint32 CsmRead(uint32 a){return PioneerRead32(queueWrite+(a-0x10000000)/4);}
 void SetQueueBounds(uint32,uint32,uint32){Touch();}
 void QueueWrite(uint32 a,uint32 v){PioneerWrite32(queueWrite+(a-0x10000000)/4,v);}
 uint32 MsiStatusRead(){return PioneerRead32(msi+2);}
 void NotifyArc(){PioneerWrite32(msi,0);}
 void Pause(){spin(100);}
};
struct PioneerDramSweepNativeContext;
static PioneerDramSweepNativeContext* sDramSweepNativeContext=NULL;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramReadSweepNative.inc"
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026110200030076)
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramSweepControl.inc"

struct Fixture {
 PioneerProbeCookie fd;
 PioneerDramSweepNativeContext* c;
 Fixture(){
  assert(views.empty() && !sDramSweepNativeContext);
  ioCalls=deletes=dropDelete=unreserves=pciWrites=dropPciWrite=pciReads=badPciRead=0;
  effectiveUid=copyCalls=failCopy=0;
  currentThread=10;currentTeam=20;semCount=0;sColdRecoveryRequired=sReloadQuiesced=sBankStorageOnly=false;
  sOpenCookies=1;sReloadGeneration=7;sReloadGuard=&guard;sPCI=&pci;sRuntimeOwner=NULL;
  c=new PioneerDramSweepNativeContext(&fd,thread_info{});sDramSweepNativeContext=c;
  c->backing=(uint32*)calloc(c->Allocation,4);assert(c->backing);c->reserved=true;
  c->profile.root_bus=0;c->profile.bar0_host_start=UINT64_C(0x4900000000);
 }
 void Map(){assert(c->io.Map() && views.size()==17);c->ready=true;assert(c->ViewsPresent());}
 void Closing(){c->closing=true;c->ready=false;}
 ~Fixture(){
  // Fixture endpoint teardown,NOT a production quarantine/recovery path.
  for(const auto& v:views)free(v.second.address);views.clear();
  delete c;sDramSweepNativeContext=NULL;sColdRecoveryRequired=false;
 }
};
int main(){
 unsigned valid=0,refused=0;
 {Fixture f;f.Map();assert(f.c->Scope());
  assert(f.c->io.SweepNativeScope(&f.c->algorithm,&f.c->dram,&f.c->noc,f.c->backing,
   f.c->backing+6490,6490,4096));f.Closing();assert(f.c->CloseViews() && views.empty() && deletes==17);valid++;}
 for(unsigned fail=1;fail<=17;fail++){
  Fixture f;f.Map();f.Closing();dropDelete=fail;
  assert(!f.c->CloseViews() && f.c->quarantined && deletes==fail && !unreserves && !pciWrites);
  const unsigned before=ioCalls;assert(!f.c->CloseViews() && ioCalls==before);refused++;
 }
 for(unsigned fault=0;fault<13;fault++){
  Fixture f;f.Map();
  if(fault==0)f.fd.magic=0;
  if(fault==1)f.fd.team++;
  if(fault==2)currentThread++;
  if(fault==3)currentTeam++;
  if(fault==4)semCount=1;
  if(fault==5)sReloadGeneration++;
  if(fault==6)sReloadQuiesced=true;
  if(fault==7)sRuntimeOwner=&f.fd;
  if(fault==8)sBankStorageOnly=true;
  if(fault==9)sOpenCookies=0;
  if(fault==10)sPCI=NULL;
  if(fault==11)sReloadGuard=NULL;
  if(fault==12)f.fd.context=(PioneerStoreContext*)uintptr_t(1);
  const unsigned before=ioCalls;
  assert(!f.c->Scope() && f.c->io.SourceRead(0)==UINT32_MAX && f.c->quarantined && ioCalls==before);
  assert(f.c->io.SweepPcRead()==UINT32_MAX && f.c->io.ComputeNocRead(0x44)==UINT32_MAX
   && f.c->io.SweepEntryOverrideRead()==UINT32_MAX && ioCalls==before);refused++;
 }
 for(unsigned fault=0;fault<4;fault++){
  Fixture f;f.Map();auto& v=views.at(f.c->io.sweepArea);
  if(fault==0)v.protection|=B_WRITE_AREA;
  if(fault==1)v.size-=4096;
  if(fault==2)v.address=(uint8*)v.address+4;
  if(fault==3)v.protection&=~B_KERNEL_WRITE_AREA;
  assert(!f.c->ViewsPresent());
  if(fault==2)v.address=(uint8*)v.address-4;
  else{const unsigned before=pciWrites;assert(f.c->io.SourceRead(0)==UINT32_MAX && f.c->quarantined && pciWrites==before);}
  refused++;
 }
 {Fixture f;f.Map();const unsigned before=ioCalls;f.c->io.ResidentNocWrite(0x40,1);
  assert(f.c->quarantined && ioCalls>=before && !pciWrites);refused++;}
 {Fixture f;f.Map();
  const uint32 offsets[]={0,0x24,0x64,0x68,0x178,0x208,0x20c,0x210,0x214,0x240,0x2bc,0x40,0x840,0x1040,0x1840};
  for(uint32 a:offsets){
   const uint32 value=0xabc00000U^a;const_cast<volatile uint32*>(f.c->io.nocRegisters)[a/4]=value;
   assert(f.c->io.ResidentNocRead(a)==value);valid++;
  }
  assert(f.c->io.ResidentNocRead(0x2c0)==UINT32_MAX && !f.c->quarantined);refused++;
 }
 {Fixture f;f.Map();uint32 value=0;
  assert(!f.c->io.SweepWrite(0x74000,0x1234));
  for(unsigned i=0;i<3;i++)f.c->io.DramTlbWrite(0,i,BlackholeDramFixedTlbWord(0,1,2,false,i));
  assert(f.c->io.SweepWrite(0x74000,0x1234) && f.c->io.SweepReadChecked(0x74000,value) && value==0x1234);
  f.c->reply.sweep.phase=7; // actual owner remaps ROOT before advancing phase8
  assert(f.c->io.SweepReadChecked(0x74000,value) && value==0x1234);
  for(unsigned i=0;i<3;i++)f.c->io.DramTlbWrite(0,i,BlackholeDramFixedTlbWord(0x04600000,18,12,true,i));
  assert(!f.c->io.SweepWrite(0x74000,0xdeadbeef) && !f.c->io.SweepReadChecked(0x74000,value));valid++;}
 {Fixture f;f.Map();
  auto* page=(volatile uint32*)views.at(f.c->io.controlArea).address;
  page[0x230/4]=0xbad00001;page[0x234/4]=0x12345678;page[0x238/4]=0xbad00002;
  assert(f.c->io.SweepEntryOverrideRead()==0x12345678 && page[0x234/4]==0x12345678);
  page[0x234/4]=1;assert(f.c->io.SweepEntryOverrideRead()==1);valid++;
 }
 {Fixture f;f.Map();f.c->reply.sweep.phase=4;f.c->io.SweepResetWrite(0x87ffff);
  assert(f.c->resetStage==1 && *f.c->io.resetWrite==0x87ffff);
  f.c->io.SweepResetWrite(0x87ffff);assert(f.c->quarantined && f.c->resetStage==1);refused++;}
 {Fixture f;f.Map();f.c->reply.sweep.phase=4;f.c->io.SweepResetWrite(0x87ffff);
  f.c->reply.sweep.phase=5;f.c->reply.sweep.launched=1;f.c->io.SweepResetWrite(0x46800);
  f.c->reply.sweep.phase=6;f.c->reply.sweep.observed=1;f.c->io.SweepResetWrite(0x47800);
  assert(f.c->resetStage==3 && *f.c->io.resetWrite==0x47800 && !f.c->quarantined);valid++;}
 for(unsigned drop=0;drop<=11;drop++){
  Fixture f;f.Closing();f.c->reply.viewsClosed=1;f.c->transaction.config_attempted=true;
  config.clear();config[Key(1,4)]=config[Key(0,4)]=0x402;
  for(unsigned i=0;i<6;i++){
   f.c->mapping.bars_before[i]=i*0x10000+0xc;config[Key(1,0x10+i*4)]=0xaa00+i;
  }
  for(unsigned i=0;i<3;i++){
   f.c->mapping.bridge_before[i]=i*0x100+0x11;config[Key(0,0x24+i*4)]=0xbb00+i;
  }
  dropPciWrite=drop;
  if(!drop){assert(f.c->ClosePci() && f.c->mapping.restored && !f.c->transaction.config_attempted);valid++;}
  else{assert(!f.c->ClosePci() && f.c->quarantined && pciWrites==drop);
   const unsigned before=ioCalls;assert(!f.c->ClosePci() && ioCalls==before);refused++;}
 }
 for(unsigned fault=1;fault<=22;fault++){
  Fixture f;f.Closing();f.c->reply.viewsClosed=1;f.c->transaction.config_attempted=true;
  config.clear();config[Key(1,4)]=config[Key(0,4)]=0x402;
  for(unsigned i=0;i<6;i++)f.c->mapping.bars_before[i]=i*0x10000+0xc;
  for(unsigned i=0;i<3;i++)f.c->mapping.bridge_before[i]=i*0x100+0x11;
  badPciRead=fault;
  assert(!f.c->ClosePci() && f.c->quarantined && pciReads==fault && !unreserves);
  const unsigned before=ioCalls;assert(!f.c->ClosePci() && ioCalls==before);refused++;
 }
 for(bool fail:{false,true}){
  Fixture f;failProfile=true;failUnreserve=fail;
  // Run the ACTUAL factory's no-mutation refusal/release edge,not a fake
  // successful engine run. The fixture itself must not own the global slot.
  sDramSweepNativeContext=NULL;PioneerDramSweepNativeReply reply;
  const status_t result=RunPioneerDramReadSweepNative(&f.fd,0,14,1,1,reply);
  assert(unreserves==1 && !reply.complete && !pciWrites && views.empty());
  if(fail){assert(result==B_DEV_NOT_READY && reply.retained && sDramSweepNativeContext);
   auto* retained=sDramSweepNativeContext;sDramSweepNativeContext=NULL;delete retained;refused++;}
  else{assert(result==B_NOT_ALLOWED && reply.reservationClosed && !reply.retained);valid++;}
  failProfile=failUnreserve=false;
 }
 auto request=[](){tt_pioneer_dram_sweep r={};r.version=1;r.chunk_shift=14;r.sweeps=1;r.nonce=1;
  r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=7;return r;};
 {Fixture f;sDramSweepNativeContext=NULL;auto r=request();
  assert(PioneerDramSweepControl(&f.fd,TT_PCI_PROBE_DRAM_SWEEP_SUPPORT,&r,sizeof(r))==B_OK);
  assert(copyCalls==2 && r.reply.version==1 && !r.reply.complete && !r.reply.preflight && !ioCalls);valid++;}
 for(unsigned word=0;word<38;word++){
  Fixture f;sDramSweepNativeContext=NULL;auto r=request();
  reinterpret_cast<uint32*>(&r.reply)[word]=1;
  assert(PioneerDramSweepControl(&f.fd,TT_PCI_PROBE_DRAM_SWEEP_RUN,&r,sizeof(r))==B_BAD_VALUE);
  assert(copyCalls==1 && !ioCalls && !unreserves);refused++;
 }
 for(unsigned fault=0;fault<16;fault++){
  Fixture f;sDramSweepNativeContext=NULL;auto r=request();
  if(fault==0)r.version=2;
  if(fault==1)r.bank=7;
  if(fault==2)r.chunk_shift=7;
  if(fault==3)r.sweeps=0;
  if(fault==4)r.sweeps=257;
  if(fault==5)r.nonce=0;
  if(fault>=6 && fault<=8)r.reserved[fault-6]=1;
  if(fault==9)r.expected_tag=0;
  if(fault==10)r.expected_generation=0;
  if(fault==11)effectiveUid=1;
  if(fault==12)f.fd.magic=0;
  if(fault==13)currentTeam++;
  if(fault==14)failCopy=1;
  if(fault==15)r.expected_generation++;
  assert(PioneerDramSweepControl(&f.fd,TT_PCI_PROBE_DRAM_SWEEP_PREFLIGHT,&r,sizeof(r))!=B_OK);
  assert(!ioCalls && !unreserves && !sColdRecoveryRequired);refused++;
 }
 for(unsigned fault=0;fault<5;fault++){
  Fixture f;sDramSweepNativeContext=NULL;auto r=request();
  const uint32 op=fault==0?UINT32_MAX:TT_PCI_PROBE_DRAM_SWEEP_SUPPORT;
  void* data=fault==1?NULL:&r;size_t size=fault==2?sizeof(r)-1:fault==3?sizeof(r)+1:sizeof(r);
  if(fault==4)failCopy=2;
  assert(PioneerDramSweepControl(&f.fd,op,data,size)!=B_OK && !ioCalls && !unreserves && !sColdRecoveryRequired);refused++;
 }
 {Fixture f;sDramSweepNativeContext=NULL;failProfile=true;auto r=request();
  assert(PioneerDramSweepControl(&f.fd,TT_PCI_PROBE_DRAM_SWEEP_RUN,&r,sizeof(r))==B_NOT_ALLOWED);
  assert(copyCalls==2 && unreserves==1 && r.reply.reservation_closed && !r.reply.complete && !r.reply.preflight);
  assert(!pciWrites && views.empty() && !sColdRecoveryRequired);failProfile=false;valid++;}
 {auto r=request();auto& p=r.reply;p.version=1;p.sweep[0]=1;
  assert(PioneerDramSweepReply(r,TT_PCI_PROBE_DRAM_SWEEP_SUPPORT));
  p.phase=3;p.preflight=p.views_closed=p.pci_closed=p.reservation_closed=1;
  assert(PioneerDramSweepReply(r,TT_PCI_PROBE_DRAM_SWEEP_PREFLIGHT));
  p.preflight=0;p.complete=1;p.busy_mhz=1350;p.idle_mhz=800;
  auto* s=p.sweep;s[1]=s[4]=s[5]=s[6]=s[7]=1;s[2]=10;
  s[8]=s[9]=6490;s[10]=4096;s[11]=r.bank;
  auto* v=s+12;v[0]=0x31524244;v[1]=1;v[2]=r.nonce;v[3]=18|(12<<6);
  v[4]=16384;v[5]=1;v[6]=0x800000;v[7]=512;v[8]=100;v[10]=500;v[12]=400;
  assert(PioneerDramSweepReply(r,TT_PCI_PROBE_DRAM_SWEEP_RUN));valid+=3;
  for(unsigned i=0;i<38;i++){
   auto bad=r;reinterpret_cast<uint32*>(&bad.reply)[i]^=1;
   const bool okay=PioneerDramSweepReply(bad,TT_PCI_PROBE_DRAM_SWEEP_RUN);
   // Firmware busy MHz and two arbitrary payload samples are DATA,not
   // fixed expected reference results or whole-memory integrity proofs.
   if(i==4 || i==35 || i==36){assert(okay);valid++;}
   else{assert(!okay);refused++;}
  }
  auto bad=r;bad.chunk_shift=UINT32_MAX;assert(!PioneerDramSweepReply(bad,TT_PCI_PROBE_DRAM_SWEEP_RUN));refused++;
 }
 printf("actual Native.inc endpoint lifecycle: %u valid / %u refused; NOT native hardware, ARC or engine execution\n",valid,refused);
}
