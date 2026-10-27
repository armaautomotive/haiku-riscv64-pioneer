/* SPDX-License-Identifier: MIT */
// INSIDE actual PioneerSessionIO. No destructor fallback or inherited
// diagnostic Unmap:ALL hardware restoration was already proved complete.
#include "PioneerQ38MixedLiveFinalReleaseRole.h"
 bool MixedLiveFinalReleaseScope();
 bool q38MixedLiveFinalUnmapAttempted=false,q38MixedLiveFinalUnmapped=false;
 bool MixedLiveFinalUnmap(){
  if(q38MixedLiveFinalUnmapAttempted)return q38MixedLiveFinalUnmapped; // CPU-only terminal result,NO retry
  if(!MixedLiveFinalReleaseScope())return false;
  q38MixedLiveFinalUnmapAttempted=true;
  area_id* views[]={&q38MixedLiveMemoryArea[0],&q38MixedLiveMemoryArea[1],&q38MixedLiveMemoryArea[2],&q38MixedLiveMemoryArea[3],
   &q38EmbeddingMemoryArea,&q38PanelArea,&q38FfnCodeArea,&runtimeAreas[0],&runtimeAreas[1],&runtimeAreas[2],&runtimeAreas[3],&runtimeAreas[4],
   &attentionArea,&bulkArea,&coordinatorArea,&layerArea,&fleetArea,
   &this->PioneerHostReadIO::scratchArea,&this->PioneerHostReadIO::writeArea,&this->PioneerHostDmaAuditIO::hdmaArea,
   &this->PioneerHostPathIO::inboundArea,&this->PioneerHostPathIO::siiArea,&this->PioneerComputeNocIO::nocArea,
   &this->PioneerDramSamplesIO::dataArea,&this->PioneerDramSamplesIO::dramArea,&this->PioneerTensixIO::resetArea,
   &this->PioneerTelemetryIO::csmArea,&this->PioneerArcIO::scratchArea,&this->PioneerArcIO::tlbArea,&this->PioneerMmioIO::area};
  static_assert(sizeof(views)/sizeof(views[0])==30,"complete mixed+prefix+runtime+inherited MMIO aliases;NOT host RAM");
  const area_id hostViews[]={PioneerHostPageIO::area,outputArea,wideArea};
  // Inspect ALL fixed owned handles before the FIRST deletion. A duplicate
  // or invalid negative handle must not dispose a foreign view or retry.
  for(unsigned i=0;i<30;i++){
   if(*views[i]<-1)return false;
   if(*views[i]>=B_OK){
    for(auto host:hostViews)if(host>=B_OK && *views[i]==host)return false; // NEVER free a wired host buffer here
    for(unsigned j=0;j<i;j++)if(*views[j]==*views[i])return false;
   }
  }
  for(auto* view:views)if(*view>=B_OK){
   if(delete_area(*view)!=B_OK)return false; // FIRST failure stops ALL later VM/physical IO
   *view=-1;
  }
  for(auto& view:q38MixedLiveMemoryView)view=NULL;
  q38EmbeddingMemoryView=q38PanelView=q38FfnCodeView=NULL;
  runtimeEntry=runtimeCode=runtimeBuffers=runtimeControl=NULL;runtimePc=NULL;runtimeMapped=false;fleetEnabled=false;
  attentionView=bulkView=coordinatorView=layerView=fleetView=NULL;
  PioneerHostReadIO::scratchData=PioneerHostReadIO::niuWrite=NULL;
  PioneerHostDmaAuditIO::hdma=NULL;PioneerHostPathIO::inbound=PioneerHostPathIO::sii=NULL;
  PioneerComputeNocIO::nocRegisters=NULL;PioneerDramSamplesIO::dramNiu=NULL;PioneerDramSamplesIO::data=NULL;
  PioneerTensixIO::reset=NULL;PioneerTelemetryIO::csm=NULL;PioneerArcIO::scratch=NULL;PioneerArcIO::tlb=NULL;PioneerMmioIO::nodeID=NULL;
  // Do NOT release the wired host page/output/wide buffers or CPU journals.
  // The actual END caller still owes final PCI disable/restore BEFORE those.
  q38MixedLiveFinalUnmapped=true;return true;
 }
