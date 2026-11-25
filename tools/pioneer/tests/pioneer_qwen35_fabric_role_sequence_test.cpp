/* SPDX-License-Identifier: MIT */
// The actual intermediate sequence with simulated owners, guard and scope.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
constexpr int B_OK=0;
using uint64=uint64_t;
static unsigned step=0,failStep=0,checks=0,failCheck=0;
static std::vector<unsigned> operations;
static bool Step(unsigned value) { operations.push_back(value);return ++step!=failStep; }
static int Validate(uint64 ticket) { return ticket==7 && ++checks!=failCheck?B_OK:-1; }
struct Guard { int (*validate)(uint64)=Validate; };
struct IO { bool protectedArea=false;bool ReferencesRecoveryHostArea(int32_t) const { return protectedArea; } };
struct Context;
struct Scope;
struct Session {
    Context* fresh=nullptr;bool retained=false,ready=true;uint64 ticket=7;
    Context* Fresh() const { return fresh; }
    bool RoleReady() const { return ready && !retained; }
    uint64 Ticket() const { return ticket; }
    void RetainUncertain(Scope&) { retained=true; }
    bool VerifyRoleRestoration(Guard*,Scope&) { return Step(7); }
    bool CloseRoleTransport(Guard*,Scope&) { return Step(9); }
    bool DisposeRoleOldHostBuffers(Guard*,Scope&) { return Step(10); }
    bool PublishRoleMappingsDisposed(Guard*,Scope&) { return Step(11); }
    bool RetireRoleGraph(Guard*,Scope&) { return Step(12); }
    bool ReleaseRoleFreshHostPage(Guard*,Scope&) { return Step(13); }
    bool RecordRoleOwnerRetirement(Guard*,Scope&) { return Step(14); }
};
struct RoleScope {};struct ParentScope {};struct Bar {};
struct Role {
    template<class Check,class Protected> bool OpenFabricRecoveryViews(Bar&,Check& check,Protected& protectedArea) {
        return check() && !protectedArea(10) && check() && Step(1);
    }
    template<class Check> bool BeginFabricCoreRecovery(IO&,RoleScope&,Check& check) { return check() && Step(2); }
    template<class Check> bool RestoreFabricCoreFleet(IO&,RoleScope&,Check& check,unsigned polls) {
        return polls && check() && Step(3);
    }
    template<class Check> bool VerifyFabricRolesAfterParent(IO&,RoleScope&,Check& check) { return check() && Step(5); }
    template<class Check,class Protected> bool CloseFabricRecoveryViews(Check& check,Protected& protectedArea) {
        return check() && !protectedArea(10) && check() && Step(6);
    }
};
struct Graph {
    template<class Check> bool RestoreFabricParent(Context&,ParentScope&,Check& check) { return check() && Step(4); }
};
struct Context { Session* recoveryFreshSession=nullptr;Role* qwen35SharedRoleBinding=nullptr;
    Graph* qwen35GraphOwner=nullptr;IO io; };
struct Scope {
    bool admission=true,access=true,protect=false,replaceGraph=false;
    Graph other;
    bool FabricRoleSequenceAdmission(Session&,Context&,Context&,uint64) const { return admission; }
    bool FabricRoleSequenceAccessAllowed(Session&,Context& old,Context&,uint64) {
        if (replaceGraph) old.qwen35GraphOwner=&other;
        return access;
    }
    bool FabricRoleSequenceAreaProtected(Session&,Context&,Context&,uint64,int32_t) const { return protect; }
    bool FabricRoleQualifyMatrix(Session&,Context&,Context&,Guard*,uint64) { return Step(8); }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRoleRecoverySequence.h"
static void Reset() { step=failStep=checks=failCheck=0;operations.clear(); }
int main() {
    unsigned guardChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>guardChecks) break;
        Reset();Context old,fresh;Role role;Graph graph;Session session;Guard guard;Scope scope;RoleScope rs;ParentScope ps;Bar bar;
        session.fresh=&fresh;old.recoveryFreshSession=&session;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        failCheck=failure;
        const bool okay=PioneerQ35RestoreFabricRoleSequence(session,old,&guard,scope,rs,ps,bar,5);
        if (!failure) {
            assert(okay && !session.retained && operations==std::vector<unsigned>({1,2,3,4,5,6,7,8,9,10,11,12,13,14}));
            guardChecks=checks;assert(guardChecks>24);
        } else assert(!okay && session.retained);
    }
    for (unsigned failure=1;failure<=14;failure++) {
        Reset();Context old,fresh;Role role;Graph graph;Session session;Guard guard;Scope scope;RoleScope rs;ParentScope ps;Bar bar;
        session.fresh=&fresh;old.recoveryFreshSession=&session;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        failStep=failure;
        assert(!PioneerQ35RestoreFabricRoleSequence(session,old,&guard,scope,rs,ps,bar,5)
            && session.retained && operations.size()==failure && operations.back()==failure);
        const auto count=operations.size();
        assert(!PioneerQ35RestoreFabricRoleSequence(session,old,&guard,scope,rs,ps,bar,5) && operations.size()==count);
    }
    for (unsigned denial=0;denial<5;denial++) {
        Reset();Context old,fresh;Role role;Graph graph;Session session;Guard guard;Scope scope;RoleScope rs;ParentScope ps;Bar bar;
        session.fresh=&fresh;old.recoveryFreshSession=&session;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        if (denial==0) scope.admission=false;
        if (denial==1) scope.access=false;
        if (denial==2) scope.protect=true;
        if (denial==3) fresh.io.protectedArea=true;
        if (denial==4) scope.replaceGraph=true;
        assert(!PioneerQ35RestoreFabricRoleSequence(session,old,&guard,scope,rs,ps,bar,5)
            && session.retained && operations.empty());
    }
    std::printf("PASS ordered role/parent recovery sequence: all steps and %u simulated ticket-loss checks; NO HARDWARE\n",guardChecks);
}
