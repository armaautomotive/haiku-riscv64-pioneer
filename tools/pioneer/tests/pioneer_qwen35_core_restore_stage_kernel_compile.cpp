/* SPDX-License-Identifier: MIT */
// Compile-only staging witness; no physical scope or release authority.
#include "pioneer_qwen35_copy_aware_compile_io.h"

extern "C" __attribute__((noinline)) bool
PioneerQ35CoreRestoreStageCompileWitness(PioneerQwen35::DataflowRoleFleetRecoveryOwner* owner,
    CopyAwareCompileIO* io,unsigned core,const uint32_t* reader,const uint32_t* writer,unsigned words)
{
    return owner && io && owner->StageCoreLocalRestore(core,reader,words,writer,words,*io);
}

extern "C" __attribute__((noinline)) bool
PioneerQ35CoreRestoreReleaseCompileWitness(PioneerQwen35::DataflowRoleFleetRecoveryOwner* owner,
    CopyAwareCompileIO* io,unsigned core)
{
    return owner && io && owner->BeginCoreLocalRestoreRelease(core,*io);
}
