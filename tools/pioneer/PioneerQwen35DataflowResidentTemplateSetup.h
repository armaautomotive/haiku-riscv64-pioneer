/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_TEMPLATE_SETUP_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_TEMPLATE_SETUP_H
#include "PioneerQwen35DataflowResidentTemplateInstaller.h"
namespace PioneerQwen35 {
// Service-install hook for the exact recorded stream and held-fleet scope.
// The referenced plan/scope must outlive installation and any retained fault.
// Scope supplies ownership and physical admission; this wrapper grants none.
template<class Scope> class DataflowResidentTemplateSetup {
    const DataflowStreamPlan& plan;
    DataflowDynamicTokenContext seed;
    unsigned routes[2][Banks]{},slot,polls;
    Scope& scope;
    DataflowResidentTemplateInstaller installer;
    template<class IO> struct Adapter {
        IO& io;Scope& scope;
        bool ResidentTemplateOwnerAllowed(const DataflowStreamPlan& p,const DataflowDynamicTokenContext& s,unsigned slot) {
            return scope.ResidentTemplateOwnerAllowed(io,p,s,slot);
        }
        bool ResidentTemplateFleetHeld() { return scope.ResidentTemplateFleetHeld(io); }
        bool ResidentTemplateCoreHeld(unsigned core) { return scope.ResidentTemplateCoreHeld(io,core); }
        void RetainResidentTemplates() { scope.RetainResidentTemplates(); }
        bool WriteCoreMemory(unsigned core,unsigned address,const void* data,unsigned bytes) {
            return scope.ResidentTemplateAccessAllowed(io,core,true)
                && io.WriteCoreMemory(core,address,data,bytes)
                && scope.ResidentTemplateAccessAllowed(io,core,true);
        }
        bool ReadCoreMemory(unsigned core,unsigned address,void* data,unsigned bytes) {
            return scope.ResidentTemplateAccessAllowed(io,core,false)
                && io.ReadCoreMemory(core,address,data,bytes)
                && scope.ResidentTemplateAccessAllowed(io,core,false);
        }
    };
public:
    DataflowResidentTemplateSetup(const DataflowStreamPlan& p,const unsigned (&bankRoutes)[2][Banks],
        const DataflowDynamicTokenContext& token,unsigned inputSlot,unsigned pollLimit,Scope& owner)
        :plan(p),seed(token),slot(inputSlot),polls(pollLimit),scope(owner) {
        for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=bankRoutes[n][b];
    }
    DataflowResidentTemplateSetup(const DataflowResidentTemplateSetup&)=delete;
    DataflowResidentTemplateSetup& operator=(const DataflowResidentTemplateSetup&)=delete;
    template<class IO> bool Stage(IO& io) {
        Adapter<IO> adapter{io,scope};
        return installer.Install(plan,routes,seed,slot,polls,adapter)==DataflowResidentTemplateInstalled;
    }
    template<class IO> bool Verify(IO& io) {
        Adapter<IO> adapter{io,scope};return installer.VerifyBeforeRelease(adapter);
    }
    bool Retained() const { return installer.IsRetained(); }
    unsigned FailedCore() const { return installer.FailedCore(); }
};
}
#endif
