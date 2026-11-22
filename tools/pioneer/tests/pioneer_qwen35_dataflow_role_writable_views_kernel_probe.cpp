/* SPDX-License-Identifier: MIT */
// Compile-only Haiku SDK witness; the simulated guard is never bound to IO.
#include "../PioneerQwen35DataflowRoleWritableViews.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowRoleHaikuMap.h"
struct WritableRoleKernelGuard {
    volatile bool okay=false;
    bool ProvePinnedBar(uint64_t,uint64_t) { return okay; }
    bool ProveSelectedRoute(unsigned) { return okay; }
    bool ProveReadAllowed(unsigned,unsigned,unsigned) { return okay; }
    bool ProveWriteAllowed(unsigned,unsigned,unsigned,unsigned) { return okay; }
    bool ProveControlWriteAllowed(unsigned,uint32_t,uint32_t) { return okay; }
};
bool PioneerWritableRoleKernelProbe(
    PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>& views,
    WritableRoleKernelGuard& guard,unsigned action,unsigned core,unsigned which,unsigned offset,
    uint32_t* words,unsigned bytes)
{
    if (action==0) return views.Open(0x4900000000ULL,0x491fffffffULL,guard);
    if (action==1 && words) return views.Read(core,which,offset,*words,guard);
    if (action==2) return views.WriteWords(core,which,offset,words,bytes,guard);
    if (action==3 && words && bytes>=8) return views.WriteResetDisable(core,words[0],words[1],guard);
    if (action==4) return views.Close();
    if (action==5) return views.ReopenForRecovery(guard);
    return false;
}
