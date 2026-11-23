/* SPDX-License-Identifier: MIT */
// Compile the actual copy-aware owners against Haiku's RISC-V SDK. These
// declarations have no implementation and cannot bind to a live driver.
#include "pioneer_qwen35_copy_aware_compile_io.h"

extern "C" __attribute__((noinline)) bool
PioneerQ35CopyAwareCompileWitness(PioneerQwen35::DataflowRoleFleetImagesOwner* images,
    PioneerQwen35::DataflowRoleFleetLocalRamOwner* ram,CopyAwareCompileIO* io,
    unsigned core,const uint32_t* reader,const uint32_t* writer,unsigned words)
{
    if (!images || !ram || !io || !reader || !writer || !words) return false;
    return images->BeginCopyAwareOwnership() && images->VerifyStagedAllWithCopies(*io)
        && images->RestageOneWithCopies(core,reader,words,writer,words,*io)
        && ram->MarkCoreRestoredByCore(core,*io) && ram->VerifyStacklessRestoredByCore(*io);
}
extern "C" __attribute__((noinline)) bool
PioneerQ35CoreLocalFleetCompileWitness(PioneerQwen35::DataflowRoleFleetRecoveryOwner* owner,
    CopyAwareCompileIO* io,unsigned core)
{
    return owner && io && owner->SelectCoreLocalRestoreMode()
        && owner->MarkCoreRestoredByCore(core,*io) && owner->VerifyCoreLocalRestores(*io);
}
