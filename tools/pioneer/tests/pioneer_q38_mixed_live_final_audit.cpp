/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_mixed_live_final_audit.h"
#include <cassert>
namespace {
using uint32=uint32_t;
static void dprintf(const char*,...){} // diagnostic text only
struct PioneerMmioIO {
 const MixedFinalAuditEndpoints& endpoints;MixedFinalAuditResults& results;
 PioneerMmioIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):endpoints(e),results(r){}
 uint32 Raw(MixedFinalAuditEndpoints::Kind kind,unsigned a=0,unsigned b=0,unsigned c=0){return endpoints.read(endpoints.object,kind,a,b,c);}
};
struct PioneerArcIO:PioneerMmioIO {
 tt_arc_scratch_result& arc;bool& dirty;
 PioneerArcIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):PioneerMmioIO(e,r),arc(r.arc),dirty(r.dirty){}
 uint32 TlbRead(unsigned i){return Raw(MixedFinalAuditEndpoints::Tlb,1,i);}
 void TlbWrite(unsigned i,uint32 value){dirty=true;endpoints.writeTlb(endpoints.object,1,i,value);}
 uint32 ScratchRead(unsigned n){return Raw(MixedFinalAuditEndpoints::Scratch,n);}
#include "native_audit_Arc.inc"
};
struct PioneerTelemetryIO:PioneerArcIO {
 tt_arc_telemetry_result& telemetry;bool& telemetryAttempted;const bool operating=false;
 PioneerTelemetryIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):PioneerArcIO(e,r),telemetry(r.telemetry),telemetryAttempted(r.telemetryAttempted){}
 uint32 CsmRead(uint32 a){return Raw(MixedFinalAuditEndpoints::Csm,a);}
#include "native_audit_Telemetry.inc"
};
struct PioneerTensixIO:PioneerTelemetryIO {
 tt_tensix_reset_result& tensix;bool& resetAttempted;
 PioneerTensixIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):PioneerTelemetryIO(e,r),tensix(r.tensix),resetAttempted(r.resetAttempted){}
 uint32 NiuRead(){return Raw(MixedFinalAuditEndpoints::Niu);}
 uint32 ResetRead(){return Raw(MixedFinalAuditEndpoints::Reset);}
#include "native_audit_Tensix.inc"
};
struct PioneerDramSamplesIO:PioneerTensixIO {
 tt_dram_samples_result& dram;bool& dramAttempted;const bool writeSamples=false;
 PioneerDramSamplesIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):PioneerTensixIO(e,r),dram(r.dram),dramAttempted(r.dramAttempted){}
 uint32 SourceRead(unsigned w){return Raw(MixedFinalAuditEndpoints::Source,w);}
 uint32 DramIdRead(unsigned w){return Raw(MixedFinalAuditEndpoints::DramId,w);}
 uint32 DramTlbRead(unsigned s,unsigned w){return Raw(MixedFinalAuditEndpoints::Tlb,s,w);}
 void DramTlbWrite(unsigned s,unsigned w,uint32 value){endpoints.writeTlb(endpoints.object,s,w,value);}
 uint32 DramRead(unsigned){assert(false && "NO diagnostic payload access");return 0;}
 void DramWrite(unsigned,uint32){assert(false && "NO diagnostic payload access");}
#include "native_audit_DramSamples.inc"
};
struct PioneerComputeNocIO:PioneerDramSamplesIO {
 tt_compute_noc_result& noc;bool& nocAttempted;const unsigned targetY=2;
 PioneerComputeNocIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):PioneerDramSamplesIO(e,r),noc(r.noc),nocAttempted(r.nocAttempted){}
 uint32 ComputeNocRead(uint32 a){return Raw(MixedFinalAuditEndpoints::Compute,a);}
#include "native_audit_ComputeNoc.inc"
};
struct PioneerHostPathIO:PioneerComputeNocIO {
 tt_host_path_result& path;bool& pathAttempted;
 PioneerHostPathIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):PioneerComputeNocIO(e,r),path(r.path),pathAttempted(r.pathAttempted){}
 uint32 HostInboundRead(unsigned w){return Raw(MixedFinalAuditEndpoints::Inbound,w);}
 uint32 HostSiiRead(unsigned w){return Raw(MixedFinalAuditEndpoints::Sii,w);}
#include "native_audit_HostPath.inc"
};
struct PioneerHostDmaAuditIO:PioneerHostPathIO {
 tt_host_dma_audit_result& audit;bool& auditAttempted;
 PioneerHostDmaAuditIO(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r):PioneerHostPathIO(e,r),audit(r.audit),auditAttempted(r.auditAttempted){}
 uint32 HostPciRead(unsigned p,uint16_t a,uint8_t n){return Raw(MixedFinalAuditEndpoints::Pci,p,a,n);}
 uint32 HostHdmaRead(unsigned w){return Raw(MixedFinalAuditEndpoints::Hdma,w);}
#include "native_audit_HostDmaAudit.inc"
};
} // no ODR collision with the native fixture's simulated virtual-map layout
bool ExecuteMixedFinalAudit(const MixedFinalAuditEndpoints& e,MixedFinalAuditResults& r,uint32_t id){
 assert(e.object && e.read && e.writeTlb);PioneerHostDmaAuditIO audit(e,r);return audit.AfterNodeId(id);
}
