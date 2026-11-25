/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_ROLE_SELECT_H
#define PIONEER_QWEN35_GUARDED_ROLE_SELECT_H
#include "BlackholeDramSamplesProbe.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowBarrierLayout.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleMailbox.h"

// Only the two TLB programming transactions in role selection are guarded
// here. The caller must separately guard its other register and view accesses
// and establish physical writer exclusion. A lost ticket poisons the selector
// permanently and leaves a potentially partially programmed route retained.
template<class IO,class Check> class PioneerQ35GuardedRoleSelector {
    IO& fIO;
    struct GuardedTlb {
        IO& io;Check& check;bool lost=false;
        bool Allowed() { if (lost || !check()) { lost=true;return false; } return true; }
        uint32_t DramTlbRead(unsigned slot,unsigned word) {
            if (!Allowed()) return UINT32_MAX;
            const uint32_t value=io.DramTlbRead(slot,word);
            return Allowed()?value:UINT32_MAX;
        }
        void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
            if (!Allowed()) return;
            io.DramTlbWrite(slot,word,value);(void)Allowed();
        }
    } fGuarded;
public:
    PioneerQ35GuardedRoleSelector(IO& io,Check& check):fIO(io),fGuarded{io,check} {}
    bool Allowed() { return fGuarded.Allowed(); }
    uint32_t DramTlbRead(unsigned slot,unsigned word) { return fGuarded.DramTlbRead(slot,word); }
    uint32_t ResidentNocRead(unsigned offset) {
        if (!Allowed()) return UINT32_MAX;
        const uint32_t value=fIO.ResidentNocRead(offset);
        return Allowed()?value:UINT32_MAX;
    }
    uint32_t ResetRead() {
        if (!Allowed()) return UINT32_MAX;
        const uint32_t value=fIO.ResetRead();
        return Allowed()?value:UINT32_MAX;
    }
    // A fresh recovery context has no graph-memory owner. Read this one fixed
    // handoff directly through its already selected panel, without invoking
    // the old session's ReadCoreMemory or changing its TLB selection.
    bool ReadRestoreHandoff(unsigned core,PioneerQwen35::DataflowRoleSnapshotHandoff& output) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !Allowed() || fIO.runtimeCore!=core
            || !fIO.DataflowPanelMappingStable() || !Allowed()) return false;
        uint32_t words[3];
        for (unsigned i=0;i<3;i++) {
            if (!Allowed()) return false;
            const uint32_t value=fIO.Q38PanelRead(DataflowRoleSnapshotHandoffAddress+i*4);
            if (!Allowed()) return false;
            words[i]=value;
        }
        output={words[0],words[1],words[2]};return true;
    }
    bool WriteRestoreHandoffWord(unsigned core,unsigned word,uint32_t value) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || word>=3 || !Allowed() || fIO.runtimeCore!=core
            || !fIO.DataflowPanelMappingStable() || !Allowed()) return false;
        fIO.Q38PanelWrite(DataflowRoleSnapshotHandoffAddress+word*4,value);
        return Allowed();
    }
    bool ReadRestoreHandoffWord(unsigned core,unsigned word,uint32_t& output) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || word>=3 || !Allowed() || fIO.runtimeCore!=core
            || !fIO.DataflowPanelMappingStable() || !Allowed()) return false;
        const uint32_t value=fIO.Q38PanelRead(DataflowRoleSnapshotHandoffAddress+word*4);
        if (!Allowed()) return false;
        output=value;return true;
    }
    bool SharedRolePrestackReleaseReset() {
        if (!Allowed()) return false;
        const bool okay=fIO.SharedRolePrestackReleaseReset();return Allowed() && okay;
    }
    bool SharedRoleHoldReset() {
        if (!Allowed()) return false;
        const bool okay=fIO.SharedRoleHoldReset();return Allowed() && okay;
    }
    bool Select(unsigned core) {
        const unsigned route=PioneerQwen35::DataflowBarrierWorkerRoute(core);
        if (route==UINT32_MAX || !fIO.fleetEnabled || !fGuarded.Allowed()) return false;
        const unsigned x=route&63U,y=(route>>6)&63U;
        if (!BlackholeDramConfigure(fGuarded,0,0,x,y,false)
            || !BlackholeDramConfigure(fGuarded,1,0xffa00000U,x,y,false)
            || !fGuarded.Allowed()) return false;
        fIO.runtimeCore=core;
        return true;
    }
};
#endif
