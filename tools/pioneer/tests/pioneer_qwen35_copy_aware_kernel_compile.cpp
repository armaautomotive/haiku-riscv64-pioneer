/* SPDX-License-Identifier: MIT */
// Compile the actual copy-aware owners against Haiku's RISC-V SDK. These
// declarations have no implementation and cannot bind to a live driver.
#include "../PioneerQwen35DataflowRoleFleetImagesOwner.h"
#include "../PioneerQwen35DataflowRoleFleetLocalRamOwner.h"

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
    bool StacklessUncapturedRamProtected(unsigned);
};

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
