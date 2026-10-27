/* SPDX-License-Identifier: MIT */
// EXPLICIT simulated VM-handle layout/endpoints. No native source,closed
// owner,clock/FD authority,host release or physical hardware proof.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using uint32=uint32_t;using area_id=int;
static constexpr int B_OK=0;
static uint32 alias=0x12345678;
static std::vector<int> removed;
static unsigned failDelete=0,inheritedHardwareCleanup=0;
static int delete_area(area_id view){removed.push_back(view);return removed.size()==failDelete?-1:0;}
struct PioneerMmioIO{area_id area=-1;const volatile uint32* nodeID=&alias;};
struct PioneerArcIO:PioneerMmioIO{area_id scratchArea=-1,tlbArea=-1;const volatile uint32* scratch=&alias;volatile uint32* tlb=&alias;};
struct PioneerTelemetryIO:PioneerArcIO{area_id csmArea=-1;const volatile uint32* csm=&alias;};
struct PioneerTensixIO:PioneerTelemetryIO{area_id resetArea=-1;const volatile uint32* reset=&alias;};
struct PioneerDramSamplesIO:PioneerTensixIO{area_id dataArea=-1,dramArea=-1;const volatile uint32* dramNiu=&alias;volatile uint32* data=&alias;};
struct PioneerComputeNocIO:PioneerDramSamplesIO{area_id nocArea=-1;const volatile uint32* nocRegisters=&alias;};
struct PioneerHostPathIO:PioneerComputeNocIO{area_id inboundArea=-1,siiArea=-1;const volatile uint32* inbound=&alias;const volatile uint32* sii=&alias;};
struct PioneerHostDmaAuditIO:PioneerHostPathIO{area_id hdmaArea=-1;const volatile uint32* hdma=&alias;};
struct PioneerHostPageIO{area_id area=900;volatile uint32* page=&alias;};
struct PioneerOutputBufferIO{area_id outputArea=901;volatile uint32* output=&alias;};
struct PioneerWideOutputIO{area_id wideArea=902;volatile uint32* wide=&alias;};
struct PioneerHostReadIO:PioneerHostDmaAuditIO,PioneerHostPageIO{area_id scratchArea=-1,writeArea=-1;volatile uint32* scratchData=&alias;volatile uint32* niuWrite=&alias;};
struct PioneerHostWriteIO:PioneerHostReadIO{bool Unmap(){inheritedHardwareCleanup++;return true;}};
struct FinalUnmapFixture:PioneerHostWriteIO,PioneerOutputBufferIO,PioneerWideOutputIO{
 area_id q38MixedLiveMemoryArea[4]={-1,-1,-1,-1},q38EmbeddingMemoryArea=-1,q38PanelArea=-1,q38FfnCodeArea=-1;
 area_id runtimeAreas[5]={-1,-1,-1,-1,-1},attentionArea=-1,bulkArea=-1,coordinatorArea=-1,layerArea=-1,fleetArea=-1;
 volatile uint32 *q38MixedLiveMemoryView[4]={&alias,&alias,&alias,&alias},*q38EmbeddingMemoryView=&alias,*q38PanelView=&alias,*q38FfnCodeView=&alias;
 volatile uint32 *runtimeEntry=&alias,*runtimeCode=&alias,*runtimeBuffers=&alias,*runtimeControl=&alias;
 const volatile uint32* runtimePc=&alias;
 volatile uint32 *attentionView=&alias,*bulkView=&alias,*coordinatorView=&alias,*layerView=&alias,*fleetView=&alias;
 bool runtimeMapped=true,fleetEnabled=true,q38MixedLiveMemoryMapAttempted=true;
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 // The TEST component scope below is NOT native authority. Actual scope
 // lives in PioneerQ38MixedLiveFinalReleaseScope.inc and is SDK-compiled.
 bool scope=true;unsigned scopeCalls=0;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveFinalUnmap.inc"
#endif
#include "session_unmap.inc" // VERBATIM actual production routing AND generic method
 std::vector<area_id*> Views(){return {
  &q38MixedLiveMemoryArea[0],&q38MixedLiveMemoryArea[1],&q38MixedLiveMemoryArea[2],&q38MixedLiveMemoryArea[3],
  &q38EmbeddingMemoryArea,&q38PanelArea,&q38FfnCodeArea,&runtimeAreas[0],&runtimeAreas[1],&runtimeAreas[2],&runtimeAreas[3],&runtimeAreas[4],
  &attentionArea,&bulkArea,&coordinatorArea,&layerArea,&fleetArea,
  &this->PioneerHostReadIO::scratchArea,&writeArea,&hdmaArea,&inboundArea,&siiArea,&nocArea,&dataArea,&dramArea,&resetArea,
  &csmArea,&this->PioneerArcIO::scratchArea,&tlbArea,&this->PioneerMmioIO::area};}
 FinalUnmapFixture(){auto views=Views();assert(views.size()==30);for(unsigned i=0;i<30;i++)*views[i]=100+int(i);}
 bool AliasesNull()const{
  for(auto* view:q38MixedLiveMemoryView)if(view)return false;
  return !q38EmbeddingMemoryView && !q38PanelView && !q38FfnCodeView && !runtimeEntry && !runtimeCode && !runtimeBuffers && !runtimeControl && !runtimePc
   && !runtimeMapped && !fleetEnabled && !attentionView && !bulkView && !coordinatorView && !layerView && !fleetView
   && !scratchData && !niuWrite && !hdma && !inbound && !sii && !nocRegisters && !dramNiu && !data && !reset && !csm && !PioneerArcIO::scratch && !tlb && !nodeID;
 }
 void HostsRetained()const{assert(PioneerHostPageIO::area==900 && outputArea==901 && wideArea==902 && page==&alias && output==&alias && wide==&alias);}
};
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
bool FinalUnmapFixture::MixedLiveFinalReleaseScope(){scopeCalls++;return scope;}
#endif
