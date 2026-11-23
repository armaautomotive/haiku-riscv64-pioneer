/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ROLE_PRESTACK_PROBE_H
#define PIONEER_QWEN35_ROLE_PRESTACK_PROBE_H
#if !defined(PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY) || !defined(PIONEER_QWEN35_ROLE_STAGE_QUALIFY)
#error This bounded startup probe is only for the restricted qualification build
#endif
#include "PioneerQwen35DataflowRoleFleetPciIO.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleBoot.h"

// Qualification only. A held-image owner must already retain the entire
// fleet's boot, shim and extra-L1 snapshots. This starts B/NC on worker zero
// without releasing any TRISC, cancels the pre-stack handoff, checks both
// actual RAM copies against the held private RAM, and restores the handoff
// and reset-preservation word. It does not prove concurrent firmware
// exclusion, initialization history, PCI fault recovery or inference.
// Any failure retains this object and its containing image owner; it never
// retries an uncertain store, release or hold.
struct PioneerQwen35RolePrestackState {
    uint32_t handoff[3]{},resetDisable=0;
    unsigned handoffZeroAttempts=0,handoffRestoreAttempts=0;
    bool attempted=false,captured=false,preserveAttempted=false,releaseAttempted=false;
    bool cancelAttempted=false,holdAttempted=false,copiesVerified=false;
    bool resetRestoreAttempted=false,complete=false;
};
template<class IO,class Scope,class Views,class Adapter>
class PioneerQwen35RolePrestackProbe {
    IO& fIO;
    Scope& fScope;
    Views& fViews;
    Adapter& fAdapter;
    PioneerQwen35RoleRouteGuard<IO,Scope> fGuard;
    PioneerQwen35RolePrestackState& fState;
    static constexpr unsigned Core=0;
    static constexpr uint32_t Held=0x47800,Running=0x7000;
    static void Progress(unsigned phase) {
#ifdef _KERNEL_MODE
        dprintf("P100A_Q35_PRESTACK_PROGRESS phase=%u\n",phase);
#else
        (void)phase;
#endif
    }
    static void RamFailure(unsigned role,unsigned offset,unsigned kind) {
#ifdef _KERNEL_MODE
        // The addresses and reason suffice for diagnosis; never print saved
        // private-RAM contents from another device session.
        dprintf("P100A_Q35_PRESTACK_RAM_FAILURE role=%u offset=%u kind=%u\n",role,offset,kind);
#else
        (void)role;(void)offset;(void)kind;
#endif
    }

    bool Select(uint32_t reset) {
        return fScope.PrestackAdmission(fIO) && fIO.SelectRuntimeCore(Core)
            && fIO.MapQ38Panel() && fIO.DataflowPanelMappingStable()
            && fGuard.ProveSelectedRoute(Core)
            && PioneerQwen35WithSelectedClock(fIO,Core,[&]() {
                return fIO.ResetRead()==reset && fIO.ResetRead()==reset;
            });
    }
    bool ReadHandoff(uint32_t reset,uint32_t (&out)[3]) {
        using namespace PioneerQwen35;
        if (!Select(reset)) return false;
        uint32_t first[3]{};
        for (unsigned i=0;i<3;i++) first[i]=fIO.Q38PanelRead(DataflowRoleSnapshotHandoffAddress+i*4);
        fIO.Pause();
        for (unsigned i=0;i<3;i++) out[i]=fIO.Q38PanelRead(DataflowRoleSnapshotHandoffAddress+i*4);
        return !memcmp(first,out,sizeof(first)) && Select(reset);
    }
    bool WriteHandoff(uint32_t reset,unsigned word,uint32_t value) {
        using namespace PioneerQwen35;
        if (word>=3 || !Select(reset)) return false;
        const unsigned address=DataflowRoleSnapshotHandoffAddress+word*4;
        fIO.Q38PanelWrite(address,value); // one attempt; failure retains the owner
        return fIO.Q38PanelRead(address)==value && Select(reset);
    }
    bool ResetDisable(uint32_t expected) {
        using namespace PioneerQwen35;
        uint32_t first=0,second=0;
        return Select(Held)
            && fViews.Read(Core,DataflowRolePhysicalControl,0x224,first,fGuard)
            && fViews.Read(Core,DataflowRolePhysicalControl,0x224,second,fGuard)
            && first==expected && second==expected && Select(Held);
    }
    bool ComparePrivateRam() {
        using namespace PioneerQwen35;
        // Keep the kernel stack bounded while checking all 4096 RAM words.
        uint32_t copy[16],ram[16];
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            const unsigned copied=role==DataflowRoleReader?DataflowRoleReaderRamCopy:DataflowRoleWriterRamCopy;
            const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
            for (unsigned offset=0;offset<DataflowRoleLocalRamBytes;offset+=sizeof(copy)) {
                if (!fAdapter.Read(Core,copied+offset,copy,sizeof(copy))) {
                    RamFailure(role,offset,1);return false;
                }
                if (!fAdapter.ReadRam(Core,role,base+offset,ram,sizeof(ram))) {
                    RamFailure(role,offset,2);return false;
                }
                if (memcmp(copy,ram,sizeof(copy))) { RamFailure(role,offset,3);return false; }
            }
        }
        if (!fAdapter.AllRolesHeld(Core)) { RamFailure(DataflowRoleCount,0,4);return false; }
        return true;
    }
