/* SPDX-License-Identifier: MIT */
// Actual service and role-recovery owners composed over simulated devices.
// No PCI mapping, reset operation or hardware access is present.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define main qwen35_single_service_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#define main qwen35_role_recovery_test_main
#include "pioneer_qwen35_dataflow_role_fleet_recovery_owner_test.cpp"
#undef main
#include <memory>
#ifdef PIONEER_QWEN35_EXPORTED_SHARED_IMAGES
#include "PioneerQwen35SharedRoleImages.h"
#endif

struct CombinedSharedIO : MockDataflowIO {
    RecoveryIO roles;
    DataflowRoleFleetRecoveryOwner roleOwner;
    std::vector<uint32_t> savedCode,savedRam;
    bool roleCaptured=false,corruptAfterResidentRestore=false;
    const uint32_t reader[3]={1,2,3},writer[2]={4,5};
    const uint32_t readerShim[2]={6,7},writerShim[2]={8,9};
    CombinedSharedIO():savedCode(DataflowRoleFleetRecoveryOwner::CodeWords()),
        savedRam(DataflowRoleFleetRecoveryOwner::RamWords()) {}
    bool SharedCaptureAndStageHeld() {
        if (!MockDataflowIO::SharedCaptureAndStageHeld()) return false;
        if (!roleOwner.SelectComputeLocalRamPreservation()) return false;
        if (!roleOwner.CaptureAllStackless(savedCode.data(),savedCode.size(),savedRam.data(),savedRam.size(),roles))
            return false;
        roleCaptured=true;
#ifdef PIONEER_QWEN35_EXPORTED_SHARED_IMAGES
        return roleOwner.StageImages(DataflowSharedReaderCodeWords,
            sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),DataflowSharedWriterCodeWords,
            sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t),DataflowSharedReaderShimWords,
            sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),DataflowSharedWriterShimWords,
            sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t),roles);
#else
        return roleOwner.StageImages(reader,3,writer,2,readerShim,2,writerShim,2,roles);
#endif
    }
    bool SharedArmAndBeginRelease() {
        return MockDataflowIO::SharedArmAndBeginRelease()
            && roleOwner.PreparePossibleRelease(roles) && roleOwner.BeginReleaseAttempt(roles);
    }
    bool SharedFleetHeldAndQuiescent() const {
        return MockDataflowIO::SharedFleetHeldAndQuiescent() && roles.held && roles.stable;
    }
    bool SharedStartCore(unsigned core) {
        if (!MockDataflowIO::SharedStartCore(core)) return false;
        roles.held=false;
        return true;
    }
    bool SharedPrepareRoleStacks(unsigned core) {
        if (!roleOwner.CaptureStacklessCore(core,roles) || !roleOwner.MarkStacklessMayRun(core,roles)) return false;
        if (!MockDataflowIO::SharedPrepareRoleStacks(core)) return false;
        // The private stack is touched only after the copy handoff.
        roles.ram[core*RecoveryIO::RamPerCore]^=0x11;
        return true;
    }
    bool SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
        if (!MockDataflowIO::SharedStopAndHoldAll(stage,core)) return false;
        roles.held=true; roles.stable=true; return true;
    }
    bool SharedRestoreRolesHeld() {
        if (!MockDataflowIO::SharedRestoreRolesHeld()) return false;
        if (!roleCaptured) return !roleOwner.RestorationRequired();
        if (roleOwner.CoveredStateRestored() && roleOwner.VerifyAfterResidentRestore(roles)) return true;
        return roleOwner.RestoreAll(roles);
    }
    bool RestoreCoreState(unsigned core) {
        if (!MockDataflowIO::RestoreCoreState(core)) return false;
        if (corruptAfterResidentRestore && core==0) {
            roles.shims[0]^=1; corruptAfterResidentRestore=false;
        }
        return true;
    }
    bool SharedVerifyRolesAfterResidentRestore() {
        return MockDataflowIO::SharedVerifyRolesAfterResidentRestore()
            && (!roleCaptured || roleOwner.VerifyAfterResidentRestore(roles));
    }
};

