/* SPDX-License-Identifier: MIT */
// Instantiate the service/session/host ownership path against the Haiku SDK.
// The policy has declarations only; no driver links or invokes this probe.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentHostPci.h"
#include "../PioneerQwen35DataflowResidentOwnedRunner.h"
using namespace PioneerQwen35;
struct OwnedProbeRaw {
    bool SelectRuntimeCore(unsigned);bool DataflowPanelMappingStable();unsigned ResetRead();
    void Q38PanelWrite(unsigned,uint32_t);uint32_t Q38PanelRead(unsigned);void Pause();
};
struct OwnedProbeHostScope {
    bool ResidentSessionAllowed(OwnedProbeRaw&,const DataflowResidentCommand&,unsigned);
    bool ResidentRootAllowed(OwnedProbeRaw&,const DataflowResidentCommand&,unsigned);
    bool AuditResidentFleet(OwnedProbeRaw&,const DataflowResidentCommand&);
    void RetainResident();
};
using OwnedProbeHost=PioneerQwen35ResidentHostPci<OwnedProbeRaw,OwnedProbeHostScope>;
struct OwnedProbeHandle { DataflowResidentSessionRun<OwnedProbeHost> run; };
struct OwnedProbePolicy {
    OwnedProbeHandle* NewResidentRun();DataflowProjectionAuthority* NewResidentCandidate();
    void DeleteResidentRun(OwnedProbeHandle*);void DeleteResidentCandidate(DataflowProjectionAuthority*);
    bool ResidentSessionSafe(const DataflowResidentGraphSession&,const DataflowStreamPlan&,unsigned);
    bool ResidentCommitAllowed(const DataflowResidentGraphSession&,const DataflowResidentSessionRun<OwnedProbeHost>&);
    void RetainResidentSession(const DataflowResidentGraphSession&,const void*);
    void RetainResidentOwnedRun(const DataflowResidentGraphSession&,OwnedProbeHandle&,bool);
    bool ResidentFleetRestored(const DataflowResidentGraphSession&,const DataflowServiceOwner&,
        const OwnedProbeHandle&,DataflowProjectionAuthority*,DataflowProjectionAuthority*,DataflowProjectionAuthority*);
    void Pause();
};
DataflowResidentSessionResult PioneerQ35ResidentOwnedRunProbe(DataflowResidentOwnedRunner<OwnedProbeHandle>& runner,
    DataflowResidentGraphSession& session,DataflowServiceOwner& service,DataflowProjectionAuthority*& live,
    unsigned position,unsigned token,bool decode,unsigned polls,DataflowProjectionProgram& scratch,
    OwnedProbePolicy& policy,DataflowArgmaxResult& output,unsigned& stage) {
    return runner.Run(session,service,live,position,token,decode,polls,scratch,policy,output,stage);
}

bool PioneerQ35ResidentOwnedRecoveryProbe(DataflowResidentOwnedRunner<OwnedProbeHandle>& runner,
    DataflowResidentGraphSession& session,DataflowServiceOwner& service,DataflowProjectionAuthority*& live,
    OwnedProbePolicy& policy) {
    return runner.ReleaseRetainedAfterRestoration(session,service,live,policy);
}
