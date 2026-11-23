/* SPDX-License-Identifier: MIT */
#pragma once
#include "../PioneerQwen35DataflowRoleFleetImagesOwner.h"
#include "../PioneerQwen35DataflowRoleFleetLocalRamOwner.h"
#include "../PioneerQwen35DataflowRoleFleetRecoveryOwner.h"

struct CopyAwareCompileIO {
    bool AllRolesHeld(unsigned);
    bool DefaultBootRouting(unsigned);
    bool Read(unsigned,unsigned,uint32_t*,unsigned);
    bool Write(unsigned,unsigned,const uint32_t*,unsigned);
    bool ReadBoot(unsigned,unsigned,uint32_t&);
    bool WriteBoot(unsigned,unsigned,uint32_t);
    bool ReadShim(unsigned,unsigned,uint32_t*,unsigned);
    bool WriteShim(unsigned,unsigned,const uint32_t*,unsigned);
    bool StacklessHeldCopySources(unsigned,const uint32_t*&,const uint32_t*&,bool&);
    bool StacklessHeldCopySourceSafe(unsigned,const uint32_t*,const uint32_t*);
    bool StacklessCoreRestoreProven(unsigned,const uint32_t*,const uint32_t*);
    bool StacklessCoreRestoreReleaseReady(unsigned,const uint32_t*,const uint32_t*);
    bool StacklessUncapturedRamProtected(unsigned);
    bool StacklessFleetPreparationSafe(unsigned);
    bool LocalRamStable(unsigned);
    bool ReadResetDisable(unsigned,uint32_t&);
    bool WriteResetDisable(unsigned,uint32_t);
    bool ReadRam(unsigned,unsigned,uint32_t,uint32_t*,unsigned);
    bool WriteRam(unsigned,unsigned,uint32_t,const uint32_t*,unsigned);
};
