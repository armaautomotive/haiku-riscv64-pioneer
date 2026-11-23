/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ROLE_PRESTACK_PROBE_H
#define PIONEER_QWEN35_ROLE_PRESTACK_PROBE_H
#if !defined(PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY) || !defined(PIONEER_QWEN35_ROLE_STAGE_QUALIFY)
#error This bounded startup probe is only for the restricted qualification build
#endif
#if defined(PIONEER_QWEN35_ROLE_FAULT_QUALIFY) && defined(_KERNEL_MODE)
#error Fault qualification is offline only until separately tagged and recovery-qualified
#endif
#include "PioneerQwen35DataflowRoleFleetPciIO.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleBoot.h"
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleMailbox.h"
#endif
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
#ifndef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
#error Fault qualification requires the guarded resident role service
#endif
#include "../../../../../tools/pioneer/PioneerQwen35DataflowKernels.h"
#endif

// Qualification only. A held-image owner must already retain the entire
// fleet's boot, shim and extra-L1 snapshots. This starts B/NC on worker zero
// without releasing any TRISC, audits the core-verified L1 copies before
// and after cancellation, and restores the handoff and reset-preservation
// word. It does not prove concurrent firmware
// exclusion, initialization history, PCI fault recovery or inference.
// Any failure retains this object and its containing image owner; it never
// retries an uncertain store, release or hold.
struct PioneerQwen35RolePrestackState {
    uint32_t handoff[3]{},resetDisable=0;
    // The enclosing qualification owner is heap-allocated, not on the kernel stack.
    uint32_t copies[PioneerQwen35::DataflowRoleCount][PioneerQwen35::DataflowRoleLocalRamWords]{};
    unsigned handoffZeroAttempts=0,handoffRestoreAttempts=0;
    bool attempted=false,captured=false,preserveAttempted=false,releaseAttempted=false;
    bool copiesCaptured=false,cancelAttempted=false,holdAttempted=false,copiesVerified=false;
    bool resetRestoreAttempted=false,complete=false;
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
    uint32_t mailbox[16]{},mailboxReadFirst[16]{},mailboxReadSecond[16]{};
    uint32_t mailboxInitAttempts=0,mailboxRestoreAttempts=0;
    bool mailboxCaptured=false,serviceReleaseAttempted=false,serviceStarted=false;
    bool serviceStopAttempted=false,serviceStopped=false,mailboxRestored=false;
#endif
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
    uint32_t faultSaved[1+PioneerQwen35::DataflowQueueStateBytes/4]{};
    unsigned faultWriteAttempts=0,faultRestoreAttempts=0;
    bool faultCaptured=false,faultPrepared=false,faultCommandAttempted=false;
    bool faultSettled=false,faultRestored=false;
#endif
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
    // The verified shims are waiting without touching their private stacks.
    // This guard permits only the two fixed L1 copy windows and samples the
    // same running reset and selected route around every bounded read.
    class RunningCopyGuard {
        PioneerQwen35RolePrestackProbe& fProbe;
    public:
        explicit RunningCopyGuard(PioneerQwen35RolePrestackProbe& probe):fProbe(probe) {}
        bool ProvePinnedBar(uint64_t start,uint64_t end) { return fProbe.fGuard.ProvePinnedBar(start,end); }
        bool ProveSelectedRoute(unsigned core) { return fProbe.fGuard.ProveSelectedRoute(core); }
        bool ProveReadAllowed(unsigned core,unsigned which,unsigned offset) {
            using namespace PioneerQwen35;
            return core==Core && which==DataflowRolePhysicalExtraCode
                && offset>=DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address
                && offset<DataflowRoleExtraL1Limit-DataflowRoleExtraL1Address
                && fProbe.Select(Running);
        }
    };
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
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
    static constexpr unsigned FaultWordCount=1+PioneerQwen35::DataflowQueueStateBytes/4;
    static constexpr unsigned FaultInvocationAddress=PioneerQwen35::DataflowProgramAddress
        +offsetof(PioneerQwen35::DataflowCoreInvocation,circularBuffer)
        +offsetof(PioneerQwen35::DataflowCircularBuffer,address);
    static_assert(FaultInvocationAddress==0xaa074 && FaultWordCount==25,
        "fixed offline fault-test memory layout changed");
    static unsigned FaultAddress(unsigned word) {
        return word?PioneerQwen35::DataflowQueueStateAddress+(word-1)*4:FaultInvocationAddress;
    }
    bool ReadFaultWord(uint32_t reset,unsigned word,uint32_t& out) {
        if (word>=FaultWordCount || !Select(reset)) return false;
        const uint32_t first=fIO.Q38PanelRead(FaultAddress(word));fIO.Pause();
        const uint32_t second=fIO.Q38PanelRead(FaultAddress(word));
        if (first!=second || !Select(reset)) return false;
        out=second;return true;
    }
    bool WriteFaultWord(uint32_t reset,unsigned word,uint32_t value) {
        if (word>=FaultWordCount || !Select(reset)) return false;
        fIO.Q38PanelWrite(FaultAddress(word),value);
        return fIO.Q38PanelRead(FaultAddress(word))==value && Select(reset);
    }
    bool FaultNocQuiet(uint32_t reset) {
        if (!Select(reset)) return false;
        for (unsigned pass=0;pass<2;pass++) {
            if (!PioneerQwen35SampleNocFifosAndClock([&](unsigned offset) { return fIO.ResidentNocRead(offset); }))
                return false;
            for (unsigned noc=0;noc<2;noc++) {
                const unsigned base=noc?0x10000:0;
                for (unsigned i=0;i<4;i++) if (fIO.ResidentNocRead(base+0x40+i*0x800)) return false;
                for (unsigned i=0;i<32;i++) if (fIO.ResidentNocRead(base+0x240+i*4)&0xffU) return false;
            }
            fIO.Pause();
            if (!Select(reset)) return false;
        }
        return true;
    }
    bool PrepareFaultWords() {
        if (fState.faultCaptured || !FaultNocQuiet(Held)) return false;
        for (unsigned i=0;i<FaultWordCount;i++)
            if (!ReadFaultWord(Held,i,fState.faultSaved[i])) return false;
        fState.faultCaptured=true;
        // Address zero makes DataflowBindQueues fail before any NoC operation.
        for (unsigned i=0;i<FaultWordCount;i++) {
            fState.faultWriteAttempts|=1U<<i;
            if (!WriteFaultWord(Held,i,0)) return false;
        }
        fState.faultPrepared=FaultNocQuiet(Held);return fState.faultPrepared;
    }
    bool FaultQueuesExpected() {
        using namespace PioneerQwen35;
        for (unsigned queue=0;queue<DataflowCircularBufferCount;queue++) {
            for (unsigned word=0;word<DataflowQueueControlBytes/4;word++) {
                uint32_t value=0;
                const uint32_t expected=word==2?DataflowFailureReader:word==4?DataflowFailureWriter:0;
                if (!ReadFaultWord(Running,1+queue*(DataflowQueueControlBytes/4)+word,value)
                    || value!=expected) return false;
            }
        }
        return true;
    }
    bool RestoreFaultWords() {
        if (!fState.faultCaptured || !fState.faultSettled || !fState.serviceStopped
            || !fState.copiesVerified || !FaultNocQuiet(Held)) return false;
        for (unsigned i=0;i<FaultWordCount;i++) {
            fState.faultRestoreAttempts|=1U<<i;
            if (!WriteFaultWord(Held,i,fState.faultSaved[i])) return false;
        }
        fState.faultRestored=true;return true;
    }
    bool VerifyFaultWords() {
        if (!fState.faultRestored) return false;
        for (unsigned i=0;i<FaultWordCount;i++) {
            uint32_t value=0;
            if (!ReadFaultWord(Held,i,value) || value!=fState.faultSaved[i]) return false;
        }
        return true;
    }
#endif
    bool ReadMailbox(uint32_t reset,PioneerQwen35::DataflowRoleMailbox& out) {
        using namespace PioneerQwen35;
        static_assert(sizeof(DataflowRoleMailbox)==16*sizeof(uint32_t),"fixed mailbox size");
        if (!Select(reset)) return false;
        auto& first=fState.mailboxReadFirst;auto& second=fState.mailboxReadSecond;
        for (unsigned i=0;i<16;i++) first[i]=fIO.Q38PanelRead(DataflowRoleMailboxAddress+i*4);
        fIO.Pause();
        for (unsigned i=0;i<16;i++) second[i]=fIO.Q38PanelRead(DataflowRoleMailboxAddress+i*4);
        if (memcmp(first,second,sizeof(fState.mailboxReadFirst)) || !Select(reset)) return false;
        memcpy(&out,second,sizeof(out));return true;
    }
    bool WriteMailbox(uint32_t reset,unsigned word,uint32_t value) {
        using namespace PioneerQwen35;
        if (word>=16 || !Select(reset)) return false;
        const unsigned address=DataflowRoleMailboxAddress+word*4;
        fIO.Q38PanelWrite(address,value);
        return fIO.Q38PanelRead(address)==value && Select(reset);
    }
    bool InitializeMailbox() {
        using namespace PioneerQwen35;
        DataflowRoleMailbox previous{};
        if (!ReadMailbox(Held,previous)) return false;
        memcpy(fState.mailbox,&previous,sizeof(previous));fState.mailboxCaptured=true;
        for (unsigned i=0;i<16;i++) {
            const uint32_t value=i==0?uint32_t(DataflowRoleMailboxMagic)
                :i==1?uint32_t(DataflowRoleMailboxVersion):0U;
            fState.mailboxInitAttempts|=1U<<i;
            if (!WriteMailbox(Held,i,value)) return false;
        }
        DataflowRoleMailbox expected{};
        expected.magic=DataflowRoleMailboxMagic;expected.version=DataflowRoleMailboxVersion;
        return ReadMailbox(Held,previous) && !memcmp(&previous,&expected,sizeof(expected));
    }
    bool RestoreMailbox() {
        using namespace PioneerQwen35;
        if (!fState.mailboxCaptured || !fState.serviceStopped || !fState.copiesVerified) return false;
        for (unsigned i=0;i<16;i++) {
            fState.mailboxRestoreAttempts|=1U<<i;
            if (!WriteMailbox(Held,i,fState.mailbox[i])) return false;
        }
        DataflowRoleMailbox restored{};
        fState.mailboxRestored=ReadMailbox(Held,restored)
            && !memcmp(&restored,fState.mailbox,sizeof(restored));
        return fState.mailboxRestored;
    }
    bool RunIdleService(unsigned polls
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
        ,bool faultProbe
#endif
        ) {
        using namespace PioneerQwen35;
        fState.serviceReleaseAttempted=true;
        if (!WriteHandoff(Running,2,DataflowRoleSnapshotRelease)) return false;
        for (unsigned n=0;n<polls;n++) {
            DataflowRoleMailbox box{};
            if (ReadMailbox(Running,box) && DataflowRoleMailboxStartupReady(box)) {
                const uint32_t r=fIO.SharedRolePcRead(DataflowRoleReader),w=fIO.SharedRolePcRead(DataflowRoleWriter);
                if (r>=DataflowRoleReaderCodeAddress && r<DataflowRoleReaderCodeLimit
                    && w>=DataflowRoleWriterCodeAddress && w<DataflowRoleWriterCodeLimit
                    && Select(Running)) { fState.serviceStarted=true;break; }
            }
            fIO.Pause();
        }
        if (!fState.serviceStarted) return false;
        DataflowRoleMailbox box{};
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
        if (!ReadMailbox(Running,box)) return false;
        if (faultProbe) {
            if (!fState.faultPrepared || !FaultNocQuiet(Running) || !DataflowRoleMailboxIssue(box,1)) return false;
            fState.faultCommandAttempted=true;
            if (!WriteMailbox(Running,offsetof(DataflowRoleMailbox,command)/sizeof(uint32_t),1)) return false;
            for (unsigned n=0;n<polls;n++) {
                if (ReadMailbox(Running,box)
                    && DataflowRoleMailboxReceipt(box,DataflowRoleReader,1,DataflowRoleFailed)
                    && DataflowRoleMailboxReceipt(box,DataflowRoleWriter,1,DataflowRoleFailed)) {
                    fState.faultSettled=true;break;
                }
                fIO.Pause();
            }
            if (!fState.faultSettled || !FaultQueuesExpected() || !FaultNocQuiet(Running)
                || !DataflowRoleMailboxRequestFaultStop(box,true,true,true)) return false;
        } else
        if (!DataflowRoleMailboxRequestStop(box)) return false;
#else
        if (!ReadMailbox(Running,box) || !DataflowRoleMailboxRequestStop(box)) return false;
#endif
        fState.serviceStopAttempted=true;
        constexpr unsigned stopWord=offsetof(DataflowRoleMailbox,stopRequest)/sizeof(uint32_t);
        if (!WriteMailbox(Running,stopWord,1)) return false;
        for (unsigned n=0;n<polls;n++) {
            if (ReadMailbox(Running,box) && DataflowRoleMailboxStopReceipts(box)) {
                fState.serviceStopped=true;return true;
            }
            fIO.Pause();
        }
        return false;
    }
#endif
    bool ResetDisable(uint32_t expected) {
        using namespace PioneerQwen35;
        uint32_t first=0,second=0;
        return Select(Held)
            && fViews.Read(Core,DataflowRolePhysicalControl,0x224,first,fGuard)
            && fViews.Read(Core,DataflowRolePhysicalControl,0x224,second,fGuard)
            && first==expected && second==expected && Select(Held);
    }
    bool CaptureRunningCopies() {
        using namespace PioneerQwen35;
        uint32_t handoff[3]{};
        if (!ReadHandoff(Running,handoff) || handoff[0]!=DataflowRoleSnapshotReaderReady
            || handoff[1]!=DataflowRoleSnapshotWriterReady || handoff[2]) return false;
        RunningCopyGuard guard(*this);
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            const unsigned copied=role==DataflowRoleReader?DataflowRoleReaderRamCopy:DataflowRoleWriterRamCopy;
            if (!fViews.CopyWords(Core,DataflowRolePhysicalExtraCode,copied-DataflowRoleExtraL1Address,
                    fState.copies[role],DataflowRoleLocalRamWords,guard)) { RamFailure(role,0,5);return false; }
        }
        // Check both complete copies again after all initial reads.
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            const unsigned copied=role==DataflowRoleReader?DataflowRoleReaderRamCopy:DataflowRoleWriterRamCopy;
            if (!fViews.Matches(Core,DataflowRolePhysicalExtraCode,copied-DataflowRoleExtraL1Address,
                    fState.copies[role],DataflowRoleLocalRamWords,guard)) { RamFailure(role,0,6);return false; }
        }
        if (!ReadHandoff(Running,handoff) || handoff[0]!=DataflowRoleSnapshotReaderReady
            || handoff[1]!=DataflowRoleSnapshotWriterReady || handoff[2]) return false;
        fState.copiesCaptured=true;return true;
    }
    bool CompareHeldCopies() {
        using namespace PioneerQwen35;
        if (!fState.copiesCaptured || !Select(Held)) return false;
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            const unsigned copied=role==DataflowRoleReader?DataflowRoleReaderRamCopy:DataflowRoleWriterRamCopy;
            if (!fViews.Matches(Core,DataflowRolePhysicalExtraCode,copied-DataflowRoleExtraL1Address,
                    fState.copies[role],DataflowRoleLocalRamWords,fGuard)) { RamFailure(role,0,7);return false; }
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

    __attribute__((noinline)) bool Run(uint32_t readerPark,uint32_t writerPark,unsigned polls,
        bool localRestoreProbe=false,bool serviceProbe=false
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
        ,bool faultProbe=false
#endif
        ) {
        using namespace PioneerQwen35;
#ifndef PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY
        if (localRestoreProbe) return false;
#endif
#ifndef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
        if (serviceProbe) return false;
#endif
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
        if (faultProbe && !serviceProbe) return false;
#endif
        if (localRestoreProbe && serviceProbe) return false;
        const bool restoresLocal=localRestoreProbe || serviceProbe;
        const unsigned readerFirst=restoresLocal?DataflowRoleReaderCodeAddress:DataflowRoleReaderShimAddress;
        const unsigned readerLimit=restoresLocal?DataflowRoleReaderCodeLimit:DataflowRoleReaderShimLimit;
        const unsigned writerFirst=restoresLocal?DataflowRoleWriterCodeAddress:DataflowRoleWriterShimAddress;
        const unsigned writerLimit=restoresLocal?DataflowRoleWriterCodeLimit:DataflowRoleWriterShimLimit;
        if (fState.attempted || !polls || polls>65536
            || (readerPark&3) || readerPark<readerFirst || readerPark>=readerLimit
            || (writerPark&3) || writerPark<writerFirst || writerPark>=writerLimit)
            return false;
        fState.attempted=true;
        uint32_t first=0,second=0;
        if (!fScope.BeginPrestack(fIO) || !fAdapter.AllRolesHeld(Core) || !fAdapter.DefaultBootRouting(Core)
            || !ReadHandoff(Held,fState.handoff) || !Select(Held)
            || !fViews.Read(Core,DataflowRolePhysicalControl,0x224,first,fGuard)
            || !fViews.Read(Core,DataflowRolePhysicalControl,0x224,second,fGuard) || first!=second) return false;
        fState.resetDisable=first;fState.captured=true;Progress(1);
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
        if (serviceProbe && !InitializeMailbox()) return false;
#endif
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
        if (faultProbe && !PrepareFaultWords()) return false;
#endif
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
        if (!CaptureRunningCopies()) return false;
        fState.cancelAttempted=true;
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
        if (serviceProbe) {
            if (!RunIdleService(polls
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
                ,faultProbe
#endif
                )) return false;
        } else
#endif
        if (!WriteHandoff(Running,2,DataflowRoleSnapshotCancel)) return false;
        bool parked=false;
        for (unsigned n=0;n<polls;n++) {
            uint32_t handoff[3]{};
            const uint32_t readerDone=restoresLocal?uint32_t(PIONEER_Q35_ROLE_RESTORE_READER_DONE)
                :uint32_t(DataflowRoleSnapshotReaderCancelled);
            const uint32_t writerDone=restoresLocal?uint32_t(PIONEER_Q35_ROLE_RESTORE_WRITER_DONE)
                :uint32_t(DataflowRoleSnapshotWriterCancelled);
            if (ReadHandoff(Running,handoff) && handoff[0]==readerDone
                && handoff[1]==writerDone && handoff[2]==(serviceProbe?DataflowRoleSnapshotRelease:DataflowRoleSnapshotCancel)) {
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
        if (!CompareHeldCopies()) return false;
        fState.copiesVerified=true;Progress(5);
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
        if (faultProbe && !RestoreFaultWords()) return false;
#endif
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
        if (serviceProbe && !RestoreMailbox()) return false;
#endif
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
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
        PioneerQwen35::DataflowRoleMailbox mailbox{};
        const bool mailboxOkay=!fState.mailboxCaptured || (fState.mailboxRestored
            && ReadMailbox(Held,mailbox) && !memcmp(&mailbox,fState.mailbox,sizeof(mailbox)));
#endif
        const bool okay=fState.complete
#ifdef PIONEER_QWEN35_ROLE_FAULT_QUALIFY
            && (!fState.faultCaptured || VerifyFaultWords())
#endif
#ifdef PIONEER_QWEN35_ROLE_SERVICE_QUALIFY
            && mailboxOkay
#endif
            && ReadHandoff(Held,restored)
            && !memcmp(restored,fState.handoff,sizeof(restored)) && ResetDisable(fState.resetDisable)
            && fAdapter.AllRolesHeld(Core);
        if (okay) Progress(7);
        return okay;
    }
};
#endif
