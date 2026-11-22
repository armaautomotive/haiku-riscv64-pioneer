/* SPDX-License-Identifier: MIT */
// Compile-only bridge for the actual Haiku session, lock and mapper types.
// The existing control path can borrow its lock in this non-loadable witness;
// no writable mapping operation is registered and no accelerator is touched.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Role scope witness must only be compiled into the non-loadable graph/recovery driver
#endif
#include "PioneerQwen35DataflowRoleHaikuMap.h"
#include "PioneerQwen35DataflowRoleRouteGuard.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleReadOnlyViews.h"
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleWritableViews.h"
#include "PioneerQwen35DataflowRoleFleetPciIO.h"

static status_t Qwen35DataflowControl(PioneerProbeCookie*,uint32_t,void*,size_t);
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
static status_t Qwen35DataflowGenerate(PioneerProbeCookie*,void*,size_t);
#endif
// Borrow, never reacquire, the lock already held by Qwen35DataflowControl.
// The private constructor limits creation to that control path. The caller
// must detach before releasing sLock; no memory or control writes are yet
// authorized by this witness.
class PioneerQwen35RoleBorrowedScopeWitness {
    friend status_t Qwen35DataflowControl(PioneerProbeCookie*,uint32_t,void*,size_t);
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    friend status_t Qwen35DataflowGenerate(PioneerProbeCookie*,void*,size_t);
#endif
    PioneerProbeCookie* fCookie;
    PioneerStoreContext* fContext;
    thread_id fThread=-1;
    team_id fTeam=-1;
    uint64 fGeneration=0;
    bool fAttached=false,fAttempted=false;
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    const void* fFaultOwner=NULL;
    const void* fFaultRun=NULL;
    bool fFaultAttempted=false,fFaultClose=false;
#endif
    PioneerQwen35RoleBorrowedScopeWitness(PioneerProbeCookie* cookie,PioneerStoreContext* context)
        :fCookie(cookie),fContext(context) {}
public:
    PioneerQwen35RoleBorrowedScopeWitness(const PioneerQwen35RoleBorrowedScopeWitness&)=delete;
    PioneerQwen35RoleBorrowedScopeWitness& operator=(const PioneerQwen35RoleBorrowedScopeWitness&)=delete;
    ~PioneerQwen35RoleBorrowedScopeWitness() {
        if (fAttached) { (void)Detach(); sColdRecoveryRequired=true; }
    }
    bool Attach() {
        if (fAttempted) return false;
        fAttempted=true;
        if (fAttached || !fCookie || !fContext || fContext->io.qwen35BorrowedRoleScope
            || fContext->io.qwen35DataflowContext!=fContext
            || fCookie->magic!=0x50313030 || fCookie->context!=fContext
            || sRuntimeOwner!=fCookie || sColdRecoveryRequired || sReloadQuiesced
            || !sReloadGeneration || !sBankStorageOnly || !fContext->bankStorageOnly
            || fContext->bytes!=PioneerResidentBanks::Total) return false;
        const thread_id current=find_thread(NULL); thread_info info={};
        if (current<0 || fCookie->team<0 || get_thread_info(current,&info)!=B_OK
            || info.team!=fCookie->team) return false;
        fThread=current; fTeam=info.team; fGeneration=sReloadGeneration;
        fContext->io.qwen35BorrowedRoleScope=this; fAttached=true; return true;
    }
    bool ProveOwnerIdentity(PioneerSessionIO& io) const {
        if (!fAttached || !fCookie || !fContext || &io!=&fContext->io
            || io.qwen35BorrowedRoleScope!=this || io.qwen35DataflowContext!=fContext
            || fThread<0 || find_thread(NULL)!=fThread || fTeam<0 || fCookie->team!=fTeam
            || fCookie->magic!=0x50313030 || fCookie->context!=fContext
            || sRuntimeOwner!=fCookie || sColdRecoveryRequired || sReloadQuiesced
            || !fGeneration || sReloadGeneration!=fGeneration || !sBankStorageOnly
            || !fContext->bankStorageOnly || fContext->bytes!=PioneerResidentBanks::Total) return false;
        thread_info info={};
        return get_thread_info(fThread,&info)==B_OK && info.team==fTeam;
    }
    bool ProveExclusiveRewrite(PioneerSessionIO& io) const {
        if (!ProveOwnerIdentity(io)) return false;
        if (!PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner)) {
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            return !fFaultOwner;
#else
            return true;
#endif
        }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
        return fFaultOwner && fFaultOwner==fContext->qwen35GraphOwner && fFaultRun
            && PioneerQ35GraphOwnerOwnsRetainedRun(fContext->qwen35GraphOwner,fFaultRun);
#else
        return false;
#endif
    }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    bool BeginRetainedFault(PioneerSessionIO& io,const void* owner,const void* run) {
        if (fFaultAttempted) return false;
        fFaultAttempted=true;
        if (!ProveOwnerIdentity(io) || !owner || owner!=fContext->qwen35GraphOwner || !run
            || !PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner)
            || !PioneerQ35GraphOwnerOwnsRetainedRun(fContext->qwen35GraphOwner,run)) return false;
        fFaultOwner=owner;fFaultRun=run;return true;
    }
    bool PromoteRetainedFaultClose(PioneerSessionIO& io) {
        if (fFaultClose || !ProveExclusiveRewrite(io) || !fFaultOwner || !fFaultRun
            || !PioneerQ35GraphOwnerFaultClosePermitted(fContext->qwen35GraphOwner,fFaultRun)) return false;
        fFaultClose=true;return true;
    }
    bool FaultCloseAuthorized(PioneerSessionIO& io) const {
        return fFaultClose && ProveExclusiveRewrite(io) && fFaultOwner && fFaultRun
            && PioneerQ35GraphOwnerFaultClosePermitted(fContext->qwen35GraphOwner,fFaultRun);
    }
    bool EndRetainedFault(PioneerSessionIO& io,const void* owner,const void* run) {
        if (!ProveOwnerIdentity(io) || !fFaultOwner || fFaultOwner!=owner || fFaultRun!=run
            || owner!=fContext->qwen35GraphOwner) return false;
        fFaultOwner=NULL;fFaultRun=NULL;fFaultClose=false;return true;
    }
