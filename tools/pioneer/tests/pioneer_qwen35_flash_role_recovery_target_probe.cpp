/* SPDX-License-Identifier: MIT */
// Compile-only composition. The IO declarations provide no physical proof.
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "pioneer_qwen35_copy_aware_compile_io.h"
struct FlashCompileIO : CopyAwareCompileIO {
    bool HeldImageStageAllowed();bool HeldImageRollbackAllowed();bool HeldImageWritesConfirmed();
    bool FlashRoleCommonAuthority(const void*,const void*);
    bool FlashRoleSnapshotsSafe(const uint32_t*,unsigned,const uint32_t*,unsigned,bool);
    bool FlashRoleRamSnapshotSafe(const uint32_t*,unsigned,const uint32_t*,unsigned,bool);
    bool FlashRoleImagesSafe(const uint32_t*,unsigned,const uint32_t*,unsigned,const uint32_t*,unsigned,const uint32_t*,unsigned);
    bool FlashRoleRestoreSafe(bool);
    bool FlashFleetHeldAndQuiet();bool FlashSnapshotSafe(const uint32_t*,unsigned,bool);
    bool FlashImageSafe(const uint32_t*,unsigned,const uint32_t*,unsigned);
    bool FlashRestoreSafe(bool);bool FlashReadAllowed(unsigned,unsigned,unsigned);bool FlashWriteAllowed(unsigned,unsigned,unsigned);
};
using namespace PioneerQwen35;
extern "C" bool q35_flash_role_capture(DataflowRoleFleetRecoveryOwner& owner,FlashCompileIO& io,
    uint32_t* code,unsigned codeWords,uint32_t* ram,unsigned ramWords,uint32_t* flash,unsigned flashWords) {
    return owner.CaptureAllStacklessHeldImages(code,codeWords,ram,ramWords,io)
        && owner.CaptureFlashAll(flash,flashWords,io);
}
extern "C" bool q35_flash_role_stage(DataflowRoleFleetRecoveryOwner& owner,FlashCompileIO& io,
    const uint32_t* flash,unsigned fw,const uint32_t* reader,unsigned rw,const uint32_t* writer,unsigned ww,
    const uint32_t* rs,unsigned rsw,const uint32_t* ws,unsigned wsw) {
    return owner.StageFlashAll(flash,fw,io) && owner.StageHeldImages(reader,rw,writer,ww,rs,rsw,ws,wsw,io);
}
extern "C" bool q35_flash_role_restore(DataflowRoleFleetRecoveryOwner& owner,FlashCompileIO& io) {
    return owner.RestoreHeldImagesBeforeRelease(io) && owner.VerifyAfterResidentRestore(io);
}
extern "C" bool q35_flash_role_restore_early(DataflowRoleFleetRecoveryOwner& owner,FlashCompileIO& io) {
    return owner.RestoreFlashBeforeRoleStage(io) && owner.VerifyAfterResidentRestore(io);
}
