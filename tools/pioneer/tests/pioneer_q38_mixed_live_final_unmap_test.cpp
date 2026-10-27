/* SPDX-License-Identifier: MIT */
#include "PioneerQ38MixedFinalUnmapFixture.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchMmioEnd.h"
struct FinalMmioIO:FinalUnmapFixture{
 uint32_t card[64]={},root[64]={};unsigned pciWrites=0,pciReads=0,failPciWrite=0;
 bool failedRead=false;
 void Write(uint32_t* config,unsigned at,unsigned size,uint32_t value){
  assert(!failedRead && (size==2 || size==4) && !(at&3));pciWrites++;config[at/4]=pciWrites==failPciWrite?value^1:value;
 }
 uint32_t Read(uint32_t* config,unsigned at,unsigned){assert(!failedRead);pciReads++;if(pciWrites==failPciWrite && failPciWrite)failedRead=true;return config[at/4];}
 void CardWrite(unsigned a,unsigned s,uint32_t v){Write(card,a,s,v);}void RootWrite(unsigned a,unsigned s,uint32_t v){Write(root,a,s,v);}
 uint32_t CardRead(unsigned a,unsigned s){return Read(card,a,s);}uint32_t RootRead(unsigned a,unsigned s){return Read(root,a,s);}
};
static tt_pioneer_mmio_result MmioResult(){
 tt_pioneer_mmio_result r={};r.version=1;r.state=TT_PIONEER_MMIO_READ_OK;
 for(unsigned i=0;i<6;i++)r.bars_before[i]=i%2?0:0x0c;
 r.bridge_before[0]=0x1fff1;return r;
}
int main(){unsigned faults=0,refusals=0;
 for(unsigned fail=0;fail<=30;fail++){
  removed.clear();failDelete=fail;FinalUnmapFixture io;auto views=io.Views();
  assert(io.MixedLiveFinalUnmap()==!fail && io.q38MixedLiveFinalUnmapAttempted && io.q38MixedLiveFinalUnmapped==!fail && !inheritedHardwareCleanup);
  assert(removed.size()==(fail?fail:30) && io.scopeCalls==1);
  for(unsigned i=0;i<removed.size();i++)assert(removed[i]==int(100+i));
  for(unsigned i=0;i<30;i++)assert(*views[i]==(i<(fail?fail-1:30)?-1:int(100+i)));
  auto before=removed;assert(io.MixedLiveFinalUnmap()==!fail && io.scopeCalls==1 && before==removed);
  if(!fail)assert(io.AliasesNull());io.HostsRetained();if(fail)faults++;
 }
 for(unsigned missing=0;missing<30;missing++){
  removed.clear();failDelete=0;FinalUnmapFixture io;*io.Views()[missing]=-1;
  assert(io.MixedLiveFinalUnmap() && removed.size()==29 && io.AliasesNull());io.HostsRetained();
 }
 for(unsigned first=0;first<30;first++)for(unsigned second=first+1;second<30;second++){
  removed.clear();FinalUnmapFixture io;auto views=io.Views();*views[second]=*views[first];
  assert(!io.MixedLiveFinalUnmap() && !io.MixedLiveFinalUnmap() && io.scopeCalls==1 && removed.empty());refusals++;
 }
 for(unsigned invalid=0;invalid<30;invalid++){
  removed.clear();FinalUnmapFixture io;*io.Views()[invalid]=-2;
  assert(!io.MixedLiveFinalUnmap() && !io.MixedLiveFinalUnmap() && io.scopeCalls==1 && removed.empty());refusals++;
 }
 for(unsigned view=0;view<30;view++)for(unsigned host=900;host<=902;host++){
  removed.clear();FinalUnmapFixture io;*io.Views()[view]=int(host);
  assert(!io.MixedLiveFinalUnmap() && !io.MixedLiveFinalUnmap() && io.scopeCalls==1 && removed.empty());io.HostsRetained();refusals++;
 }
 removed.clear();FinalUnmapFixture io;io.scope=false;
 assert(!io.MixedLiveFinalUnmap() && !io.q38MixedLiveFinalUnmapAttempted && removed.empty());
 printf("PASS actual fixed30view VM-only final Unmap/ALL30 deletion faults stop immediately/no repeated scope or retry/555 duplicate+negative+host-alias refusals/30 single-absent handle layouts/all aliases invalidated on success/ALL host buffers retained;faults%u refusals%u;EXPLICIT simulated scope+VM endpoints ONLY,NO genuine native release/ARC/host/PCI/physical proof\n",faults,refusals);
 for(unsigned failure=0;failure<=30;failure++){
  removed.clear();failDelete=failure;FinalMmioIO final;auto result=MmioResult();tt_pioneer_mmio_transaction transaction={true,true};
  assert(PioneerQ38DispatchMmioEnd(final,result,transaction)==!failure && !transaction.config_attempted);
  assert(removed.size()==(failure?failure:30) && final.pciWrites==(failure?0:11) && result.restored==!failure);
  if(failure)assert(!final.pciReads && transaction.map_attempted && result.state==TT_PIONEER_MMIO_RESTORE_FAILED);
  final.HostsRetained();auto writes=final.pciWrites,reads=final.pciReads;auto unmaps=removed.size();
  assert(PioneerQ38DispatchMmioEnd(final,result,transaction)==!failure && writes==final.pciWrites && reads==final.pciReads && unmaps==removed.size());
 }
 for(unsigned failure=1;failure<=11;failure++){
  removed.clear();failDelete=0;FinalMmioIO final;final.failPciWrite=failure;auto result=MmioResult();tt_pioneer_mmio_transaction transaction={true,true};
  assert(!PioneerQ38DispatchMmioEnd(final,result,transaction) && removed.size()==30 && final.pciWrites==failure
   && final.failedRead && !result.restored && !transaction.config_attempted);
  auto reads=final.pciReads;assert(!PioneerQ38DispatchMmioEnd(final,result,transaction) && final.pciWrites==failure && final.pciReads==reads && removed.size()==30);
  final.HostsRetained();
 }
 puts("PASS composed ACTUAL DispatchMmioEnd+fixed30view Unmap:ALL30 VM failures fence EVERY PCI write/read,ALL11 PCI write/readback failures fence following IO,repeat attempts inert,ALL host buffers retained;SIMULATED scope/VM/PCI ONLY,NOT whole native END or physical qualification");
}
