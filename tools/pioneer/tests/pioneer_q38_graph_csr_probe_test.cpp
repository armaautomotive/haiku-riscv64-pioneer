/* SPDX-License-Identifier: MIT */
// Simulated CSR/NoC endpoints. NO native mapping/passivity qualification.
#define PIONEER_Q38_DISPATCH_BOOTSTRAP_MOCK_ONLY
#include "pioneer_q38_dispatch_bootstrap_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphCsrProbe.h"
struct CsrIO:BootstrapIO {
 bool probe=false;unsigned touches=0,observed=0,packets=0,group=0,worker=0,rootWrites=0,aliasReads=0;
 unsigned fault=0,faultWorker=1,faultGroup=0,faultWord=0;
 bool Fired()const{return worker==faultWorker && group==faultGroup;}
 void Touch(){assert(!quarantined);touches++;}
 uint32_t Bad(uint32_t value){observed=touches;return value^1;}
 bool SelectRuntimeCore(unsigned w){if(probe)Touch();aliasReads=0;return BootstrapIO::SelectRuntimeCore(w);}
 uint32_t Q38PanelRead(uint32_t a){if(probe)Touch();uint32_t v=BootstrapIO::Q38PanelRead(a);
  if(probe && fault==1 && !current && Fired() && a==0x75000+(PioneerQ38GraphCsrOffset(group)&31)+faultWord*4)return Bad(v);return v;}
 uint32_t ResetRead(){if(probe)Touch();uint32_t v=BootstrapIO::ResetRead();
  if(probe && fault==7 && current==faultWorker && packets && Fired())return Bad(v);return v;}
 uint32_t SourceRead(unsigned i){if(probe)Touch();return BootstrapIO::SourceRead(i);}
 uint32_t RuntimePcRead(){if(probe)Touch();return BootstrapIO::RuntimePcRead();}
 uint32_t RuntimeCodeRead(unsigned i){if(probe)Touch();return BootstrapIO::RuntimeCodeRead(i);}
 bool Q38PanelMappingStable(){if(probe)Touch();return BootstrapIO::Q38PanelMappingStable();}
 uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t b){if(probe)Touch();uint32_t v=BootstrapIO::HostPciRead(p,o,b);
  if(probe && p==faultWord){
   // Leave a next capability present:bad device status must stop BEFORE
   // following that link,not merely fail after walking the whole list.
   if(fault==8 && o==0x70)return v|0x9000;
   if((fault==8 && o==0x7a) || (fault==9 && o==0x34) || (fault==10 && o==4))return Bad(v);
  }
  return v;}
 uint32_t Physical(unsigned w,unsigned offset){auto& p=*core.at(w);
  for(unsigned i=0;i<16;i++)if(offset==BlackholeDramSourceOffset(i))return i?p.source[i]:p.config;
  const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)if(offset==at[i])return i==2 || i==6?p.accepted:i<4?p.counters[i]:p.writeCounters[i-4];
  assert(false && "reserved remote CSR");return 0;
 }
 uint32_t ResidentNocRead(uint32_t o){if(probe)Touch();uint32_t v=BootstrapIO::ResidentNocRead(o);
  if(probe && fault==11 && current==faultWord && o==0x210 && ++aliasReads==2)return Bad(v);
  if(!probe || !Fired())return v;
  if(fault==5 && !current && o==faultWord*4 && rootWrites%11)return Bad(v);
  if(fault==2 && !current && packets && o==0x204)return Bad(v);
  if(fault==3 && current==faultWorker && packets && o==0x208)return Bad(v);
  if(fault==6 && !current && packets && o==0x100)return Bad(v);
  return v;
 }
 void ResidentNocWrite(uint32_t o,uint32_t v){if(!probe){BootstrapIO::ResidentNocWrite(o,v);return;}
  Touch();assert(!current && (o<=0x24 || o==0x40));auto& p=C();rootWrites++;
  if(o<=0x24){assert(!p.pending);p.command[o/4]=v;
   if(o==0){unsigned offset=v-0xffb20000;group=23;for(unsigned g=0;g<23;g++)if(offset==PioneerQ38GraphCsrOffset(g))group=g;assert(group<23);}
   if(o==8){worker=120;for(unsigned w=1;w<120;w++)if(v==(PioneerWorkerX(w)|(PioneerWorkerY(w)<<6)))worker=w;assert(worker<120);}
   return;
  }
  assert(v==1 && !p.pending && !p.command[1] && !p.command[4] && p.command[5]==129 && !p.command[6] && !p.command[7] && !p.command[9]);
  unsigned offset=PioneerQ38GraphCsrOffset(group),words=PioneerQ38GraphCsrWords(group),local=0x75000+(offset&31);
  assert(p.command[0]==0xffb20000+offset && p.command[3]==local && p.command[8]==words*4);packets++;p.doorbells++;p.doorbellRead=false;
  if(fault==4 && Fired()){p.pending=true;return;}
  for(unsigned i=0;i<words;i++)p.view[(local-TT_Q38_PANEL_VIEW_BASE)/4+i]=Physical(worker,offset+i*4);
  p.counters[0]++;p.counters[1]++;p.accepted++;p.counters[3]++;
 }
 void QuarantineHostPage(){quarantined=true;}
};
struct CsrFixture {
 CsrIO io;std::unique_ptr<PioneerQ38DispatchLease> lease{new PioneerQ38DispatchLease};
 std::vector<std::unique_ptr<PanelOwners>> owners;tt_dram_samples_result dram;PioneerResidentBanks banks;
 explicit CsrFixture(bool live=false){dram=BankProof(*io.core[0]);assert(banks.Bind(dram));
  for(unsigned w=0;w<120;w++){owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=lease->worker[w];
   n.master=&owners[w]->store;n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);
   assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*lease,w));}
  if(live)assert(PioneerQ38DispatchBootstrap(io,dram,banks,*lease));io.probe=true;
 }
 bool Run(PioneerQ38GraphCsrResult& r){return PioneerQ38GraphCsrProbe(io,dram,banks,*lease,r);}
 void Unknown(const PioneerQ38GraphCsrResult& r){assert(r.unsafe && lease->unsafe && banks.Quarantined() && io.quarantined);
  if(io.observed)assert(io.touches==io.observed);unsigned reads=io.touches,writes=io.rootWrites;PioneerQ38GraphCsrResult again;
  assert(!Run(again) && reads==io.touches && writes==io.rootWrites && !again.started);
  for(auto& n:lease->worker)assert(n.image.unsafe && n.Store().recovery_required);
 }
};
int main(){unsigned positive=0,faults=0;
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 for(bool live:{false,true})for(bool wrap:{false,true}){CsrFixture f(live);
  if(wrap){auto& root=*f.io.core[0];root.counters[0]=root.counters[1]=root.counters[3]=root.accepted=0xfffffff0;}
  PioneerQ38GraphCsrResult r;assert(f.Run(r) && r.passed && !r.unsafe && r.live==live && r.workers==119 && r.packets==2737 && r.words==2737);
  assert(f.io.rootWrites==2737*11 && f.io.current==0 && f.io.armed==(live?119U:0U) && f.io.released==(live?119U:0U));
  for(auto& n:f.lease->worker)assert(!n.Store().pending && !n.image.unsafe);
  unsigned reads=f.io.touches,writes=f.io.rootWrites;assert(!f.Run(r) && reads==f.io.touches && writes==f.io.rootWrites);positive++;}
 for(unsigned group=0;group<23;group++)for(unsigned word=0;word<PioneerQ38GraphCsrWords(group);word++){
  CsrFixture f;f.io.fault=1;f.io.faultGroup=group;f.io.faultWord=word;PioneerQ38GraphCsrResult r;
  assert(!f.Run(r) && r.packets==group+1);f.Unknown(r);faults++;}
 for(unsigned fault=2;fault<=7;fault++)for(unsigned worker:{1U,60U,119U}){
  CsrFixture f(true);f.io.fault=fault;f.io.faultWorker=worker;f.io.faultGroup=22;f.io.faultWord=2;PioneerQ38GraphCsrResult r;
  assert(!f.Run(r));f.Unknown(r);faults++;}
 for(unsigned bad=0;bad<4;bad++){CsrFixture f;PioneerQ38GraphCsrResult r;
  if(!bad)f.lease->unsafe=true;if(bad==1)f.lease->rootRunning=true;if(bad==2)f.lease->prepared=119;if(bad==3)f.lease->bootAttempted=true;
  assert(!f.Run(r) && !r.started && !f.io.touches && !f.io.rootWrites);faults++;}
 for(bool live:{false,true})for(unsigned fault=8;fault<=10;fault++)for(unsigned peer:{0U,1U}){
  CsrFixture f(live);f.io.fault=fault;f.io.faultWord=peer;PioneerQ38GraphCsrResult r;
  assert(!f.Run(r) && !r.packets && !f.io.rootWrites);f.Unknown(r);faults++;
 }
 for(bool live:{false,true})for(unsigned core:{0U,1U}){CsrFixture f(live);f.io.fault=11;f.io.faultWord=core;PioneerQ38GraphCsrResult r;
  assert(!f.Run(r) && !r.packets && !f.io.rootWrites);f.Unknown(r);faults++;}
 printf("PASS OFFLINE fixed remote CSR probe:positive%u/faults%u/held+live-idle ALL119/2737packets2737words per pass/ALL23responsewords/exact4B_MMIO_ONLY/first-mid-last faults/first-observed-error no further IO/whole-context quarantine/no resets or cleanup;SIMULATED_NOT_NATIVE_PASSIVITY_OR_FFN_AUTHORITY\n",positive,faults);
}
