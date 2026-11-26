/* SPDX-License-Identifier: MIT */
// Held-only role recovery and flash code ownership over fake device memories.
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_Q35_RECOVERY_FIXTURE_ONLY 1
#include "pioneer_qwen35_dataflow_role_fleet_recovery_owner_test.cpp"
#include <cstring>
struct FlashRecoveryIO : RecoveryIO {
    std::vector<uint32_t> flash=std::vector<uint32_t>(DataflowFlashFleetSnapshotWords);
    bool common=true,snapshots=true,ramSafe=true,imageSafe=true,restoreSafe=true,retrySafe=true;
    int failFlashWrite=-1;unsigned flashWrites=0;
    FlashRecoveryIO() {for (unsigned i=0;i<flash.size();i++) flash[i]=i^0xc15139a7U;}
    bool FlashRoleCommonAuthority(const void* roles,const void* extra) const {return common && held && defaultRoute && roles && extra;}
    bool FlashRoleSnapshotsSafe(const uint32_t* roles,unsigned roleWords,const uint32_t* extra,unsigned flashWords,bool) const {
        return snapshots && roles && extra && roleWords==DataflowRoleFleetImagesOwner::RequiredWords()
            && flashWords==DataflowFlashFleetSnapshotWords;
    }
    bool FlashRoleRamSnapshotSafe(const uint32_t* ramStorage,unsigned ramWords,const uint32_t* extra,unsigned flashWords,bool) const {
        return ramSafe && ramStorage && extra && ramWords==DataflowRoleFleetRecoveryOwner::RamWords()
            && flashWords==DataflowFlashFleetSnapshotWords;
    }
    bool FlashRoleImagesSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,
        const uint32_t* c,unsigned cw,const uint32_t* d,unsigned dw) const {
        return imageSafe && a && aw && b && bw && c && cw && d && dw;
    }
    bool FlashRoleRestoreSafe(bool retry) const {return restoreSafe && (!retry || retrySafe);}
    bool FlashFleetHeldAndQuiet() const {return common && held && defaultRoute;}
    bool FlashSnapshotSafe(const uint32_t* p,unsigned n,bool) const {return snapshots && p && n==DataflowFlashFleetSnapshotWords;}
    bool FlashImageSafe(const uint32_t* p,unsigned n,const uint32_t* saved,unsigned total) const {
        return imageSafe && p && n && saved && total==DataflowFlashFleetSnapshotWords;
    }
    bool FlashRestoreSafe(bool retry) const {return restoreSafe && (!retry || retrySafe);}
    bool FlashReadAllowed(unsigned core,unsigned address,unsigned bytes) const {
        return FlashFleetHeldAndQuiet() && core<DataflowBarrierWorkers && DataflowFlashCodeRange(address,bytes);
    }
    bool FlashWriteAllowed(unsigned core,unsigned address,unsigned bytes) const {return FlashReadAllowed(core,address,bytes);}
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        if (!DataflowFlashCodeRange(address,bytes)) return RecoveryIO::Read(core,address,out,bytes);
        if (!FlashReadAllowed(core,address,bytes) || !out) return false;
        std::memcpy(out,flash.data()+core*DataflowFlashCodeWords+(address-DataflowFlashCodeAddress)/4,bytes);return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        if (!DataflowFlashCodeRange(address,bytes)) return RecoveryIO::Write(core,address,in,bytes);
        if (!FlashWriteAllowed(core,address,bytes) || !in) return false;
        const unsigned number=flashWrites++;
        std::memcpy(flash.data()+core*DataflowFlashCodeWords+(address-DataflowFlashCodeAddress)/4,in,bytes);
        return int(number)!=failFlashWrite;
    }
};
#ifndef PIONEER_Q35_FLASH_RECOVERY_FIXTURE_ONLY
int main() {
    const uint32_t reader[]={1,2,3},writer[]={4,5},readerShim[]={6,7},writerShim[]={8,9},flashImage[]={0x101,0x202,0x303};
    for (unsigned mode=0;mode<3;mode++) {
        FlashRecoveryIO io;const auto originalCode=io.code,originalBoot=io.boots,originalShim=io.shims;
        const auto originalRam=io.ram,originalDisable=io.resetDisable,originalFlash=io.flash;
        std::vector<uint32_t> code(DataflowRoleFleetRecoveryOwner::CodeWords()),ram(DataflowRoleFleetRecoveryOwner::RamWords());
        std::vector<uint32_t> flash(DataflowRoleFleetImagesOwner::FlashRequiredWords());
        DataflowRoleFleetRecoveryOwner owner;io.heldImageStageAllowed=true;io.heldImageRollbackAllowed=true;
        assert(!owner.CaptureAllStacklessHeldImages(code.data(),code.size(),code.data(),ram.size(),io));
        assert(!owner.CaptureAll(code.data(),code.size(),code.data(),ram.size(),io));
        assert(owner.SelectAutoRestoreMode() && owner.CaptureAllStacklessHeldImages(code.data(),code.size(),ram.data(),ram.size(),io));
        assert(owner.Untouched() && !owner.StageHeldImages(reader,3,writer,2,readerShim,2,writerShim,2,io));
        assert(!owner.CaptureFlashAll(ram.data(),flash.size(),io));
        io.ramSafe=false;assert(!owner.CaptureFlashAll(flash.data(),flash.size(),io));io.ramSafe=true;
        io.common=false;assert(!owner.CaptureFlashAll(flash.data(),flash.size(),io));io.common=true;
        assert(owner.CaptureFlashAll(flash.data(),flash.size(),io));
        assert(!owner.StageFlashAll(ram.data(),3,io));
        if (mode==2) io.failFlashWrite=0;
        const bool staged=owner.StageFlashAll(flashImage,3,io);
        if (mode==2) {
            assert(!staged && owner.RestorationRequired() && !owner.Untouched());
            io.failFlashWrite=-1;io.restoreSafe=false;assert(!owner.RestoreFlashBeforeRoleStage(io));io.restoreSafe=true;
            io.failFlashWrite=int(io.flashWrites);assert(!owner.RestoreFlashBeforeRoleStage(io));
            io.failFlashWrite=-1;io.retrySafe=false;assert(!owner.RestoreFlashBeforeRoleStage(io));io.retrySafe=true;
            assert(owner.RestoreFlashBeforeRoleStage(io));
        } else {
            assert(staged && owner.RestorationRequired() && !owner.Untouched());
            if (mode==1) assert(owner.RestoreFlashBeforeRoleStage(io));
            else {
                io.ramSafe=false;assert(!owner.StageHeldImages(reader,3,writer,2,readerShim,2,writerShim,2,io));io.ramSafe=true;
                assert(owner.StageHeldImages(reader,3,writer,2,readerShim,2,writerShim,2,io));
                assert(!owner.PrepareHeldPreservation(io) && !owner.PreparePossibleRelease(io)
                    && !owner.BeginReleaseAttempt(io) && !owner.RestoreAll(io));
                assert(owner.RestoreHeldImagesBeforeRelease(io));
            }
        }
        assert(owner.CoveredStateRestored() && owner.VerifyAfterResidentRestore(io) && !io.privateRamReads);
        assert(io.code==originalCode && io.boots==originalBoot && io.shims==originalShim
            && io.ram==originalRam && io.resetDisable==originalDisable && io.flash==originalFlash);
    }
    std::puts("PASS held role recovery includes flash window, RAM-snapshot protection and release refusal; NO HARDWARE");
}
#endif
