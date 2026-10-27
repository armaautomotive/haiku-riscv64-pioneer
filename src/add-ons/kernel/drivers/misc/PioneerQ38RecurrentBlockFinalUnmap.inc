/* SPDX-License-Identifier: MIT */
// Inside PioneerSessionIO. ALL restoration precedes this VM-only stage.
#include "PioneerQ38RecurrentBlockFinalReleaseRole.h"
bool RecurrentBlockFinalReleaseScope();
bool q38RecurrentBlockFinalUnmapAttempted=false,q38RecurrentBlockFinalUnmapped=false;
bool RecurrentBlockFinalUnmap(){
 if(q38RecurrentBlockFinalUnmapAttempted)return q38RecurrentBlockFinalUnmapped;
 if(!RecurrentBlockFinalReleaseScope())return false;
 q38RecurrentBlockFinalUnmapAttempted=true;
 area_id* views[]={&q38RecurrentBlockMemoryArea[0],&q38RecurrentBlockMemoryArea[1],&q38RecurrentBlockMemoryArea[2],&q38RecurrentBlockMemoryArea[3],
  &q38EmbeddingMemoryArea,&q38PanelArea,&q38FfnCodeArea,&runtimeAreas[0],&runtimeAreas[1],&runtimeAreas[2],&runtimeAreas[3],&runtimeAreas[4],
  &attentionArea,&bulkArea,&coordinatorArea,&layerArea,&fleetArea,
  &this->PioneerHostReadIO::scratchArea,&this->PioneerHostReadIO::writeArea,&this->PioneerHostDmaAuditIO::hdmaArea,
  &this->PioneerHostPathIO::inboundArea,&this->PioneerHostPathIO::siiArea,&this->PioneerComputeNocIO::nocArea,
  &this->PioneerDramSamplesIO::dataArea,&this->PioneerDramSamplesIO::dramArea,&this->PioneerTensixIO::resetArea,
  &this->PioneerTelemetryIO::csmArea,&this->PioneerArcIO::scratchArea,&this->PioneerArcIO::tlbArea,&this->PioneerMmioIO::area};
 static_assert(sizeof(views)/sizeof(views[0])==30,"complete recurrent+prefix+runtime+inherited MMIO aliases;NOT host RAM");
 const area_id hostViews[]={PioneerHostPageIO::area,outputArea,wideArea};
 for(unsigned i=0;i<30;i++){
  if(*views[i]<-1)return false;
  if(*views[i]>=B_OK){
   for(auto host:hostViews)if(host>=B_OK && *views[i]==host)return false;
   for(unsigned j=0;j<i;j++)if(*views[j]==*views[i])return false;
  }
 }
 for(auto* view:views)if(*view>=B_OK){
  if(delete_area(*view)!=B_OK)return false; // FIRST failure fences ALL later disposal/IO
  *view=-1;
 }
 for(auto& view:q38RecurrentBlockMemoryView)view=NULL;
 q38EmbeddingMemoryView=q38PanelView=q38FfnCodeView=NULL;
 runtimeEntry=runtimeCode=runtimeBuffers=runtimeControl=NULL;runtimePc=NULL;runtimeMapped=false;fleetEnabled=false;
 attentionView=bulkView=coordinatorView=layerView=fleetView=NULL;
 PioneerHostReadIO::scratchData=PioneerHostReadIO::niuWrite=NULL;
 PioneerHostDmaAuditIO::hdma=NULL;PioneerHostPathIO::inbound=PioneerHostPathIO::sii=NULL;
 PioneerComputeNocIO::nocRegisters=NULL;PioneerDramSamplesIO::dramNiu=NULL;PioneerDramSamplesIO::data=NULL;
 PioneerTensixIO::reset=NULL;PioneerTelemetryIO::csm=NULL;PioneerArcIO::scratch=NULL;PioneerArcIO::tlb=NULL;PioneerMmioIO::nodeID=NULL;
 // Wired host buffers and CPU journals remain owned until actual PCI END.
 q38RecurrentBlockFinalUnmapped=true;return true;
}
