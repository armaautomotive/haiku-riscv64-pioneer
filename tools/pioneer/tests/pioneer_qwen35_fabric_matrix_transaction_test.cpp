/* SPDX-License-Identifier: MIT */
// Exercise the real transaction composition with simulated IO and guards.
#include <cassert>
#include <cstdint>
#include <cstdio>
using uint64=uint64_t;
static unsigned checks=0,failCheck=0;
static int Validate(uint64 ticket) { return ticket==19 && ++checks!=failCheck?0:-1; }
struct Guard { int (*validate)(uint64)=Validate; };
struct Session;struct Context;
struct Receipt {
    bool sealed=false;
    template<class Scope> bool Matches(Context&,uint64 ticket,Guard* guard,Scope& scope) const {
        return sealed && guard->validate(ticket)==0 && scope.receiptOwned && guard->validate(ticket)==0;
    }
};
struct Qualification { bool bad=false,fail=false;unsigned calls=0; };
struct Context {
    Session* recoveryFreshSession=nullptr;
    Receipt recoveryFabricMatrixReceipt;
    template<class Callback>
    bool RunDataflowQualificationTransactionWithReceipt(Qualification& q,Callback& receipt,
        uint32_t& stage,uint32_t& routeStage,uint32_t& routeCore,bool& output,bool& cleaned) {
        ++q.calls;stage=12;routeStage=2;routeCore=119;output=!q.bad;cleaned=false;
        if (q.fail) return false;
        stage=13;
        if (output && !receipt()) return false;
        cleaned=true;stage=14;return output;
    }
};
struct Recovery {
    bool admission=true,access=true,complete=true,receiptOwned=true,sealAllowed=true;
    bool FabricMatrixRunAccessAllowed(Session&,Context&,Context&,uint64) { return access; }
    bool FabricMatrixRunAdmission(Session&,Context&,Context&,uint64) { return admission; }
    bool FabricMatrixRunComplete(Session&,Context&,Context&,uint64) { return complete; }
};
struct Session {
    enum Stage { RoleVerified,Retained };
    Context* old=nullptr;Context* fresh=nullptr;uint64 ticket=19;Stage stage=RoleVerified;
    Context* Fresh() const { return fresh; }
    uint64 Ticket() const { return ticket; }
    Stage CurrentStage() const { return stage; }
    void RetainUncertain(Recovery&) { stage=Retained; }
    bool RecordFabricMatrixQualification(Guard*,Recovery& scope) {
        if (!scope.sealAllowed) return false;
        old->recoveryFabricMatrixReceipt.sealed=true;return true;
    }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricMatrixQualification.h"
int main() {
    unsigned totalChecks=0;
    for (unsigned failure=0;;++failure) {
        if (failure && failure>totalChecks) break;
        checks=0;failCheck=failure;Context old,fresh;Session session{&old,&fresh};old.recoveryFreshSession=&session;
        Recovery recovery;Qualification q;Guard guard;uint32_t stage,routeStage,routeCore;bool output,cleaned;
        const bool okay=PioneerQ35RunFabricMatrixQualification(session,old,fresh,&guard,recovery,q,
            stage,routeStage,routeCore,output,cleaned);
        if (!failure) {
            assert(okay && output && cleaned && stage==14 && old.recoveryFabricMatrixReceipt.sealed);
            totalChecks=checks;assert(totalChecks>=12);
        } else assert(!okay && session.stage==Session::Retained);
    }
    for (unsigned failure=0;failure<6;++failure) {
        checks=failCheck=0;Context old,fresh;Session session{&old,&fresh};old.recoveryFreshSession=&session;
        Recovery recovery;Qualification q;Guard guard;uint32_t stage,rs,rc;bool output,cleaned;
        if (failure==0) recovery.admission=false;
        if (failure==1) recovery.access=false;
        if (failure==2) recovery.complete=false;
        if (failure==3) recovery.receiptOwned=false;
        if (failure==4) recovery.sealAllowed=false;
        if (failure==5) q.bad=true;
        assert(!PioneerQ35RunFabricMatrixQualification(session,old,fresh,&guard,recovery,q,
            stage,rs,rc,output,cleaned) && session.stage==Session::Retained);
        if (failure==5) assert(!old.recoveryFabricMatrixReceipt.sealed && cleaned && !output);
    }
    std::printf("PASS matrix transaction and %u ticket-loss boundaries; SIMULATED ONLY\n",totalChecks);
}
