/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLASH_ROLE_PCI_SCOPE_H
#define PIONEER_QWEN35_DATAFLOW_FLASH_ROLE_PCI_SCOPE_H
#include "PioneerQwen35DataflowFlashCodePciView.h"
namespace PioneerQwen35 {
// Retained with the physical view and guard across temporary adapters. The
// owner must serialize operations and keep all three objects alive; pointer
// equality does not prove physical mapping identity or prevent address reuse.
class DataflowFlashRoleRouteLedger {
    template<class,class,class> friend class DataflowFlashRolePciScope;
    const void* fView=nullptr;const void* fGuard=nullptr;
    bool fWriteUnconfirmed=false;
public:
    DataflowFlashRoleRouteLedger()=default;
    DataflowFlashRoleRouteLedger(const DataflowFlashRoleRouteLedger&)=delete;
    DataflowFlashRoleRouteLedger& operator=(const DataflowFlashRoleRouteLedger&)=delete;
    bool WriteUnconfirmed() const {return fWriteUnconfirmed;}
};
// Routes the extra fixed L1 window through the flash BAR view while keeping
// all other role operations on the existing role adapter. This is a guarded,
// non-loadable composition: Guard must bind the one retained ledger and both
// adapters to the same live session, route and buffers throughout each
// operation. It supplies neither those proofs nor processor release.
template<class RoleIO,class View,class Guard> class DataflowFlashRolePciScope : public RoleIO {
    DataflowFlashCodePciOwnerScope<View,Guard> fFlash;
    Guard& fGuard;
    DataflowFlashRoleRouteLedger& fLedger;
    bool fBound=false;
    bool Bound() {
        return fBound && fGuard.ProveFlashRoleLedgerBound(static_cast<RoleIO*>(this),&fFlash,&fLedger);
    }
    bool SameRoute(unsigned core) {
        return Bound() && fGuard.ProveFlashRoleRouteSame(static_cast<RoleIO*>(this),&fFlash,core);
    }
public:
    template<class... Args> explicit DataflowFlashRolePciScope(View& view,Guard& guard,
        DataflowFlashRoleRouteLedger& ledger,Args&... args)
        :RoleIO(args...),fFlash(view,guard),fGuard(guard),fLedger(ledger) {
        if (!fLedger.fView && !fLedger.fGuard) {fLedger.fView=&view;fLedger.fGuard=&guard;}
        fBound=fLedger.fView==&view && fLedger.fGuard==&guard;
    }
    DataflowFlashRolePciScope(const DataflowFlashRolePciScope&)=delete;
    DataflowFlashRolePciScope& operator=(const DataflowFlashRolePciScope&)=delete;
    bool Read(unsigned core,unsigned address,uint32_t* output,unsigned bytes) {
        if (!Bound() || fLedger.fWriteUnconfirmed) return false;
        if (DataflowFlashCodeRange(address,bytes))
            return SameRoute(core) && fFlash.Read(core,address,output,bytes) && SameRoute(core);
        return DataflowRoleExtraL1Range(address,bytes) && RoleIO::Read(core,address,output,bytes) && Bound();
    }
    bool Write(unsigned core,unsigned address,const uint32_t* input,unsigned bytes) {
        if (!Bound() || fLedger.fWriteUnconfirmed) return false;
        if (DataflowFlashCodeRange(address,bytes)) {
            if (!SameRoute(core)) return false;
            // A lost cross-adapter route proof after a write is uncertain even
            // if the flash view's own local readback succeeded.
            fLedger.fWriteUnconfirmed=true;
            if (!fFlash.Write(core,address,input,bytes) || !SameRoute(core)) return false;
            fLedger.fWriteUnconfirmed=false;return true;
        }
        return DataflowRoleExtraL1Range(address,bytes) && RoleIO::Write(core,address,input,bytes) && Bound();
    }
    bool FlashRoleCommonAuthority(const void* roles,const void* extra) {
        return Bound() && fGuard.ProveFlashRoleCommonAuthority(static_cast<RoleIO*>(this),&fFlash,roles,extra);
    }
    bool FlashRoleSnapshotsSafe(const uint32_t* roles,unsigned rw,const uint32_t* extra,unsigned ew,bool frozen) {
        return Bound() && fGuard.ProveFlashRoleSnapshotsSafe(roles,rw,extra,ew,frozen);
    }
    bool FlashRoleRamSnapshotSafe(const uint32_t* ram,unsigned rw,const uint32_t* extra,unsigned ew,bool frozen) {
        return Bound() && fGuard.ProveFlashRoleRamSnapshotSafe(ram,rw,extra,ew,frozen);
    }
    bool FlashRoleImagesSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,
        const uint32_t* c,unsigned cw,const uint32_t* d,unsigned dw) {
        return Bound() && fGuard.ProveFlashRoleImagesSafe(a,aw,b,bw,c,cw,d,dw);
    }
    bool FlashRoleRestoreSafe(bool retry) {
        return Bound() && !fLedger.fWriteUnconfirmed && fFlash.FlashFleetHeldAndQuiet()
            && fGuard.ProveFlashRoleRestoreSafe(retry);
    }
    bool FlashFleetHeldAndQuiet() {return Bound() && !fLedger.fWriteUnconfirmed && fFlash.FlashFleetHeldAndQuiet();}
    bool FlashSnapshotSafe(const uint32_t* p,unsigned n,bool frozen) {return Bound() && fFlash.FlashSnapshotSafe(p,n,frozen);}
    bool FlashImageSafe(const uint32_t* p,unsigned n,const uint32_t* saved,unsigned words) {
        return Bound() && fFlash.FlashImageSafe(p,n,saved,words);
    }
    bool FlashRestoreSafe(bool retry) {return Bound() && !fLedger.fWriteUnconfirmed && fFlash.FlashRestoreSafe(retry);}
    bool FlashReadAllowed(unsigned core,unsigned address,unsigned bytes) {
        return Bound() && !fLedger.fWriteUnconfirmed && fFlash.FlashReadAllowed(core,address,bytes) && SameRoute(core);
    }
    bool FlashWriteAllowed(unsigned core,unsigned address,unsigned bytes) {
        return Bound() && !fLedger.fWriteUnconfirmed && fFlash.FlashWriteAllowed(core,address,bytes) && SameRoute(core);
    }
    // The view's own uncertain write must be independently drained first.
    // This separate proof covers a lost role/flash route relationship.
    bool ConfirmRoleFlashWriteDrain() {
        if (!Bound() || !fLedger.fWriteUnconfirmed || !fFlash.FlashFleetHeldAndQuiet()
            || !fGuard.ProveFlashRoleWriteDrained(static_cast<RoleIO*>(this),&fFlash)
            || !Bound() || !fFlash.FlashFleetHeldAndQuiet()
            || !fGuard.ProveFlashRoleWriteDrained(static_cast<RoleIO*>(this),&fFlash) || !Bound()) return false;
        fLedger.fWriteUnconfirmed=false;return true;
    }
};
}
#endif
