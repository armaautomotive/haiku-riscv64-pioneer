/* SPDX-License-Identifier: MIT */
// TEST VM/legacy-probe layout ONLY. No source/closed/clock/scope setter.
// Actual private native state and real clock lease live in the factory.
static uint32 finalAlias=0x12345678;
static std::vector<area_id> finalRemoved;
static unsigned finalDeleteFailure=0,finalPciFailure=0,finalDestroyed=0;
static bool finalDeleteFailed=false,finalPciFailed=false;
static int delete_area(area_id view){
 assert(!finalDeleteFailed && !finalPciFailed);finalRemoved.push_back(view);
 if(finalRemoved.size()==finalDeleteFailure){finalDeleteFailed=true;return -1;}return B_OK;
}
struct PioneerMmioIO{area_id area=-1;const volatile uint32* nodeID=&finalAlias;};
struct PioneerArcIO:PioneerMmioIO{area_id scratchArea=-1,tlbArea=-1;const volatile uint32* scratch=&finalAlias;volatile uint32* tlb=&finalAlias;bool dirty=false;};
struct PioneerTelemetryIO:PioneerArcIO{area_id csmArea=-1;const volatile uint32* csm=&finalAlias;};
struct PioneerTensixIO:PioneerTelemetryIO{area_id resetArea=-1;const volatile uint32* reset=&finalAlias;};
struct PioneerDramSamplesIO:PioneerTensixIO{area_id dataArea=-1,dramArea=-1;const volatile uint32* dramNiu=&finalAlias;volatile uint32* data=&finalAlias;};
struct PioneerComputeNocIO:PioneerDramSamplesIO{area_id nocArea=-1;const volatile uint32* nocRegisters=&finalAlias;};
struct PioneerHostPathIO:PioneerComputeNocIO{area_id inboundArea=-1,siiArea=-1;const volatile uint32* inbound=&finalAlias;const volatile uint32* sii=&finalAlias;};
struct PioneerHostDmaAuditIO:PioneerHostPathIO{area_id hdmaArea=-1;const volatile uint32* hdma=&finalAlias;
 bool AfterNodeId(uint32){assert(false && "fresh diagnostic recheck is not yet supplied by this raw endpoint fixture");return false;}};
struct PioneerHostReadIO:PioneerHostDmaAuditIO{area_id scratchArea=-1,writeArea=-1;volatile uint32* scratchData=&finalAlias;volatile uint32* niuWrite=&finalAlias;};
struct PioneerHostWriteIO:PioneerHostReadIO{bool Unmap(){assert(false && "NO inherited diagnostic cleanup");return false;}};
struct MixedFinalNativeLayout:PioneerHostWriteIO{
 area_id q38PanelArea=-1,q38FfnCodeArea=-1,runtimeAreas[5]={-1,-1,-1,-1,-1},attentionArea=-1,bulkArea=-1,coordinatorArea=-1,layerArea=-1,fleetArea=-1;
 area_id outputArea=9002,wideArea=9003;
 volatile uint32 *q38PanelView=&finalAlias,*q38FfnCodeView=&finalAlias,*runtimeCode=&finalAlias,*runtimeBuffers=&finalAlias,*runtimeControl=&finalAlias;
 const volatile uint32* runtimePc=&finalAlias;
 volatile uint32 *attentionView=&finalAlias,*bulkView=&finalAlias,*coordinatorView=&finalAlias,*layerView=&finalAlias,*fleetView=&finalAlias;
 volatile uint32 *outputPage=&finalAlias,*widePage=&finalAlias;
 bool outputReleaseAttempted=false,outputQuarantined=false,wideReleaseAttempted=false;
 bool telemetryAttempted=false,resetAttempted=false,dramAttempted=false,nocAttempted=false,pathAttempted=false,auditAttempted=false,readAttempted=false;
 tt_arc_scratch_result arc={};tt_arc_telemetry_result telemetry={};tt_tensix_reset_result tensix={};
 tt_host_path_result path={};tt_host_dma_audit_result audit={};tt_host_read_result read={};
 uint32 finalCard[64]={},finalRoot[64]={};unsigned finalPciWrites=0,finalPciReads=0;
 MixedFinalNativeLayout(){finalCard[1]=finalRoot[1]=0x402;}
 uint32 CardRead(unsigned a,unsigned){assert(!finalDeleteFailed && !finalPciFailed);finalPciReads++;
  if(finalPciFailure && finalPciWrites==finalPciFailure)finalPciFailed=true;return finalCard[a/4];}
 uint32 RootRead(unsigned a,unsigned){assert(!finalDeleteFailed && !finalPciFailed);finalPciReads++;
  if(finalPciFailure && finalPciWrites==finalPciFailure)finalPciFailed=true;return finalRoot[a/4];}
 void CardWrite(unsigned a,unsigned,uint32 v){assert(!finalDeleteFailed && !finalPciFailed);finalPciWrites++;finalCard[a/4]=finalPciWrites==finalPciFailure?v^1:v;}
 void RootWrite(unsigned a,unsigned,uint32 v){assert(!finalDeleteFailed && !finalPciFailed);finalPciWrites++;finalRoot[a/4]=finalPciWrites==finalPciFailure?v^1:v;}
};
