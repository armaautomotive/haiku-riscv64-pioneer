/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLASH_ROLE_PCI_SCOPE_H
#define PIONEER_QWEN35_DATAFLOW_FLASH_ROLE_PCI_SCOPE_H
#include "PioneerQwen35DataflowFlashCodePciView.h"
namespace PioneerQwen35 {
// Routes the extra fixed L1 window through the flash BAR view while keeping
// all other role operations on the existing role adapter. This is a guarded,
// non-loadable composition: Guard must bind both adapters to the same live
// retained session, route and buffers throughout each operation. It does not
// supply those physical proofs or authorize processor release.
template<class RoleIO,class View,class Guard> class DataflowFlashRolePciScope : public RoleIO {
    DataflowFlashCodePciOwnerScope<View,Guard> fFlash;
    Guard& fGuard;
public:
    template<class... Args> explicit DataflowFlashRolePciScope(View& view,Guard& guard,Args&... args)
        :RoleIO(args...),fFlash(view,guard),fGuard(guard) {}
    DataflowFlashRolePciScope(const DataflowFlashRolePciScope&)=delete;
    DataflowFlashRolePciScope& operator=(const DataflowFlashRolePciScope&)=delete;
    bool Read(unsigned core,unsigned address,uint32_t* output,unsigned bytes) {
        if (DataflowFlashCodeRange(address,bytes)) return fFlash.Read(core,address,output,bytes);
        return DataflowRoleExtraL1Range(address,bytes) && RoleIO::Read(core,address,output,bytes);
    }
    bool Write(unsigned core,unsigned address,const uint32_t* input,unsigned bytes) {
        if (DataflowFlashCodeRange(address,bytes)) return fFlash.Write(core,address,input,bytes);
        return DataflowRoleExtraL1Range(address,bytes) && RoleIO::Write(core,address,input,bytes);
    }
    bool FlashRoleCommonAuthority(const void* roles,const void* extra) {
        return fGuard.ProveFlashRoleCommonAuthority(static_cast<RoleIO*>(this),&fFlash,roles,extra);
    }
    bool FlashRoleSnapshotsSafe(const uint32_t* roles,unsigned rw,const uint32_t* extra,unsigned ew,bool frozen) {
        return fGuard.ProveFlashRoleSnapshotsSafe(roles,rw,extra,ew,frozen);
    }
    bool FlashRoleRamSnapshotSafe(const uint32_t* ram,unsigned rw,const uint32_t* extra,unsigned ew,bool frozen) {
        return fGuard.ProveFlashRoleRamSnapshotSafe(ram,rw,extra,ew,frozen);
    }
    bool FlashRoleImagesSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,
        const uint32_t* c,unsigned cw,const uint32_t* d,unsigned dw) {
        return fGuard.ProveFlashRoleImagesSafe(a,aw,b,bw,c,cw,d,dw);
    }
    bool FlashRoleRestoreSafe(bool retry) {return fGuard.ProveFlashRoleRestoreSafe(retry);}
    bool FlashFleetHeldAndQuiet() {return fFlash.FlashFleetHeldAndQuiet();}
    bool FlashSnapshotSafe(const uint32_t* p,unsigned n,bool frozen) {return fFlash.FlashSnapshotSafe(p,n,frozen);}
    bool FlashImageSafe(const uint32_t* p,unsigned n,const uint32_t* saved,unsigned words) {
        return fFlash.FlashImageSafe(p,n,saved,words);
    }
    bool FlashRestoreSafe(bool retry) {return fFlash.FlashRestoreSafe(retry);}
    bool FlashReadAllowed(unsigned core,unsigned address,unsigned bytes) {return fFlash.FlashReadAllowed(core,address,bytes);}
    bool FlashWriteAllowed(unsigned core,unsigned address,unsigned bytes) {return fFlash.FlashWriteAllowed(core,address,bytes);}
};
}
#endif
