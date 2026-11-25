/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FRESH_RESIDENT_RECOVERY_IO_H
#define PIONEER_QWEN35_FRESH_RESIDENT_RECOVERY_IO_H
#if !defined(PIONEER_QWEN35_FABRIC_GUARD_WITNESS) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY)
#error Fresh resident recovery IO is restricted to the non-loadable fabric witness
#endif
#include "PioneerQwen35GuardedRoleSelect.h"
#include "PioneerQwen35DataflowNocSafetySample.h"

// The caller's check binds the retained snapshot owner, exact fresh session,
// ticket and physical writer exclusion. A lost check is sticky across every
// TLB, reset, panel and boot access. No callback here proves writer exclusion.
template<class OldIO,class FreshIO,class Check> class PioneerQ35FreshResidentRecoveryIO {
    OldIO& fOld;
    FreshIO& fFresh;
    Check& fCheck;
    bool fLost=false;
    bool Permit() { if (fLost || !fCheck()) { fLost=true;return false; } return true; }
    struct CheckProxy {
        PioneerQ35FreshResidentRecoveryIO& owner;
        bool operator()() { return owner.Permit(); }
    } fProxy;
    PioneerQ35GuardedRoleSelector<FreshIO,CheckProxy> fSelector;
public:
    unsigned& runtimeCore;
    PioneerQ35FreshResidentRecoveryIO(OldIO& old,FreshIO& fresh,Check& check)
        :fOld(old),fFresh(fresh),fCheck(check),fProxy{*this},fSelector(fresh,fProxy),runtimeCore(fresh.runtimeCore) {}
    bool FabricRecoveryAllowed() { return Permit() && fSelector.Allowed(); }
    bool SelectRuntimeCore(unsigned core) {
        return Permit() && fSelector.Select(core) && Permit()
            && PioneerQwen35WithSelectedClock(*this,core,[]() { return true; }) && Permit();
    }
    bool DataflowPanelMappingStable() { return Permit() && fFresh.DataflowPanelMappingStable() && Permit(); }
    uint32_t ResidentNocRead(unsigned offset) { return fSelector.ResidentNocRead(offset); }
    void Pause() { fFresh.Pause(); }
    bool DataflowInvalidateServiceImage(unsigned core) {
        if (!Permit()) return false;
        const bool okay=fOld.DataflowInvalidateServiceImage(core);return Permit() && okay;
    }
    uint32_t ResetRead() { return fSelector.ResetRead(); }
    uint32_t Q38PanelRead(unsigned address) {
        if (!Permit()) return UINT32_MAX;
        const uint32_t value=fFresh.Q38PanelRead(address);return Permit()?value:UINT32_MAX;
    }
    void Q38PanelWrite(unsigned address,uint32_t value) {
        if (!Permit()) return;
        fFresh.Q38PanelWrite(address,value);(void)Permit();
    }
    uint32_t RuntimeCodeRead(unsigned word) {
        if (!Permit()) return UINT32_MAX;
        const uint32_t value=fFresh.RuntimeCodeRead(word);return Permit()?value:UINT32_MAX;
    }
    void RuntimeCodeWrite(unsigned word,uint32_t value) {
        if (!Permit()) return;
        fFresh.RuntimeCodeWrite(word,value);(void)Permit();
    }
};
#endif
