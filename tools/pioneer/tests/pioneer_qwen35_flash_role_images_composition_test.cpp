/* SPDX-License-Identifier: MIT */
// Actual role-image owner plus flash fleet owner over fake, disjoint memories.
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_Q35_IMAGES_FIXTURE_ONLY 1
#include "pioneer_qwen35_dataflow_role_fleet_images_owner_test.cpp"
#include <cstring>
struct CompositeIO : ImagesIO {
    std::vector<uint32_t> flash=std::vector<uint32_t>(DataflowFlashFleetSnapshotWords);
    bool common=true,snapshots=true,imageSafe=true,restoreSafe=true,retrySafe=true;
    bool corruptRoleOnFlash=false,corruptFlashOnRole=false;
    int failFlashWrite=-1;unsigned flashWrites=0;
    CompositeIO() {for (unsigned i=0;i<flash.size();i++) flash[i]=i^0xb15342a7U;}
    bool FlashRoleCommonAuthority(const void* roles,const void* extra) const {
        return common && held && defaultRoute && roles && extra;
    }
    bool FlashRoleSnapshotsSafe(const uint32_t* roles,unsigned roleWords,const uint32_t* extra,unsigned flashWords,bool) const {
        return snapshots && roles && extra && roleWords==DataflowRoleFleetImagesOwner::RequiredWords()
            && flashWords==DataflowFlashFleetSnapshotWords;
    }
    bool FlashRoleRestoreSafe(bool retry) const {return restoreSafe && (!retry || retrySafe);}
    bool FlashRoleImagesSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,
        const uint32_t* c,unsigned cw,const uint32_t* d,unsigned dw) const {
        return imageSafe && a && aw && b && bw && c && cw && d && dw;
    }
    bool FlashFleetHeldAndQuiet() const {return held && defaultRoute && common;}
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
        if (!DataflowFlashCodeRange(address,bytes)) return ImagesIO::Read(core,address,out,bytes);
        if (!FlashReadAllowed(core,address,bytes) || !out) return false;
        std::memcpy(out,flash.data()+core*DataflowFlashCodeWords+(address-DataflowFlashCodeAddress)/4,bytes);return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        if (!DataflowFlashCodeRange(address,bytes)) {
            const bool okay=ImagesIO::Write(core,address,in,bytes);
            if (okay && corruptFlashOnRole && core==3) {flash[0]^=1;corruptFlashOnRole=false;}
            return okay;
        }
        if (!FlashWriteAllowed(core,address,bytes) || !in) return false;
        const unsigned number=flashWrites++;
        std::memcpy(flash.data()+core*DataflowFlashCodeWords+(address-DataflowFlashCodeAddress)/4,in,bytes);
        if (corruptRoleOnFlash && core==3) {code[119*PerCore]^=1;corruptRoleOnFlash=false;}
        return int(number)!=failFlashWrite;
    }
};
int main() {
    const uint32_t reader[]={1,2,3},writer[]={4,5},readerShim[]={6,7},writerShim[]={8,9};
    const uint32_t flashImage[]={0x101,0x202,0x303};
    for (unsigned mode=0;mode<8;mode++) {
        CompositeIO io;const auto originalCode=io.code,originalBoot=io.boots,originalShim=io.shims,originalFlash=io.flash;
        std::vector<uint32_t> roleSnapshot(DataflowRoleFleetImagesOwner::RequiredWords());
        std::vector<uint32_t> flashSnapshot(DataflowRoleFleetImagesOwner::FlashRequiredWords());
        DataflowRoleFleetImagesOwner owner;
        assert(owner.CaptureAll(roleSnapshot.data(),roleSnapshot.size(),io));
        assert(!owner.StageAll(reader,3,writer,2,readerShim,2,writerShim,2,io) && !io.writes);
        assert(!owner.CaptureFlashAll(roleSnapshot.data(),owner.FlashRequiredWords(),io));
        io.common=false;assert(!owner.CaptureFlashAll(flashSnapshot.data(),flashSnapshot.size(),io));io.common=true;
        assert(owner.CaptureFlashAll(flashSnapshot.data(),flashSnapshot.size(),io) && owner.FlashCaptured());
        assert(!owner.StageFlashAll(roleSnapshot.data(),3,io));
        io.snapshots=false;assert(!owner.StageFlashAll(flashImage,3,io) && !io.flashWrites);io.snapshots=true;
        if (mode==1) io.corruptRoleOnFlash=true;
        if (mode==3) io.failFlashWrite=0;
        const bool flashStaged=owner.StageFlashAll(flashImage,3,io);
        if (mode==1 || mode==3) {
            assert(!flashStaged && owner.RestorationRequired());
            io.restoreSafe=false;assert(!owner.RestoreAll(io));io.restoreSafe=true;
            if (mode==3) {
                io.failFlashWrite=int(io.flashWrites);assert(!owner.RestoreAll(io));
                io.failFlashWrite=-1;io.retrySafe=false;assert(!owner.RestoreAll(io));io.retrySafe=true;
            }
            assert(owner.RestoreAll(io) && owner.ImagesRestored() && owner.FlashRegionRestored());
        } else {
            assert(flashStaged && owner.FlashStagedAndVerified() && owner.RestorationRequired());
            if (mode==4) {
                assert(owner.RestoreAll(io) && owner.ImagesRestored() && owner.FlashRegionRestored());
                assert(io.code==originalCode && io.boots==originalBoot && io.shims==originalShim
                    && io.flash==originalFlash && owner.VerifyOriginalAll(io));
                continue;
            }
            if (mode==6) {
                io.flash[0]^=1;const unsigned prior=io.writes;
                assert(!owner.StageAll(reader,3,writer,2,readerShim,2,writerShim,2,io)
                    && owner.RestorationRequired() && io.writes==prior);
                assert(owner.RestoreAll(io) && owner.ImagesRestored() && owner.FlashRegionRestored());
                assert(io.code==originalCode && io.boots==originalBoot && io.shims==originalShim
                    && io.flash==originalFlash && owner.VerifyOriginalAll(io));
                continue;
            }
            if (mode==2) io.corruptFlashOnRole=true;
            io.imageSafe=false;const unsigned priorRoleWrites=io.writes;
            assert(!owner.StageAll(reader,3,writer,2,readerShim,2,writerShim,2,io) && io.writes==priorRoleWrites);
            io.imageSafe=true;
            const bool staged=owner.StageAll(reader,3,writer,2,readerShim,2,writerShim,2,io);
            if (mode==2) assert(!staged && owner.RestorationRequired());
            else {
                assert(staged && owner.StagedAndVerified() && owner.VerifyStagedAll(io));
                assert(!owner.StageOne(0,reader,3,writer,2,readerShim,2,writerShim,2,io));
                if (mode==5) {
                    io.imageSafe=false;assert(!owner.VerifyStagedAll(io) && owner.RestorationRequired());
                    io.imageSafe=true;assert(owner.RestoreAll(io));
                    assert(io.code==originalCode && io.boots==originalBoot && io.shims==originalShim
                        && io.flash==originalFlash && owner.VerifyOriginalAll(io));
                    continue;
                }
                if (mode==7) {
                    io.corruptRoleOnFlash=true;assert(!owner.RestoreAll(io) && owner.RestorationRequired());
                    io.retrySafe=false;assert(!owner.RestoreAll(io));io.retrySafe=true;
                    assert(owner.RestoreAll(io));
                    assert(io.code==originalCode && io.boots==originalBoot && io.shims==originalShim
                        && io.flash==originalFlash && owner.VerifyOriginalAll(io));
                    continue;
                }
                assert(owner.BeginCopyAwareOwnership());
                io.InstallCapturedCopies(3);io.InstallCapturedCopies(119);
                assert(owner.VerifyStagedAllWithCopies(io));
                assert(!owner.RestoreAll(io));io.retirementProof=true;
                assert(owner.RestoreAfterFleetRamProof(io));
                assert(owner.ImagesRestored() && owner.FlashRegionRestored());
            }
            if (mode==2) assert(owner.RestoreAll(io) && owner.ImagesRestored() && owner.FlashRegionRestored());
        }
        assert(io.code==originalCode && io.boots==originalBoot && io.shims==originalShim
            && io.flash==originalFlash && owner.VerifyOriginalAll(io));
    }
    std::puts("PASS role images and flash window share retained ownership and cross-audited restoration; NO HARDWARE");
}
