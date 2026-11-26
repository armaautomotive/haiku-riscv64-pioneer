/* SPDX-License-Identifier: MIT */
// Compile-only: declarations supply no physical ownership or PCI access.
#include "pioneer_qwen35_flash_role_recovery_target_probe.cpp"
#include "../PioneerQwen35DataflowFlashRolePciScope.h"
struct FlashCompileView {
    bool Opened();bool WritesConfirmed();
    template<class Guard> bool Read(unsigned,unsigned,uint32_t*,unsigned,Guard&);
    template<class Guard> bool Write(unsigned,unsigned,const uint32_t*,unsigned,Guard&);
};
struct FlashCompileGuard {
    bool ProveFlashRoleCommonAuthority(const void*,const void*,const void*,const void*);
    bool ProveFlashRoleSnapshotsSafe(const uint32_t*,unsigned,const uint32_t*,unsigned,bool);
    bool ProveFlashRoleRamSnapshotSafe(const uint32_t*,unsigned,const uint32_t*,unsigned,bool);
    bool ProveFlashRoleImagesSafe(const uint32_t*,unsigned,const uint32_t*,unsigned,const uint32_t*,unsigned,const uint32_t*,unsigned);
    bool ProveFlashRoleRestoreSafe(bool);bool ProveFlashFleetHeldAndQuiet();
    bool ProveFlashSnapshotSafe(const uint32_t*,unsigned,bool);
    bool ProveFlashImageSafe(const uint32_t*,unsigned,const uint32_t*,unsigned);
    bool ProveFlashRestoreSafe(bool);bool SelectFlashRoute(unsigned);
    bool ProveFlashReadAllowed(unsigned,unsigned,unsigned);bool ProveFlashWriteAllowed(unsigned,unsigned,unsigned);
};
using FlashRoleScope=DataflowFlashRolePciScope<FlashCompileIO,FlashCompileView,FlashCompileGuard>;
extern "C" bool q35_flash_pci_role_capture(DataflowRoleFleetRecoveryOwner& owner,FlashRoleScope& io,
    uint32_t* code,unsigned cw,uint32_t* ram,unsigned rw,uint32_t* flash,unsigned fw) {
    return owner.CaptureAllStacklessHeldImages(code,cw,ram,rw,io) && owner.CaptureFlashAll(flash,fw,io);
}
extern "C" bool q35_flash_pci_role_stage(DataflowRoleFleetRecoveryOwner& owner,FlashRoleScope& io,
    const uint32_t* flash,unsigned fw,const uint32_t* reader,unsigned rw,const uint32_t* writer,unsigned ww,
    const uint32_t* rs,unsigned rsw,const uint32_t* ws,unsigned wsw) {
    return owner.StageFlashAll(flash,fw,io) && owner.StageHeldImages(reader,rw,writer,ww,rs,rsw,ws,wsw,io);
}
extern "C" bool q35_flash_pci_role_restore(DataflowRoleFleetRecoveryOwner& owner,FlashRoleScope& io) {
    return owner.RestoreHeldImagesBeforeRelease(io) && owner.VerifyAfterResidentRestore(io);
}
