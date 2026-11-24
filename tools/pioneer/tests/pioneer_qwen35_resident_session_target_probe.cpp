/* SPDX-License-Identifier: MIT */
// Type-check the real session and host transport against the Haiku SDK.
// Declarations only: not linked, loaded, or authorized for device use.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentHostPci.h"
#include "../PioneerQwen35DataflowResidentGraphSession.h"
using namespace PioneerQwen35;
struct SessionProbeRaw {
    bool SelectRuntimeCore(unsigned);bool DataflowPanelMappingStable();unsigned ResetRead();
    void Q38PanelWrite(unsigned,uint32_t);uint32_t Q38PanelRead(unsigned);void Pause();
};
struct SessionProbeHostScope {
    bool ResidentSessionAllowed(SessionProbeRaw&,const DataflowResidentCommand&,unsigned);
    bool ResidentRootAllowed(SessionProbeRaw&,const DataflowResidentCommand&,unsigned);
    bool AuditResidentFleet(SessionProbeRaw&,const DataflowResidentCommand&);
    void RetainResident();
};
using SessionProbeHost=PioneerQwen35ResidentHostPci<SessionProbeRaw,SessionProbeHostScope>;
struct SessionProbePolicy {
    bool ResidentBindAllowed(const DataflowResidentGraphSession&,const DataflowStreamPlan&,unsigned);
    bool ReserveResidentSession(const DataflowResidentGraphSession&,const DataflowStreamPlan&,unsigned);
    bool ResidentSessionSafe(const DataflowResidentGraphSession&,const DataflowStreamPlan&,unsigned);
    void RetainResidentSession(const DataflowResidentGraphSession&,const void*);
    void Pause();
};
bool PioneerQ35ResidentSessionBindProbe(DataflowResidentGraphSession& session,const DataflowStreamPlan& plan,
    const unsigned (&routes)[2][Banks],unsigned slot,DataflowProjectionAuthority*& authority,SessionProbePolicy& policy) {
    return session.Bind(plan,routes,slot,authority,policy);
}
DataflowResidentSessionResult PioneerQ35ResidentSessionRunProbe(DataflowResidentGraphSession& session,
    DataflowResidentSessionRun<SessionProbeHost>& run,DataflowProjectionAuthority& candidate,
    unsigned position,unsigned token,bool decode,unsigned polls,DataflowProjectionProgram& scratch,
    SessionProbePolicy& policy,DataflowArgmaxResult& output,DataflowProjectionAuthority*& retired,unsigned& stage) {
    return session.Run(run,candidate,position,token,decode,polls,scratch,policy,output,retired,stage);
}
