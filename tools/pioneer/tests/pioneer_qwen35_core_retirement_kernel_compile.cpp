/* SPDX-License-Identifier: MIT */
// Compile-only full-fleet retirement witness; no live scope grants authority.
#include "pioneer_qwen35_copy_aware_compile_io.h"

extern "C" __attribute__((noinline)) bool
PioneerQ35CoreRetirementCompileWitness(PioneerQwen35::DataflowRoleFleetRecoveryOwner* owner,
    CopyAwareCompileIO* io,unsigned operation)
{
    if (!owner || !io) return false;
    if (operation==0) return owner->RestoreAfterCoreLocalFleetProof(*io);
    if (operation==1) return owner->VerifyAfterCoreLocalResidentRestore(*io);
    return false;
}
