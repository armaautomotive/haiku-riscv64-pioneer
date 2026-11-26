/* SPDX-License-Identifier: MIT */
// Actual flash PCI view and held recovery owner over separate fake memories.
#define PIONEER_Q35_FLASH_PCI_FIXTURE_ONLY 1
#define PIONEER_Q35_FLASH_RECOVERY_FIXTURE_ONLY 1
#include "pioneer_qwen35_flash_code_pci_view_test.cpp"
#include "pioneer_qwen35_flash_role_recovery_composition_test.cpp"
#include "../PioneerQwen35DataflowFlashRolePciScope.h"
struct CombinedGuard : Guard {
    bool common=true,roles=true,ram=true,images=true,restore=true;
    const void* expectedLedger=nullptr;
    unsigned routeChecks=0,failRouteAt=UINT32_MAX;
    bool ProveFlashRoleLedgerBound(const void* role,const void* flash,const void* ledger) const {
        return role && flash && ledger && ledger==expectedLedger;
    }
    bool ProveFlashRoleRouteSame(const void* role,const void* flash,unsigned core) {
        return role && flash && selected==core && routeChecks++!=failRouteAt;
    }
    bool ProveFlashRoleWriteDrained(const void* role,const void* flash) const {
        return role && flash && drained;
    }
    bool ProveFlashRoleCommonAuthority(const void* role,const void* flash,const void* owner,const void* extra) const {
        return common && role && flash && owner && extra;
    }
    bool ProveFlashRoleSnapshotsSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,bool) const {
        return roles && a && b && aw==DataflowRoleFleetImagesOwner::RequiredWords() && bw==DataflowFlashFleetSnapshotWords;
    }
    bool ProveFlashRoleRamSnapshotSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,bool) const {
        return ram && a && b && aw==DataflowRoleFleetRecoveryOwner::RamWords() && bw==DataflowFlashFleetSnapshotWords;
    }
    bool ProveFlashRoleImagesSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,
        const uint32_t* c,unsigned cw,const uint32_t* d,unsigned dw) const {
        return images && a && aw && b && bw && c && cw && d && dw;
    }
    bool ProveFlashRoleRestoreSafe(bool retry) const {return restore && (!retry || drained);}
};
int main() {
    const uint32_t reader[]={1,2,3},writer[]={4,5},rs[]={6,7},ws[]={8,9},image[]={0x101,0x202,0x303};
    for (unsigned mode=0;mode<2;mode++) {
        CombinedGuard guard;Platform platform{guard};
        for (unsigned i=0;i<platform.words.size();++i) platform.words[i]=i^0x86a3c151U;
        const auto originalFlash=platform.words;
        DataflowFlashCodePciView<Platform> view(platform);
        assert(view.Open(0x100000000ULL,0x11fffffffULL,guard));
        DataflowFlashRoleRouteLedger ledger;
        guard.expectedLedger=&ledger;
        DataflowFlashRolePciScope<RecoveryIO,decltype(view),CombinedGuard> io(view,guard,ledger);
        io.heldImageStageAllowed=true;io.heldImageRollbackAllowed=true;
        if (!mode) {
            const unsigned address=DataflowFlashCodeAddress;
            const uint32_t unchanged=platform.words[7*DataflowFlashCodeWords];
            assert(io.FlashWriteAllowed(7,address,4));
            guard.failRouteAt=guard.routeChecks+1;
            assert(!io.Write(7,address,&unchanged,4) && view.WritesConfirmed() && ledger.WriteUnconfirmed());
            guard.failRouteAt=UINT32_MAX;
            uint32_t output=0;
            assert(!io.Read(7,DataflowRoleExtraL1Address,&output,4));
            assert(!io.FlashReadAllowed(7,address,4) && !io.ConfirmRoleFlashWriteDrain());
            {
                DataflowFlashRolePciScope<RecoveryIO,decltype(view),CombinedGuard> resumed(view,guard,ledger);
                CombinedGuard otherGuard;
                DataflowFlashRolePciScope<RecoveryIO,decltype(view),CombinedGuard> wrong(view,otherGuard,ledger);
                DataflowFlashRoleRouteLedger freshLedger;
                DataflowFlashRolePciScope<RecoveryIO,decltype(view),CombinedGuard> fresh(view,guard,freshLedger);
                assert(!resumed.FlashReadAllowed(7,address,4) && !wrong.FlashFleetHeldAndQuiet());
                assert(!fresh.FlashFleetHeldAndQuiet() && !fresh.FlashReadAllowed(7,address,4));
                guard.drained=true;assert(!wrong.ConfirmRoleFlashWriteDrain());
                assert(resumed.ConfirmRoleFlashWriteDrain() && !ledger.WriteUnconfirmed());
            }
            guard.drained=false;
        }
        const auto originalCode=io.code,originalBoot=io.boots,originalShim=io.shims;
        const auto originalRam=io.ram,originalDisable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        std::vector<uint32_t> code(owner.CodeWords()),ram(owner.RamWords()),flash(DataflowFlashFleetSnapshotWords);
        assert(owner.SelectAutoRestoreMode());
        assert(owner.CaptureAllStacklessHeldImages(code.data(),code.size(),ram.data(),ram.size(),io));
        guard.common=false;assert(!owner.CaptureFlashAll(flash.data(),flash.size(),io));guard.common=true;
        assert(owner.CaptureFlashAll(flash.data(),flash.size(),io) && flash==originalFlash);
        uint32_t value=0;
        assert(!io.Read(0,DataflowFlashCodeLimit-4,&value,8));
        if (mode) platform.revokeAfterWrite=true;
        const bool staged=owner.StageFlashAll(image,3,io);
        if (mode) {
            assert(!staged && owner.RestorationRequired() && !view.WritesConfirmed());
            platform.revokeAfterWrite=false;guard.write=true;
            assert(!owner.RestoreFlashBeforeRoleStage(io));
            guard.drained=true;assert(!io.ConfirmRoleFlashWriteDrain());
            assert(view.ConfirmWriteDrain(guard));
            assert(io.ConfirmRoleFlashWriteDrain());
            assert(owner.RestoreFlashBeforeRoleStage(io));
        } else {
            assert(staged && owner.StageHeldImages(reader,3,writer,2,rs,2,ws,2,io));
            assert(!owner.BeginReleaseAttempt(io));
            assert(owner.RestoreHeldImagesBeforeRelease(io));
        }
        assert(owner.VerifyAfterResidentRestore(io));
        assert(platform.words==originalFlash && io.code==originalCode && io.boots==originalBoot
            && io.shims==originalShim && io.ram==originalRam && io.resetDisable==originalDisable);
        guard.retire=true;assert(view.Close(guard));
    }
    std::puts("PASS flash PCI scope and held role recovery composition; SIMULATED ONLY");
}
