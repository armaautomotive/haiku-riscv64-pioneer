/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TOKEN_PCI_RAW_TEST 1
#include "pioneer_q38_token_pci_raw.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPciLease.h"
#include <vector>
struct IO {
 TokenPciRaw raw;tt_pioneer_mmio_result result={};tt_pioneer_mmio_transaction transaction={};
 PioneerQ38TokenPciLease lease;
 bool capture=true,eligible=false,transportLost=false;
 unsigned readRefusal=0,checkedReads=0,base=0,lossAt=0;
 uint64_t endpoint=0x02030000;
 IO(){assert(BlackholePioneerMmioBegin(raw,result,transaction) && raw.writes==11 && raw.mmioReads==3);base=raw.events;}
 bool TokenPciLeaseIdentity(const PioneerQ38TokenPciLease& p)const{return &p==&lease;}
 const void* TokenPciTransport(){return transportLost?nullptr:&raw;}
 uint64_t TokenPciEndpoint(){return endpoint;}
 bool TokenPciCaptureScope(){return capture && lease.Pristine();}
 bool TokenPciScope(){return eligible;}
 bool TokenPciReadChecked(bool root,unsigned offset,unsigned width,uint32_t& value){
  if(!eligible)return false;
  const uint32_t actual=root?raw.RootRead(offset,width):raw.CardRead(offset,width);
  if(++checkedReads==readRefusal)return false;
  value=actual;return true;
 }
 bool TokenPciWriteChecked(bool root,unsigned offset,unsigned width,uint32_t value){
  if(!eligible || !lease.PermitsWrite(root,offset,width,value))return false;
  if(root)raw.RootWrite(offset,width,value);else raw.CardWrite(offset,width,value);return true;
 }
 void Prepare(){assert(lease.Capture(*this,result,transaction) && lease.Prepared());capture=false;eligible=true;raw.mapped=false;}
 bool Close(){return lease.Close(*this,result,transaction);}
 void ObserveLoss(){
  raw.observer=this;raw.after=[](void* p,bool,unsigned,unsigned,bool){auto& io=*static_cast<IO*>(p);
   if(io.raw.events==io.base+io.lossAt)io.eligible=false;};
 }
}; // isolated OS/PCI endpoint DATA only;not a supplied genuine native owner
int main(){
 unsigned cases=0,events=0,reads=0;
 {
  IO io;PioneerQ38TokenPciLease foreign;assert(!foreign.Capture(io,io.result,io.transaction) && foreign.Pristine());
  assert(!io.Close() && io.raw.events==io.base);io.Prepare();
  auto copied=io.result;assert(!io.lease.Close(io,copied,io.transaction) && io.raw.events==io.base && io.lease.Prepared());
  assert(io.Close() && io.lease.Complete() && io.lease.Matches(io,io.result,io.transaction)
   && io.lease.Writes()==11 && io.raw.writes==22 && io.result.restored && io.result.stage==5);
  events=io.raw.events-io.base;reads=io.lease.Reads();assert(events==reads+11);
  assert(!io.transaction.map_attempted && !io.transaction.config_attempted && !io.raw.command[0] && !io.raw.command[1]);
  for(unsigned i=0;i<6;i++)assert(io.raw.bars[i]==(i%2?0U:12U));
  assert(io.raw.bridge[0]==0x0001fff1 && !io.raw.bridge[1] && !io.raw.bridge[2] && io.raw.mmioReads==3);
  const unsigned done=io.raw.events;assert(!io.Close() && io.raw.events==done);cases++;
 }
 for(unsigned nth=1;nth<=events;nth++){
  IO io;io.Prepare();io.lossAt=nth;io.ObserveLoss();
  assert(!io.Close() && io.lease.Unsafe() && !io.lease.Complete() && !io.result.restored
   && io.result.state==TT_PIONEER_MMIO_RESTORE_FAILED && io.raw.events==io.base+nth && io.raw.mmioReads==3);
  const unsigned done=io.raw.events;io.eligible=true;assert(!io.Close() && io.raw.events==done);cases++;
 }
 for(unsigned nth=1;nth<=reads;nth++){
  IO io;io.Prepare();io.readRefusal=nth;
  assert(!io.Close() && io.lease.Unsafe() && io.checkedReads==nth && !io.result.restored && io.raw.mmioReads==3);
  const unsigned done=io.raw.events;assert(!io.Close() && io.raw.events==done);cases++;
 }
 unsigned harmlessDrops=0;
 for(unsigned nth=1;nth<=11;nth++){
  IO io;io.Prepare();io.raw.droppedWrite=11+nth;
  const bool okay=io.Close();
  if(okay){harmlessDrops++;assert(io.lease.Complete() && io.result.restored);}
  else assert(io.lease.Unsafe() && !io.result.restored);
  const unsigned done=io.raw.events;assert(!io.Close() && io.raw.events==done);cases++;
 }
 for(unsigned kind=0;kind<14;kind++){
  IO io;io.Prepare();
  if(kind==0)io.result.bars_before[5]++;if(kind==1)io.result.bars_trial[4]++;if(kind==2)io.result.bridge_before[1]++;
  if(kind==3)io.result.node_id[2]++;if(kind==4)io.result.commands_before[0]=4;if(kind==5)io.result.profile_stage++;
  if(kind==6)io.result.state++;if(kind==7)io.result.restored=1;if(kind==8)io.result.stage++;
  if(kind==9)io.transaction.config_attempted=false;if(kind==10)io.transaction.map_attempted=false;
  if(kind==11)io.endpoint++;if(kind==12)io.transportLost=true;if(kind==13)io.eligible=false;
  assert(!io.Close() && io.raw.events==io.base && !io.lease.Complete());cases++;
 }
 for(unsigned kind=0;kind<9;kind++){
  IO io;io.Prepare();
  if(kind==0)io.raw.pcieStatus[0]=0x20;if(kind==1)io.raw.pcieStatus[1]=1;if(kind==2)io.raw.command[0]=0x406;
  if(kind==3)io.raw.msiControl=1;if(kind==4)io.raw.msixControl=0x8000;if(kind==5)io.raw.pmControl=3;
  if(kind==6)io.raw.capCycle=true;if(kind==7)io.raw.bars[0]^=0x1000;if(kind==8)io.raw.bridge[2]++;
  assert(!io.Close() && io.lease.Unsafe() && !io.result.restored && io.raw.writes==11);cases++;
 }
 printf("PASS PCI-journal %u cases/actual simulated Blackhole BEGIN/%u checked closing reads+11 exact ordered writes/every%u post-access lifetime loss+every read refusal/metadata and pending-error-capability mismatches/harmless idempotent drops%u/NO following IO/retry/Unmap/host release;endpoint DATA only,NOT native END/hardware/TPS qualification\n",cases,reads,events,harmlessDrops);
}
