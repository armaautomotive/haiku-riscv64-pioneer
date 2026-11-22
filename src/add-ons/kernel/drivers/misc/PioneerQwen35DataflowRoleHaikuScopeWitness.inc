/* SPDX-License-Identifier: MIT */
// Compile-only bridge for the actual Haiku session, lock and mapper types.
// There is NO registered ioctl or caller. This is included only in the
// non-loadable recovery witness; it is not an accelerator operation.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Role scope witness must only be compiled into the non-loadable graph/recovery driver
#endif
#include "PioneerQwen35DataflowRoleHaikuMap.h"
#include "PioneerQwen35DataflowRoleRouteGuard.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleReadOnlyViews.h"

class PioneerQwen35RoleHaikuScopeWitness {
    PioneerProbeCookie* fCookie=NULL;
    PioneerStoreContext* fContext=NULL;
    thread_id fThread=-1;
    team_id fTeam=-1;
    uint64 fGeneration=0;
    bool fHeld=false,fAttempted=false;
public:
    PioneerQwen35RoleHaikuScopeWitness()=default;
    PioneerQwen35RoleHaikuScopeWitness(const PioneerQwen35RoleHaikuScopeWitness&)=delete;
    PioneerQwen35RoleHaikuScopeWitness& operator=(const PioneerQwen35RoleHaikuScopeWitness&)=delete;
    bool Open(PioneerProbeCookie* cookie) {
        if (fAttempted || !cookie) return false;
        fAttempted=true;
        if (AcquireProbeLock(cookie,true)!=B_OK) return false;
        fHeld=true; fCookie=cookie; fContext=cookie->context;
        fThread=find_thread(NULL); fTeam=cookie->team; fGeneration=sReloadGeneration;
        thread_info info={};
        if (!fContext || fThread<0 || fTeam<0
            || get_thread_info(fThread,&info)!=B_OK || info.team!=fTeam
            || !ProveExclusiveRewrite(fContext->io)) {
            (void)ReleaseOnce(); return false;
        }
        return true;
    }
    PioneerSessionIO* IO() const { return fHeld && fContext?&fContext->io:NULL; }
    bool ProveExclusiveRewrite(PioneerSessionIO& io) const {
        return fHeld && fCookie && fContext && fIOEqual(io)
            && fThread>=0 && find_thread(NULL)==fThread && fTeam>=0 && fCookie->team==fTeam
            && fCookie->magic==0x50313030 && sRuntimeOwner==fCookie
            && fCookie->context==fContext && io.qwen35DataflowContext==fContext
            && !sColdRecoveryRequired && !sReloadQuiesced && fGeneration
            && sReloadGeneration==fGeneration && sBankStorageOnly && fContext->bankStorageOnly
            && fContext->bytes==PioneerResidentBanks::Total
            && fContext->qwen35DataflowService && fContext->qwen35DataflowService->Ready()
            && !PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner);
    }
    // No held/quiet/reset-window proof is wired yet. Thus even this compile
    // witness denies all boot, L1 and private-RAM reads through its guard.
    bool ProveRoleMemoryReadSafe(PioneerSessionIO&,unsigned,unsigned) const { return false; }
    bool Close() {
        if (!fHeld || fThread<0 || find_thread(NULL)!=fThread) return false;
        return ReleaseOnce();
    }
private:
    bool ReleaseOnce() {
        if (!fHeld) return false;
        // Release is attempted once; an ambiguous failure latches recovery.
        fHeld=false;
        const status_t status=release_sem(sLock);
        if (status!=B_OK) sColdRecoveryRequired=true;
        return status==B_OK;
    }
    bool fIOEqual(const PioneerSessionIO& io) const { return fContext && &fContext->io==&io; }
};

// Persistent ownership is intentional: on an unmap failure the caller keeps
// the object and the exclusive scope rather than dropping live area IDs.
class PioneerQwen35RoleHaikuViewsWitness {
    PioneerQwen35RoleHaikuScopeWitness fScope;
    PioneerQwen35RoleHaikuMap fPlatform;
    PioneerQwen35::DataflowRoleReadOnlyViews<PioneerQwen35RoleHaikuMap> fViews;
public:
    PioneerQwen35RoleHaikuViewsWitness():fViews(fPlatform) {}
    bool Open(PioneerProbeCookie* cookie) {
        if (!fScope.Open(cookie)) return false;
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.Open(io->profile.bar0_host_start,io->profile.bar0_host_end,guard);
    }
    bool Read(unsigned core,unsigned window,unsigned offset,uint32_t& output) {
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.Read(core,window,offset,output,guard);
    }
    bool Close() {
        if (!fViews.Close()) return false;
        return fScope.Close();
    }
};
// Force type-checking against the actual driver session without registering
// an operation or making the compile-only driver loadable.
__attribute__((used)) static bool
PioneerQwen35RoleHaikuViewsCompileWitness(PioneerQwen35RoleHaikuViewsWitness& owner,
    PioneerProbeCookie* cookie,unsigned action,unsigned core,unsigned window,unsigned offset,uint32_t& value)
{
    if (action==0) return owner.Open(cookie);
    if (action==1) return owner.Read(core,window,offset,value);
    if (action==2) return owner.Close();
    return false;
}