public:
    PioneerQwen35RolePrestackProbe(IO& io,Scope& scope,Views& views,Adapter& adapter,
        PioneerQwen35RolePrestackState& state)
        :fIO(io),fScope(scope),fViews(views),fAdapter(adapter),fGuard(io,scope),fState(state) {}
    PioneerQwen35RolePrestackProbe(const PioneerQwen35RolePrestackProbe&)=delete;
    PioneerQwen35RolePrestackProbe& operator=(const PioneerQwen35RolePrestackProbe&)=delete;
    bool Attempted() const { return fState.attempted; }
    bool Complete() const { return fState.complete; }

    __attribute__((noinline)) bool Run(uint32_t readerPark,uint32_t writerPark,unsigned polls) {
        using namespace PioneerQwen35;
        if (fState.attempted || !polls || polls>65536
            || (readerPark&3) || readerPark<DataflowRoleReaderShimAddress || readerPark>=DataflowRoleReaderShimLimit
            || (writerPark&3) || writerPark<DataflowRoleWriterShimAddress || writerPark>=DataflowRoleWriterShimLimit)
            return false;
        fState.attempted=true;
        uint32_t first=0,second=0;
        if (!fScope.BeginPrestack(fIO) || !fAdapter.AllRolesHeld(Core) || !fAdapter.DefaultBootRouting(Core)
            || !ReadHandoff(Held,fState.handoff) || !Select(Held)
            || !fViews.Read(Core,DataflowRolePhysicalControl,0x224,first,fGuard)
            || !fViews.Read(Core,DataflowRolePhysicalControl,0x224,second,fGuard) || first!=second) return false;
        fState.resetDisable=first;fState.captured=true;Progress(1);
        for (unsigned i=0;i<3;i++) {
            fState.handoffZeroAttempts|=1U<<i;
            if (!WriteHandoff(Held,i,0)) return false;
        }
        const uint32_t preserved=fState.resetDisable|DataflowRolePreserveResetMask;
        fState.preserveAttempted=true;
        if (!fViews.WriteResetDisable(Core,fState.resetDisable,preserved,fGuard) || !ResetDisable(preserved)
            || !Select(Held)) return false;
        fState.releaseAttempted=true;
        Progress(2);
        if (!fIO.SharedRolePrestackReleaseReset() || !Select(Running)) return false;
        bool ready=false;
        for (unsigned n=0;n<polls;n++) {
            uint32_t handoff[3]{};
            if (ReadHandoff(Running,handoff) && handoff[0]==DataflowRoleSnapshotReaderReady
                && handoff[1]==DataflowRoleSnapshotWriterReady && !handoff[2]) { ready=true;break; }
            fIO.Pause();
        }
        if (!ready) return false;
        Progress(3);
        fState.cancelAttempted=true;
        if (!WriteHandoff(Running,2,DataflowRoleSnapshotCancel)) return false;
        bool parked=false;
        for (unsigned n=0;n<polls;n++) {
            uint32_t handoff[3]{};
            if (ReadHandoff(Running,handoff) && handoff[0]==DataflowRoleSnapshotReaderCancelled
                && handoff[1]==DataflowRoleSnapshotWriterCancelled && handoff[2]==DataflowRoleSnapshotCancel) {
                const uint32_t r1=fIO.SharedRolePcRead(DataflowRoleReader),w1=fIO.SharedRolePcRead(DataflowRoleWriter);
                fIO.Pause();
                const uint32_t r2=fIO.SharedRolePcRead(DataflowRoleReader),w2=fIO.SharedRolePcRead(DataflowRoleWriter);
                if (r1==readerPark && r2==readerPark && w1==writerPark && w2==writerPark
                    && Select(Running)) { parked=true;break; }
            }
            fIO.Pause();
        }
        if (!parked || !Select(Running)) return false;
        Progress(4);
        fState.holdAttempted=true;
        if (!fIO.SharedRoleHoldReset()) return false;
        Progress(41);
        if (!Select(Held)) return false;
        Progress(42);
        if (!fScope.MarkPrestackHeldAfterCancel(fIO)) return false;
        Progress(43);
        if (!ResetDisable(preserved)) return false;
        Progress(44);
        if (!ComparePrivateRam()) return false;
        fState.copiesVerified=true;Progress(5);
        for (unsigned i=0;i<3;i++) {
            fState.handoffRestoreAttempts|=1U<<i;
            if (!WriteHandoff(Held,i,fState.handoff[i])) return false;
        }
        fState.resetRestoreAttempted=true;
        if (!fViews.WriteResetDisable(Core,preserved,fState.resetDisable,fGuard)
            || !ResetDisable(fState.resetDisable)) return false;
        uint32_t restored[3]{};
        if (!ReadHandoff(Held,restored) || memcmp(restored,fState.handoff,sizeof(restored))) return false;
        fState.complete=true;Progress(6);return true;
    }
    // Called after the containing owner restores the entire saved image fleet.
    bool VerifyAfterImages() {
        uint32_t restored[3]{};
        const bool okay=fState.complete && ReadHandoff(Held,restored)
            && !memcmp(restored,fState.handoff,sizeof(restored)) && ResetDisable(fState.resetDisable)
            && fAdapter.AllRolesHeld(Core);
        if (okay) Progress(7);
        return okay;
    }
};
#endif
