/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_TEMPLATE_INSTALLER_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_TEMPLATE_INSTALLER_H
#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include "PioneerQwen35DataflowStreamPlan.h"
#include <string.h>
namespace PioneerQwen35 {
enum DataflowResidentTemplateResult {
    DataflowResidentTemplateInvalid=0,DataflowResidentTemplatePreflight=1,
    DataflowResidentTemplateRetained=2,DataflowResidentTemplateInstalled=3
};
// One-time initialization while all workers are held and their L1 snapshots
// remain owned. IO must bind the exact immutable sealed stream, model and
// session and restore any temporary routes; this class grants no authority.
// Partial writes and readback errors retain the fleet; no image is released.
class DataflowResidentTemplateInstaller {
    enum Phase { Empty,Installed,Retained } phase=Empty;
    unsigned failedCore=DataflowCoreCount;
    const DataflowStreamPlan* installedPlan=nullptr;
    DataflowDynamicTokenContext installedSeed{};
    unsigned installedRoutes[2][Banks]{};
    unsigned installedSlot=ActivationBuffers,installedPolls=0;
    template<class IO> DataflowResidentTemplateResult Fail(IO& io) {
        phase=Retained;io.RetainResidentTemplates();return DataflowResidentTemplateRetained;
    }
public:
    DataflowResidentTemplateInstaller()=default;
    DataflowResidentTemplateInstaller(const DataflowResidentTemplateInstaller&)=delete;
    DataflowResidentTemplateInstaller& operator=(const DataflowResidentTemplateInstaller&)=delete;
    unsigned FailedCore() const { return failedCore; }
    bool IsInstalled() const { return phase==Installed; }
    bool IsRetained() const { return phase==Retained; }
    template<class IO> DataflowResidentTemplateResult Install(const DataflowStreamPlan& plan,
        const unsigned (&routes)[2][Banks],const DataflowDynamicTokenContext& seed,
        unsigned slot,unsigned polls,IO& io) {
        if (phase!=Empty || !plan.ready || !plan.epoch || plan.count!=DataflowGraphDecodeSteps
            || !seed.Valid() || slot>=ActivationBuffers || !polls || polls>DataflowGraphRunMaxPolls
            || !DataflowBankRoutesValid(routes)) return DataflowResidentTemplateInvalid;
        const DataflowGraphRunTicket seedTicket{1,1,DataflowGraphDecodeSteps};
        if (!io.ResidentTemplateOwnerAllowed(plan,seed,slot) || !io.ResidentTemplateFleetHeld())
            return DataflowResidentTemplatePreflight;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            if (plan.binding[core].epoch!=plan.epoch || plan.binding[core].count!=plan.count
                || !DataflowGraphRunInputsValid(plan.binding[core],plan.lease[core],routes,seed,
                    seedTicket,1,polls,slot,core) || !io.ResidentTemplateCoreHeld(core)) {
                failedCore=core;return DataflowResidentTemplatePreflight;
            }
        }
        DataflowGraphRunDescriptor expected{},observed{};
        // Every destination is written, then immediately checked. Record the
        // first attempted write before invoking IO: a failed write may land.
        phase=Retained;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            failedCore=core;
            if (!io.ResidentTemplateOwnerAllowed(plan,seed,slot) || !io.ResidentTemplateCoreHeld(core)
                || !DataflowGraphRunBuild(plan.binding[core],plan.lease[core],routes,seed,
                    seedTicket,1,polls,slot,expected)
                || !io.WriteCoreMemory(core,DataflowGraphRunAddress,&expected,sizeof(expected))
                || !io.ReadCoreMemory(core,DataflowGraphRunAddress,&observed,sizeof(observed))
                || memcmp(&expected,&observed,sizeof(expected))) return Fail(io);
        }
        // Later writes must not have damaged an earlier core's descriptor.
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            failedCore=core;
            if (!io.ResidentTemplateOwnerAllowed(plan,seed,slot) || !io.ResidentTemplateCoreHeld(core)
                || !DataflowGraphRunBuild(plan.binding[core],plan.lease[core],routes,seed,
                    seedTicket,1,polls,slot,expected)
                || !io.ReadCoreMemory(core,DataflowGraphRunAddress,&observed,sizeof(observed))
                || memcmp(&expected,&observed,sizeof(expected))) return Fail(io);
        }
        if (!io.ResidentTemplateFleetHeld() || !io.ResidentTemplateOwnerAllowed(plan,seed,slot))
            return Fail(io);
        installedPlan=&plan;installedSeed=seed;installedSlot=slot;installedPolls=polls;
        for (unsigned n=0;n<2;n++) for (unsigned bank=0;bank<Banks;bank++)
            installedRoutes[n][bank]=routes[n][bank];
        failedCore=DataflowCoreCount;phase=Installed;return DataflowResidentTemplateInstalled;
    }
    // Recheck the same sealed owner and every descriptor immediately before
    // the service releases its first worker. This is a fresh bounded audit,
    // not proof that an independent firmware writer cannot act later.
    template<class IO> bool VerifyBeforeRelease(IO& io) {
        if (phase!=Installed || !installedPlan || !io.ResidentTemplateFleetHeld()
            || !io.ResidentTemplateOwnerAllowed(*installedPlan,installedSeed,installedSlot)) {
            if (phase==Installed) Fail(io);
            return false;
        }
        DataflowGraphRunDescriptor expected{},observed{};
        const DataflowGraphRunTicket seedTicket{1,1,DataflowGraphDecodeSteps};
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            failedCore=core;
            if (!io.ResidentTemplateOwnerAllowed(*installedPlan,installedSeed,installedSlot)
                || !io.ResidentTemplateCoreHeld(core)
                || !DataflowGraphRunBuild(installedPlan->binding[core],installedPlan->lease[core],
                    installedRoutes,installedSeed,seedTicket,1,installedPolls,installedSlot,expected)
                || !io.ReadCoreMemory(core,DataflowGraphRunAddress,&observed,sizeof(observed))
                || memcmp(&expected,&observed,sizeof(expected))) { Fail(io);return false; }
        }
        if (!io.ResidentTemplateFleetHeld()
            || !io.ResidentTemplateOwnerAllowed(*installedPlan,installedSeed,installedSlot)) {
            Fail(io);return false;
        }
        failedCore=DataflowCoreCount;return true;
    }
};
}
#endif