int main()
{
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    {
        auto io=std::make_unique<CombinedSharedIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        const auto codeBefore=io->roles.code,ramBefore=io->roles.ram;
        const auto bootBefore=io->roles.boots,shimBefore=io->roles.shims,disableBefore=io->roles.resetDisable;
        assert(owner->Install(image,*io)==DataflowServiceComplete && owner->Ready());
        assert(!io->roles.privateRamReads);
        for (unsigned core=0;core<DataflowCoreCount;core++)
            assert(io->roles.resetDisable[core]==(disableBefore[core]|DataflowSharedPreserveResetMask));
#ifdef PIONEER_QWEN35_EXPORTED_SHARED_IMAGES
        uint32_t readerBoot=0,writerBoot=0;
        assert(DataflowRoleBootWord(DataflowRoleReader,DataflowRoleReaderShimAddress,readerBoot));
        assert(DataflowRoleBootWord(DataflowRoleWriter,DataflowRoleWriterShimAddress,writerBoot));
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            const unsigned start=core*RecoveryIO::CodePerCore;
            const unsigned writerOffset=(DataflowRoleWriterCodeAddress-DataflowRoleExtraL1Address)/4;
            assert(!memcmp(io->roles.code.data()+start,DataflowSharedReaderCodeWords,
                sizeof(DataflowSharedReaderCodeWords)));
            assert(!memcmp(io->roles.code.data()+start+writerOffset,DataflowSharedWriterCodeWords,
                sizeof(DataflowSharedWriterCodeWords)));
            assert(io->roles.boots[core*2]==readerBoot && io->roles.boots[core*2+1]==writerBoot);
            assert(!memcmp(io->roles.shims.data()+core*DataflowRoleShimWords,DataflowSharedReaderShimWords,
                sizeof(DataflowSharedReaderShimWords)));
            assert(!memcmp(io->roles.shims.data()+core*DataflowRoleShimWords+(DataflowRoleWriterShimAddress-DataflowRoleReaderShimAddress)/4,DataflowSharedWriterShimWords,
                sizeof(DataflowSharedWriterShimWords)));
        }
#endif
        io->corruptAfterResidentRestore=true;
        assert(owner->Close(*io)==DataflowServiceRestoreFailed && !io->resumeCount);
        assert(owner->Close(*io)==DataflowServiceClosed && io->resumeCount==1);
        assert(io->roles.code==codeBefore && io->roles.ram==ramBefore);
        assert(io->roles.boots==bootBefore && io->roles.shims==shimBefore);
        assert(io->roles.resetDisable==disableBefore);
    }
    {
        auto io=std::make_unique<CombinedSharedIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        const auto disableBefore=io->roles.resetDisable;
        const auto ramBefore=io->roles.ram;
        const unsigned readsPerTile=DataflowRoleCount*(DataflowRoleLocalRamWords/64)*3;
        io->roles.failCopyCall=3*readsPerTile+2;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed && io->roleStarts==4);
        assert(io->roleOwner.RestorationRequired() && io->roleOwner.OriginalRamWords(0,DataflowRoleReader));
        assert(!io->roleOwner.OriginalRamWords(3,DataflowRoleReader) && !io->roles.privateRamReads);
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            assert(io->roleStarted[core]==(core<4));
            assert(io->roleOwner.StacklessCopiesCaptured(core)==(core<3));
        }
        io->roles.failCopyCall=UINT32_MAX;
        assert(owner->Close(*io)==DataflowServiceClosed && io->resumeCount==1);
        assert(io->roles.ram==ramBefore && io->roles.resetDisable==disableBefore);
    }
    {
        auto io=std::make_unique<CombinedSharedIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        const auto disableBefore=io->roles.resetDisable;
        io->failStartCore=37;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed && io->roleStarts==37);
        for (unsigned core=0;core<DataflowCoreCount;core++)
            assert(io->roleOwner.StacklessCopiesCaptured(core)==(core<37));
        assert(owner->Close(*io)==DataflowServiceClosed && io->resumeCount==1);
        assert(io->roles.resetDisable==disableBefore);
    }
    {
        auto io=std::make_unique<CombinedSharedIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        const auto disableBefore=io->roles.resetDisable;
        io->roles.failDisableCore=37;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed && !io->roleStarts);
        assert(owner->Close(*io)==DataflowServiceClosed && io->resumeCount==1);
        assert(io->roles.resetDisable==disableBefore);
    }
    std::puts("PASS shared service and 120-tile recovery owners, partial release and restore retry; SIMULATED ONLY");
}
