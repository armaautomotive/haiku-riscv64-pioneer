/* SPDX-License-Identifier: MIT */
// Compile-only receipt lifecycle witness; no physical IO authority.
#include "pioneer_qwen35_copy_aware_compile_io.h"

extern "C" __attribute__((noinline)) bool
PioneerQ35CoreRestoreHandoffCompileWitness(PioneerQwen35::DataflowRoleFleetRecoveryOwner* owner,
    CopyAwareCompileIO* io,unsigned core,unsigned operation)
{
    if (!owner || !io) return false;
    switch (operation) {
        case 0: return owner->PrepareCoreLocalRestoreHandoff(core,*io);
        case 1: return owner->ConfirmCoreLocalRestoreHandoff(core,*io);
        case 2: return owner->RestoreCoreLocalHandoff(core,*io);
        default: return false;
    }
}
