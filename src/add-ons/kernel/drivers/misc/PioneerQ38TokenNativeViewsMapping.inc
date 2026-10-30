/* SPDX-License-Identifier: MIT */
// Inside the exact native-only PioneerSessionIO,not an inherited Unmap path.
PioneerQ38TokenViewLease q38TokenViews;
bool TokenViewLeaseIdentity(const PioneerQ38TokenViewLease& views)const{return &views==&q38TokenViews;}
#ifndef PIONEER_Q38_TOKEN_NATIVE_VIEW_ENDPOINT_TEST
__attribute__((noinline)) area_id* TokenViewHandle(unsigned index){
 area_id* views[]={&q38TokenMemoryArea[0],&q38TokenMemoryArea[1],&q38TokenMemoryArea[2],&q38TokenMemoryArea[3],
  &q38PanelArea,&q38FfnCodeArea,&runtimeAreas[0],&runtimeAreas[1],&runtimeAreas[2],&runtimeAreas[3],&runtimeAreas[4],
  &attentionArea,&bulkArea,&coordinatorArea,&layerArea,&fleetArea,
  &this->PioneerHostReadIO::scratchArea,&this->PioneerHostReadIO::writeArea,&this->PioneerHostDmaAuditIO::hdmaArea,
  &this->PioneerHostPathIO::inboundArea,&this->PioneerHostPathIO::siiArea,&this->PioneerComputeNocIO::nocArea,
  &this->PioneerDramSamplesIO::dataArea,&this->PioneerDramSamplesIO::dramArea,&this->PioneerTensixIO::resetArea,
  &this->PioneerTelemetryIO::csmArea,&this->PioneerArcIO::scratchArea,&this->PioneerArcIO::tlbArea,&this->PioneerMmioIO::area};
 static_assert(sizeof(views)/sizeof(views[0])==PioneerQ38TokenViewLease::Count,"ALL29 native+runtime+inherited MMIO aliases,not host RAM");
 return index<PioneerQ38TokenViewLease::Count?views[index]:NULL;
}
__attribute__((noinline)) area_id TokenHostView(unsigned index){
 const area_id views[]={PioneerHostPageIO::area,outputArea,wideArea};return index<3?views[index]:-1;
}
__attribute__((noinline)) uintptr_t TokenHostPointer(unsigned index){
 return index==0?reinterpret_cast<uintptr_t>(PioneerHostPageIO::page):index==1?reinterpret_cast<uintptr_t>(outputPage)
  :index==2?reinterpret_cast<uintptr_t>(widePage):0;
}
__attribute__((noinline)) uint64 TokenHostPhysical(unsigned index){return index==0?PioneerHostPageIO::physical:index==1?outputPhysical:index==2?widePhysical:0;}
__attribute__((noinline)) bool TokenHostAttempted(unsigned index){return index==0?PioneerHostPageIO::releaseAttempted:index==1?outputReleaseAttempted:index==2?wideReleaseAttempted:true;}
__attribute__((noinline)) bool TokenHostQuarantined(unsigned index){return index==0?PioneerHostPageIO::quarantined:index==1?outputQuarantined:index==2?wideQuarantined:true;}
__attribute__((noinline)) status_t TokenUnreserveDeviceRaw(){
 if(!sPCI || !sPCI->unreserve_device)return B_DEV_NOT_READY;
 return sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
} // SAME reservation cookie and native PCI module as original BEGIN;status MUST be consumed
__attribute__((noinline)) void TokenClearViewPointers(){
 for(auto& view:q38TokenMemoryView)view=NULL;
 q38PanelView=q38FfnCodeView=NULL;
 runtimeEntry=runtimeCode=runtimeBuffers=runtimeControl=NULL;runtimePc=NULL;runtimeMapped=false;fleetEnabled=false;
 attentionView=bulkView=coordinatorView=layerView=fleetView=NULL;
 PioneerHostReadIO::scratchData=PioneerHostReadIO::niuWrite=NULL;
 PioneerHostDmaAuditIO::hdma=NULL;PioneerHostPathIO::inbound=PioneerHostPathIO::sii=NULL;
 PioneerComputeNocIO::nocRegisters=NULL;PioneerDramSamplesIO::dramNiu=NULL;PioneerDramSamplesIO::data=NULL;
 PioneerTensixIO::reset=NULL;PioneerTelemetryIO::csm=NULL;PioneerArcIO::scratch=NULL;PioneerArcIO::tlb=NULL;PioneerMmioIO::nodeID=NULL;
}
#else
#if !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE != 1 \
 || PIONEER_Q38_TOKEN_NATIVE_VIEW_ENDPOINT_TEST != 1 || defined(PIONEER_Q38_WORK_INFERENCE)
#error VM endpoint substitution is TEST ONLY,not a native or Work grant
#endif
area_id* TokenViewHandle(unsigned);area_id TokenHostView(unsigned);void TokenClearViewPointers();
uintptr_t TokenHostPointer(unsigned);uint64 TokenHostPhysical(unsigned);bool TokenHostAttempted(unsigned);bool TokenHostQuarantined(unsigned);
status_t TokenUnreserveDeviceRaw();
#endif
bool TokenViewDelete(area_id);