#endif
    bool ProveRoleMemoryReadSafe(PioneerSessionIO&,unsigned,unsigned) const { return false; }
    bool ProveRoleMemoryWriteSafe(PioneerSessionIO&,unsigned,unsigned,unsigned,unsigned) const { return false; }
    bool ProveRoleControlWriteSafe(PioneerSessionIO&,unsigned,uint32_t,uint32_t) const { return false; }
    bool ProveRoleFleetHeldAndQuiet(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleDefaultBootRouting(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleLocalRamStable(PioneerSessionIO&,unsigned) const { return false; }
    // Must additionally establish partial-fleet cleanup and a role-aware
    // stop/hold path before permitting any physical reset release.
    bool ProveRoleReleaseSafe(PioneerSessionIO&,unsigned) const { return false; }
    // A stop request requires the retained graph and compute/queue writers
    // to be settled; fault and clean stops need different proofs.
    bool ProveRoleStopSafe(PioneerSessionIO&,unsigned,bool) const { return false; }
    bool ProveRolePostStopSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveComputeStopSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveComputePostStopSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleHoldSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleHeldAfterAttempt(PioneerSessionIO&,unsigned) const { return false; }
    // Must independently establish the owned role/compute images, routes,
    // NoC quiescence and session/graph state; mailbox/PC samples alone are
    // insufficient. This proof is not yet implemented.
    bool ProveRoleRuntimeSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool Detach() {
        if (!fAttached || !fContext) return false;
        const bool valid=find_thread(NULL)==fThread && fContext->io.qwen35BorrowedRoleScope==this
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            && !fFaultOwner
#endif
            ;
        if (fContext->io.qwen35BorrowedRoleScope==this) fContext->io.qwen35BorrowedRoleScope=NULL;
        fAttached=false;
        if (!valid) sColdRecoveryRequired=true;
        return valid;
    }
};
#endif

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
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
    // No held-fleet, retained-snapshot or RAM-window proof is connected yet.
    // Mapping and guard types compile, but every actual write is denied.
    bool ProveRoleMemoryWriteSafe(PioneerSessionIO&,unsigned,unsigned,unsigned,unsigned) const { return false; }
    bool ProveRoleControlWriteSafe(PioneerSessionIO&,unsigned,uint32_t,uint32_t) const { return false; }
#endif
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
    // Selected tile only. This samples known status registers twice; it does
    // not prove fleet-wide quiescence, completion of the local-RAM reset
    // window, or absence of incoming NoC traffic, and grants no RAM access.
    bool AuditHeldAndQuiet(unsigned core) {
        using namespace PioneerQwen35;
        for (unsigned pass=0;pass<2;pass++) {
            uint32_t reset=0;
            if (!Read(core,DataflowRolePhysicalControl,0x1b0,reset) || reset!=0x47800)
                return false;
            for (unsigned noc=0;noc<2;noc++) {
                const unsigned window=noc?DataflowRolePhysicalNoc1:DataflowRolePhysicalNoc0;
                uint32_t raw=0,route=0;
                if (!Read(core,window,DataflowNocLogicalIdOffset,raw)
                    || !DataflowNocDecodeLocalRoute(raw,route)
                    || !DataflowNocRouteMatchesWorker(route,core)) return false;
                for (unsigned initiator=0;initiator<4;initiator++) {
                    uint32_t status=0;
                    if (!Read(core,window,0x40+initiator*0x800,status) || status)
                        return false;
                }
                for (unsigned id=0;id<32;id++) {
                    uint32_t status=0;
                    if (!Read(core,window,0x240+id*4,status) || (status&0xffU))
                        return false;
                }
            }
            if (!Read(core,DataflowRolePhysicalControl,0x1b0,reset) || reset!=0x47800)
                return false;
        }
        return true;
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
    if (action==3) return owner.AuditHeldAndQuiet(core);
    return false;
}
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
// Separate persistent owner because a failed unmap must retain the lock and
// area IDs for retry. There is no ioctl/caller and the scope denies writes.
class PioneerQwen35RoleHaikuWritableViewsWitness {
    PioneerQwen35RoleHaikuScopeWitness fScope;
    PioneerQwen35RoleHaikuMap fPlatform;
    PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> fViews;
public:
    PioneerQwen35RoleHaikuWritableViewsWitness():fViews(fPlatform) {}
    bool Open(PioneerProbeCookie* cookie) {
        if (!fScope.Open(cookie)) return false;
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.Open(io->profile.bar0_host_start,io->profile.bar0_host_end,guard);
    }
    bool Write(unsigned core,unsigned which,unsigned offset,const uint32_t* data,unsigned bytes) {
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.WriteWords(core,which,offset,data,bytes,guard);
    }
    bool WriteResetDisable(unsigned core,uint32_t expected,uint32_t desired) {
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.WriteResetDisable(core,expected,desired,guard);
    }
    bool Close() {
        if (!fViews.Close()) return false;
        return fScope.Close();
    }
};
__attribute__((used)) static bool
PioneerQwen35RoleHaikuWritableViewsCompileWitness(PioneerQwen35RoleHaikuWritableViewsWitness& owner,
    PioneerProbeCookie* cookie,unsigned action,unsigned core,unsigned which,unsigned offset,
    const uint32_t* data,unsigned bytes,uint32_t expected,uint32_t desired)
{
    if (action==0) return owner.Open(cookie);
    if (action==1) return owner.Write(core,which,offset,data,bytes);
    if (action==2) return owner.WriteResetDisable(core,expected,desired);
    if (action==3) return owner.Close();
    return false;
}
// Type-check the real recovery owner against the borrowed session, selected
// routes and fixed Haiku mappings. The scope denies all fleet/memory proofs,
// so this static witness cannot capture, write or release anything.
__attribute__((used)) static bool
PioneerQwen35RoleFleetPciCompileWitness(PioneerSessionIO& io,
    PioneerQwen35RoleBorrowedScopeWitness& scope,
    PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>& views,
    PioneerQwen35::DataflowRoleFleetRecoveryOwner& owner,uint32_t* codeStorage,
    uint32_t* ramStorage,const uint32_t* image,unsigned words,unsigned action)
{
    using namespace PioneerQwen35;
    PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness,
        DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>> adapter(io,scope,views);
    if (action==0) return adapter.AllRolesHeld(0);
    if (action==1) return owner.CaptureAll(codeStorage,DataflowRoleFleetRecoveryOwner::CodeWords(),
        ramStorage,DataflowRoleFleetRecoveryOwner::RamWords(),adapter);
    if (action==2) return owner.StageImages(image,words,image,words,image,words,image,words,adapter);
    if (action==3) return owner.PreparePossibleRelease(adapter) && owner.BeginReleaseAttempt(adapter);
    if (action==4) return owner.RestoreAll(adapter) && owner.VerifyAfterResidentRestore(adapter);
    return false;
}
#endif
