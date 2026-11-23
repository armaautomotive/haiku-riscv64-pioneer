/* SPDX-License-Identifier: MIT */
// Compile-only lifecycle witness; no live IO implementation.
#include "pioneer_qwen35_copy_aware_compile_io.h"

extern "C" __attribute__((noinline)) bool
PioneerQ35CoreLocalReleaseCompileWitness(PioneerQwen35::DataflowRoleFleetRecoveryOwner* owner,
    CopyAwareCompileIO* io)
{
    return owner && io && owner->PreparePossibleRelease(*io)
        && owner->BeginReleaseAttempt(*io) && owner->RestoreAll(*io);
}
