/* SPDX-License-Identifier: MIT */
// TEST bad observations AFTER real native preparation accesses. No owner,
// PC/history,model product,ready or completion is supplied to the controller.
bool PioneerSessionIO::NativePreparationObservation(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:nullptr;
 return preparationFault && !preparationFaultSeen && c && c->q38DispatchRoutes && s
  && s->phase==PioneerQ38TokenNativeState::DispatchPreparing;
}
uint32 PioneerSessionIO::NativePreparationFifoObserved(uint32 address,uint32 value){
 if(!NativePreparationObservation())return value;
 preparationFifoEvents.push_back(runtimeCore*0x100+address);
 const bool enabled=bool(C().config&0x10000U);
 const bool hit=preparationFault==1?!runtimeCore && address==0x64
  :preparationFault==2?runtimeCore==1 && !enabled && address==0x64
  :preparationFault==3?runtimeCore==1 && enabled && address==0x64
  :preparationFault==4 && runtimeCore==1 && enabled && address==0x68;
 if(!hit)return value;
 preparationFaultSeen++;preparationFaultTouches=touches;return address==0x64?(enabled?0x1010100fU:1U):1U;
}
void PioneerSessionIO::DramTlbWrite(unsigned slot,unsigned word,uint32 value){
 TokenHeldEndpoints::DramTlbWrite(slot,word,value);
 if(NativePreparationObservation())preparationMapWrites++;
}
uint32 PioneerSessionIO::DramTlbRead(unsigned slot,unsigned word){
 const uint32 actual=TokenHeldEndpoints::DramTlbRead(slot,word);
 if(NativePreparationObservation() && preparationFault==5 && !runtimeCore && !slot && !word){
  assert(preparationMapWrites==1);preparationFaultSeen++;preparationFaultTouches=touches;return actual^1U;
 }return actual;
}
void PioneerSessionIO::ResidentNocWrite(uint32 address,uint32 value){
 TokenHeldEndpoints::ResidentNocWrite(address,value);
 if(NativePreparationObservation() && preparationFault==6 && runtimeCore==1 && address==0x100){
  assert(C().config==value && (value&0x10000U));sReloadGeneration++;preparationFaultSeen++;preparationFaultTouches=touches;
 }
}
void PioneerSessionIO::Q38PanelWrite(uint32 address,uint32 value){
 TokenHeldEndpoints::Q38PanelWrite(address,value);
 if(NativePreparationObservation()){
  preparationPanelWrites++;
  if(preparationFault==7 && !runtimeCore && address==0x70000){
   assert(preparationPanelWrites==1);sReloadGeneration++;preparationFaultSeen++;preparationFaultTouches=touches;
  }
 }
}
uint32 PioneerSessionIO::Q38PanelRead(uint32 address){
 const uint32 actual=TokenHeldEndpoints::Q38PanelRead(address);
 if(NativePreparationObservation() && preparationFault==8 && !runtimeCore && address==0x70000 && preparationPanelWrites){
  assert(preparationPanelWrites==1);preparationFaultSeen++;preparationFaultTouches=touches;return actual^1U;
 }return actual;
}
