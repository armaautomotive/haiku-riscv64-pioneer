/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_PCI_IO_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_PCI_IO_H
#include <stddef.h>
#include <string.h>
#if !defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Role fleet PCI adapter is a non-loadable recovery witness only
#endif
#include "PioneerQwen35DataflowRoleRouteGuard.h"
#include "PioneerQwen35DataflowNocSafetySample.h"
#include "PioneerQwen35DataflowPciMemory.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleFleetRecoveryOwner.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleWritableViews.h"

// Adapter from the existing role recovery owner's exact ranges to the fixed
// Haiku BAR views. The snapshot path requires held/quiet and RAM proofs.
// The dedicated staging witness instead uses bounded observations and
// selected-tile rechecks; it does not establish firmware write exclusion.
// Reset and stop writes require separate, currently unavailable proofs.
template<class IO,class Scope,class Views> class PioneerQwen35RoleFleetPciIO {
    IO& fIO;
    Scope& fScope;
    Views& fViews;
    PioneerQwen35RoleRouteGuard<IO,Scope> fGuard;
    class ImageReadGuard {
        PioneerQwen35RoleFleetPciIO& fOwner;
    public:
        explicit ImageReadGuard(PioneerQwen35RoleFleetPciIO& owner):fOwner(owner) {}
        bool ProvePinnedBar(uint64_t start,uint64_t end) { return fOwner.fGuard.ProvePinnedBar(start,end); }
        bool ProveSelectedRoute(unsigned core) { return fOwner.fGuard.ProveSelectedRoute(core); }
        bool ProveReadAllowed(unsigned core,unsigned which,unsigned offset) {
            return which<=PioneerQwen35::DataflowRolePhysicalExtraCode
                && PioneerQwen35::DataflowRolePhysicalReadAllowed(which,offset)
                && fOwner.fScope.ProveRoleImageInspectionAllowed(fOwner.fIO,core);
        }
    };
    bool Select(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers && fScope.ProveExclusiveRewrite(fIO)
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            // The dedicated stage uses sampled ownership and rechecks each
            // selected tile. This does not claim firmware write exclusion.
            && (fScope.ProveRoleDedicatedObservedStage(fIO) || fScope.ProveRoleFleetHeldAndQuiet(fIO,core))
#else
            // The held/quiet check requires the requested tile already be
            // selected. Check session authority first, then select it, and
            // only then inspect that tile's reset and NoC state.
            && fScope.CanSelectRoleHeldCore(fIO,core)
#endif
            && fIO.SelectRuntimeCore(core) && fIO.MapQ38Panel() && fIO.DataflowPanelMappingStable()
            && fGuard.ProveSelectedRoute(core)
            && PioneerQwen35WithSelectedClock(fIO,core,[]() { return true; })
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            && SelectedHeldAndQuiet(core)
#else
            && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
#endif
            ;
    }
    bool ReadWords(unsigned core,unsigned which,unsigned offset,uint32_t* output,unsigned bytes) {
        using namespace PioneerQwen35;
        if (!output || !bytes || (offset&3) || (bytes&3) || offset>UINT32_MAX-bytes
            || !DataflowRolePhysicalReadAllowed(which,offset)
            || !DataflowRolePhysicalReadAllowed(which,offset+bytes-4)) {
#if defined(PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY) && defined(_KERNEL_MODE)
            if (which==DataflowRolePhysicalBriscRam || which==DataflowRolePhysicalNcriscRam)
                dprintf("P100A_Q35_PRESTACK_READ_FAILURE stage=whitelist core=%u which=%u offset=%u bytes=%u\n",
                    core,which,offset,bytes);
#endif
            return false;
        }
        if (!Select(core)) {
#if defined(PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY) && defined(_KERNEL_MODE)
            if (which==DataflowRolePhysicalBriscRam || which==DataflowRolePhysicalNcriscRam)
                dprintf("P100A_Q35_PRESTACK_READ_FAILURE stage=select core=%u which=%u offset=%u\n",core,which,offset);
#endif
            return false;
        }
        // Image windows use bounded copies that recheck route and authority
        // around every 64 words. A failed copy is never a valid snapshot.
        // RAM and control reads retain their per-word checks.
        if (which<=DataflowRolePhysicalExtraCode)
            return fViews.CopyWords(core,which,offset,output,bytes/4,fGuard);
        for (unsigned n=0;n<bytes/4;n++) {
            const unsigned at=offset+n*4;
#if defined(PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY) && defined(_KERNEL_MODE)
            unsigned readFailure=ReadOkay;
            if (!fViews.Read(core,which,at,output[n],fGuard,&readFailure)) {
#else
            if (!fViews.Read(core,which,at,output[n],fGuard)) {
#endif
#if defined(PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY) && defined(_KERNEL_MODE)
                if (which==DataflowRolePhysicalBriscRam || which==DataflowRolePhysicalNcriscRam) {
                    // These post-failure samples are diagnostic only. They do
                    // not turn an uncertain earlier read into a valid result.
                    const bool bound=fViews.ReadViewBound(which,at);
                    const bool pinned=fGuard.ProvePinnedBar(fIO.profile.bar0_host_start,fIO.profile.bar0_host_end);
                    const bool route=fGuard.ProveSelectedRoute(core);
                    const bool allowed=fGuard.ProveReadAllowed(core,which,at);
                    dprintf("P100A_Q35_PRESTACK_READ_FAILURE stage=view core=%u which=%u offset=%u step=%u issued=%u bound=%u pinned=%u route=%u allowed=%u\n",
                        core,which,at,readFailure,unsigned(readFailure>=ReadPostAuthority),
                        unsigned(bound),unsigned(pinned),unsigned(route),unsigned(allowed));
                }
#endif
                return false;
            }
        }
        return true;
    }
    bool WriteWords(unsigned core,unsigned which,unsigned offset,const uint32_t* input,unsigned bytes) {
        return input && PioneerQwen35::DataflowRolePhysicalWriteRange(which,offset,bytes)
            && Select(core) && fViews.WriteWords(core,which,offset,input,bytes,fGuard);
    }
    // A selected-tile status scan is necessary but not sufficient: Scope
    // must separately prove no fleet-wide/incoming traffic can start.
    bool SelectedHeldAndQuiet(unsigned core) {
        using namespace PioneerQwen35;
        for (unsigned pass=0;pass<2;pass++) {
            if (!PioneerQwen35SampleNocFifosAndClock([&](unsigned offset) { return fIO.ResidentNocRead(offset); })
                || fIO.ResetRead()!=0x47800 || fIO.ResetRead()!=0x47800) return false;
            for (unsigned noc=0;noc<2;noc++) {
                const unsigned base=noc?0x10000:0;
                uint32_t first=0,second=0;
                if (!DataflowNocDecodeLocalRoute(fIO.ResidentNocRead(base+DataflowNocLogicalIdOffset),first)
                    || !DataflowNocDecodeLocalRoute(fIO.ResidentNocRead(base+DataflowNocLogicalIdOffset),second)
                    || first!=second || !DataflowNocRouteMatchesWorker(first,core)) return false;
                for (unsigned i=0;i<4;i++)
                    if (fIO.ResidentNocRead(base+0x40+i*0x800)) return false;
                for (unsigned i=0;i<32;i++)
                    if (fIO.ResidentNocRead(base+0x240+i*4)&0xffU) return false;
            }
            if (!PioneerQwen35SampleNocFifosAndClock([&](unsigned offset) { return fIO.ResidentNocRead(offset); })
                || fIO.ResetRead()!=0x47800 || !fGuard.ProveSelectedRoute(core)) return false;
            fIO.Pause();
        }
        return true;
    }
    bool SelectForStop(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers && fScope.ProveExclusiveRewrite(fIO)
            && fIO.SelectRuntimeCore(core) && fIO.MapQ38Panel()
            && fIO.DataflowPanelMappingStable() && fGuard.ProveSelectedRoute(core)
            && PioneerQwen35WithSelectedClock(fIO,core,[&]() {
                return fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
            });
    }
    bool ReadStableRoleMailbox(unsigned core,PioneerQwen35::DataflowRoleMailbox& output) {
        PioneerQwen35::DataflowRoleMailbox first{},second{};
        if (!fIO.ReadCoreMemory(core,PioneerQwen35::DataflowRoleMailboxAddress,&first,sizeof(first))) return false;
        fIO.Pause();
        if (!fIO.ReadCoreMemory(core,PioneerQwen35::DataflowRoleMailboxAddress,&second,sizeof(second))
            || memcmp(&first,&second,sizeof(first)) || !fGuard.ProveSelectedRoute(core)) return false;
        memcpy(&output,&second,sizeof(output)); return true;
    }
    bool ReadStableStackHandoff(unsigned core,PioneerQwen35::DataflowRoleSnapshotHandoff& output) {
        PioneerQwen35::DataflowRoleSnapshotHandoff first{},second{};
        if (!fIO.ReadCoreMemory(core,PioneerQwen35::DataflowRoleSnapshotHandoffAddress,&first,sizeof(first))) return false;
        fIO.Pause();
        if (!fIO.ReadCoreMemory(core,PioneerQwen35::DataflowRoleSnapshotHandoffAddress,&second,sizeof(second))
            || memcmp(&first,&second,sizeof(first)) || !fGuard.ProveSelectedRoute(core)) return false;
        output=second; return true;
    }
    bool ReadStableWorkerMailbox(unsigned core,PioneerQwen35::DataflowWorkerMailbox& output) {
        PioneerQwen35::DataflowWorkerMailbox first{},second{};
        if (!fIO.ReadCoreMemory(core,PioneerQwen35::DataflowWorkerMailboxAddress,&first,sizeof(first))) return false;
        fIO.Pause();
        if (!fIO.ReadCoreMemory(core,PioneerQwen35::DataflowWorkerMailboxAddress,&second,sizeof(second))
            || memcmp(&first,&second,sizeof(first)) || !fGuard.ProveSelectedRoute(core)) return false;
        memcpy(&output,&second,sizeof(output)); return true;
    }
    bool QueuesSettled(unsigned core,bool requireZero,bool allowFailures=false,
        bool allowPending=false) {
        using namespace PioneerQwen35;
        DataflowQueueControl first[DataflowCircularBufferCount]{},second[DataflowCircularBufferCount]{};
        if (!fIO.ReadCoreMemory(core,DataflowQueueStateAddress,first,sizeof(first))) return false;
        fIO.Pause();
        if (!fIO.ReadCoreMemory(core,DataflowQueueStateAddress,second,sizeof(second))
            || memcmp(first,second,sizeof(first)) || !fGuard.ProveSelectedRoute(core)) return false;
        for (const auto& queue:second)
            if ((queue.produced!=queue.consumed && (!allowPending
                    || queue.produced-queue.consumed>DataflowCircularBufferPages))
                || (requireZero && queue.produced)
                || (!allowFailures && DataflowQueueFailure(queue))
                || queue.reserved[0] || queue.reserved[1]) return false;
        return true;
    }
    bool QueuesCleared(unsigned core) { return QueuesSettled(core,true); }
#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) || defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
    bool CleanStopWorkerIdle(unsigned core) {
        PioneerQwen35::DataflowWorkerMailbox worker{};
        return ReadStableWorkerMailbox(core,worker)
            && DataflowPciMailboxReadyForStop(worker,core) && !worker.shutdownSequence
            && QueuesCleared(core) && fGuard.ProveSelectedRoute(core);
    }
#endif
    bool RuntimeInspectionAllowed(unsigned core) {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return fScope.CanInspectDedicatedRuntime(fIO,core);
#else
        return fScope.ProveRoleRuntimeSafe(fIO,core);
#endif
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool SnapshotObservationAllowed(unsigned core) {
        return fScope.CanInspectSnapshotRuntime(fIO,core);
    }
#endif
public:
    PioneerQwen35RoleFleetPciIO(IO& io,Scope& scope,Views& views)
        :fIO(io),fScope(scope),fViews(views),fGuard(io,scope) {}
    bool HeldImageStageAllowed() {
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) \
    && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
        return fScope.ProveRoleHeldImageMutationAllowed(fIO,false);
#else
        return false;
#endif
    }
    bool HeldImageRollbackAllowed() {
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) \
    && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
        return fScope.ProveRoleHeldImageMutationAllowed(fIO,true);
#else
        return false;
#endif
    }
    bool HeldImageWritesConfirmed() const { return fViews.ImageWritesConfirmed(); }
    bool HeldPreservationRollbackAllowed() {
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) \
    && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
        return fScope.ProveRoleHeldControlRollbackAllowed(fIO);
#else
        return false;
#endif
    }
    bool HeldPreservationWritesConfirmed() const { return fViews.ControlWritesConfirmed(); }
    bool HeldPreservationStageAllowed() {
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) \
    && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
        return fScope.ProveRoleHeldControlStageAllowed(fIO);
#else
        return false;
#endif
    }
    // The caller owns the fleet release ledger. Consume this tile's attempt
    // before the MMIO write: an unconfirmed write can still have succeeded.
    // The independent scope proof must cover both partially released fleets
    // and role-aware cleanup; held-only proofs are no longer true after the
    // first release. The real scope denies this operation until that exists.
    bool ReleaseCore(unsigned core,bool& attempted) {
        using namespace PioneerQwen35;
        if (attempted || core>=DataflowBarrierWorkers || !fViews.Opened()
            || !fScope.ProveExclusiveRewrite(fIO) || !fScope.ProveRoleReleaseSafe(fIO,core)
            || !fIO.SelectRuntimeCore(core) || !fIO.MapQ38Panel()
            || !fIO.DataflowPanelMappingStable() || !fGuard.ProveSelectedRoute(core)
            || !PioneerQwen35WithSelectedClock(fIO,core,[]() { return true; })
            || fIO.ResetRead()!=0x47800 || fIO.ResetRead()!=0x47800) return false;
        uint32_t first=0,second=0;
        if (!fViews.Read(core,DataflowRolePhysicalControl,0x224,first,fGuard)
            || !fViews.Read(core,DataflowRolePhysicalControl,0x224,second,fGuard)
            || first!=second || (first&DataflowSharedPreserveResetMask)!=DataflowSharedPreserveResetMask
            || !fScope.ProveRoleReleaseSafe(fIO,core) || !fGuard.ProveSelectedRoute(core)
            || fIO.ResetRead()!=0x47800 || fIO.ResetRead()!=0x47800) return false;
        attempted=true;
        // Exact transition: release B, NC and T0; leave T1/T2 held and every
        // other reset bit unchanged. Never use a broad Tensix reset here.
        if (!fIO.SharedRoleReleaseReset()) return false;
        fIO.Pause();
        return fScope.ProveExclusiveRewrite(fIO) && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    // Compile-only dedicated release uses observed admission, not a physical
    // proof of ownership, partial-fleet cleanup or reset recovery.
    bool ReleaseDedicatedCore(unsigned core,bool& attempted) {
        using namespace PioneerQwen35;
        if (attempted || core>=DataflowBarrierWorkers || !fViews.Opened()
            || !fScope.ProveExclusiveRewrite(fIO) || !fScope.CanAttemptDedicatedObservedRelease(fIO,core)
            || !fIO.SelectRuntimeCore(core) || !fIO.MapQ38Panel()
            || !fIO.DataflowPanelMappingStable() || !fGuard.ProveSelectedRoute(core)
            || !PioneerQwen35WithSelectedClock(fIO,core,[&]() {
                return fIO.ResetRead()==0x47800 && fIO.ResetRead()==0x47800;
            })
            || !fScope.CanAttemptDedicatedObservedRelease(fIO,core)
            || !fGuard.ProveSelectedRoute(core)) return false;
        // The authorization check can take time. Re-sample the clock before
        // touching reset, and again before readback after the attempted write.
        // These checks do not exclude a concurrent external clock change.
        return PioneerQwen35WithSelectedClock(fIO,core,[&]() {
            if (fIO.ResetRead()!=0x47800 || fIO.ResetRead()!=0x47800
                || !fGuard.ProveSelectedRoute(core)) return false;
            attempted=true;
            if (!fIO.SharedRoleReleaseReset()) return false;
            fIO.Pause();
            return fScope.ProveExclusiveRewrite(fIO) && fGuard.ProveSelectedRoute(core)
                && PioneerQwen35WithSelectedClock(fIO,core,[&]() {
                    return fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
                });
        });
    }
#endif
    // Only an unconfirmed release may be classified as never released. The
    // independent proof must fence the attempted write: two held reads alone
    // cannot exclude a posted write arriving after the reads.
    bool UnconfirmedReleaseHeld(unsigned core) {
        if (core>=PioneerQwen35::DataflowBarrierWorkers || !fScope.ProveExclusiveRewrite(fIO)
            || !fScope.ProveRoleReleaseWriteDrained(fIO,core) || !fIO.SelectRuntimeCore(core)
            || !fIO.MapQ38Panel() || !fIO.DataflowPanelMappingStable()
            || !fGuard.ProveSelectedRoute(core)
            || !PioneerQwen35WithSelectedClock(fIO,core,[]() { return true; })) return false;
        if (fIO.ResetRead()!=0x47800) return false;
        fIO.Pause();
        return fIO.ResetRead()==0x47800 && fGuard.ProveSelectedRoute(core)
            && fScope.ProveRoleReleaseWriteDrained(fIO,core);
    }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    bool UnconfirmedReleaseRunning(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers && fScope.ProveExclusiveRewrite(fIO)
            && fScope.ProveRoleReleaseWriteDrained(fIO,core) && SelectForStop(core)
            && fScope.ProveRoleReleaseWriteDrained(fIO,core) && fGuard.ProveSelectedRoute(core);
    }
    void Pause() { fIO.Pause(); }
#endif
    enum StackReleaseObservation { StackReleaseNotObserved, StackReleaseObserved };
    // This is evidence for recovery only, not authorization to issue a
    // release. Two reads do not themselves drain an earlier posted write.
    bool ObserveDrainedStackRelease(unsigned core,StackReleaseObservation& observation) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !fScope.ProveExclusiveRewrite(fIO)
            || !fScope.ProveRoleStackReleaseWriteDrained(fIO,core) || !SelectForStop(core)) return false;
        DataflowRoleSnapshotHandoff snapshot{};
        if (!ReadStableStackHandoff(core,snapshot)
            || snapshot.readerReady!=DataflowRoleSnapshotReaderReady
            || snapshot.writerReady!=DataflowRoleSnapshotWriterReady
            || (snapshot.release && snapshot.release!=DataflowRoleSnapshotRelease)
            || !fScope.ProveRoleStackReleaseWriteDrained(fIO,core) || !SelectForStop(core)) return false;
        observation=snapshot.release?StackReleaseObserved:StackReleaseNotObserved;
        return true;
    }
    // Called only if no stack-release write was attempted, or if that exact
    // attempt was independently drained and the release word remained zero.
    // Consume the cancellation before the write; a posted write may arrive
    // even if readback fails, so retries may observe but must not reissue it.
    bool RequestStackCancel(unsigned core,bool& attempted,bool releaseWasAttempted=false) {
        using namespace PioneerQwen35;
        if (attempted || core>=DataflowBarrierWorkers || !fScope.ProveExclusiveRewrite(fIO)
            || (releaseWasAttempted && !fScope.ProveRoleStackReleaseWriteDrained(fIO,core))
            || !fScope.ProveRoleStackCancelSafe(fIO,core) || !SelectForStop(core)) return false;
        DataflowRoleSnapshotHandoff snapshot{};
        if (!ReadStableStackHandoff(core,snapshot)
            || snapshot.readerReady!=DataflowRoleSnapshotReaderReady
            || snapshot.writerReady!=DataflowRoleSnapshotWriterReady || snapshot.release
            || (releaseWasAttempted && !fScope.ProveRoleStackReleaseWriteDrained(fIO,core))
            || !fScope.ProveRoleStackCancelSafe(fIO,core) || !fGuard.ProveSelectedRoute(core)
            || fIO.ResetRead()!=0x6000 || fIO.ResetRead()!=0x6000) return false;
        attempted=true;
        const unsigned address=DataflowRoleSnapshotHandoffAddress+offsetof(DataflowRoleSnapshotHandoff,release);
        fIO.Q38PanelWrite(address,DataflowRoleSnapshotCancel);
        return fGuard.ProveSelectedRoute(core) && fIO.Q38PanelRead(address)==DataflowRoleSnapshotCancel
            && fGuard.ProveSelectedRoute(core);
    }
    // A partial fleet start can reach cleanup while a shim is still copying
    // its private RAM. Wait only while both handoff fields have their initial
    // or ready values and the same scope and route remain valid. Once both
    // are ready, RequestStackCancel consumes the sole write attempt. A lost
    // write receipt is returned to the caller and is never reissued here.
    bool WaitAndRequestStackCancel(unsigned core,bool& attempted,bool releaseWasAttempted,unsigned polls) {
        using namespace PioneerQwen35;
        if (attempted || core>=DataflowBarrierWorkers || !polls || polls>65536) return false;
        for (unsigned n=0;n<polls;n++) {
            if (!fScope.ProveExclusiveRewrite(fIO)
                || (releaseWasAttempted && !fScope.ProveRoleStackReleaseWriteDrained(fIO,core))
                || !fScope.ProveRoleStackCancelSafe(fIO,core) || !SelectForStop(core)) return false;
            DataflowRoleSnapshotHandoff handoff{};
            if (!ReadStableStackHandoff(core,handoff) || handoff.release
                || (handoff.readerReady && handoff.readerReady!=DataflowRoleSnapshotReaderReady)
                || (handoff.writerReady && handoff.writerReady!=DataflowRoleSnapshotWriterReady)
                || !fScope.ProveRoleStackCancelSafe(fIO,core)
                || (releaseWasAttempted && !fScope.ProveRoleStackReleaseWriteDrained(fIO,core))
                || !fGuard.ProveSelectedRoute(core) || fIO.ResetRead()!=0x6000
                || fIO.ResetRead()!=0x6000) return false;
            if (handoff.readerReady==DataflowRoleSnapshotReaderReady
                && handoff.writerReady==DataflowRoleSnapshotWriterReady)
                return RequestStackCancel(core,attempted,releaseWasAttempted);
            fIO.Pause();
        }
        return false;
    }
    // The cancellation receipts are from the stackless shims, not normal
    // role stop receipts. Speculative PC samples are supplemental only: the
    // independent scope proof must exclude writers and in-flight traffic.
    bool StackCancelledAndParked(unsigned core,uint32_t readerPark,uint32_t writerPark) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || (readerPark&3) || (writerPark&3)
            || readerPark<DataflowRoleReaderShimAddress || readerPark>=DataflowRoleReaderShimLimit
            || writerPark<DataflowRoleWriterShimAddress || writerPark>=DataflowRoleWriterShimLimit
            || !fScope.ProveRoleStackCancelledSafe(fIO,core) || !SelectForStop(core)) return false;
        DataflowRoleSnapshotHandoff first{},second{};
        if (!ReadStableStackHandoff(core,first) || first.release!=DataflowRoleSnapshotCancel
            || first.readerReady!=DataflowRoleSnapshotReaderCancelled
            || first.writerReady!=DataflowRoleSnapshotWriterCancelled) return false;
        const uint32_t reader1=fIO.SharedRolePcRead(DataflowRoleReader);
        const uint32_t writer1=fIO.SharedRolePcRead(DataflowRoleWriter);
        fIO.Pause();
        const uint32_t reader2=fIO.SharedRolePcRead(DataflowRoleReader);
        const uint32_t writer2=fIO.SharedRolePcRead(DataflowRoleWriter);
        return reader1==readerPark && reader2==readerPark && writer1==writerPark && writer2==writerPark
            && ReadStableStackHandoff(core,second) && !memcmp(&first,&second,sizeof(first))
            && fScope.ProveRoleStackCancelledSafe(fIO,core) && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
    // This issues exactly one host-owned mailbox word, never a command or a
    // reset. A write failure may still have reached the tile; the caller
    // retains the attempt and token and may only observe, never reissue.
    bool RequestStop(unsigned core,bool fault,bool& attempted,uint32_t& token) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || attempted || token || !fScope.ProveExclusiveRewrite(fIO)
            || !fScope.ProveRoleStopSafe(fIO,core,fault) || !SelectForStop(core)) return false;
#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) || defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
        if (!fault && !CleanStopWorkerIdle(core)) return false;
#endif
        DataflowRoleMailbox snapshot{};
        if (!ReadStableRoleMailbox(core,snapshot) || !DataflowRoleMailboxAllStarted(snapshot)) return false;
        const bool valid=fault?DataflowRoleMailboxRequestFaultStop(snapshot,true,true,true)
            :DataflowRoleMailboxRequestStop(snapshot);
        if (!valid || !fScope.ProveRoleStopSafe(fIO,core,fault)
#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) || defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
            || (!fault && !CleanStopWorkerIdle(core))
#endif
            || !fGuard.ProveSelectedRoute(core)
            || fIO.ResetRead()!=0x6000 || fIO.ResetRead()!=0x6000) return false;
        token=DataflowLoadAcquire(snapshot.stopRequest);
        if (!token) return false;
        attempted=true;
        const unsigned address=DataflowRoleMailboxAddress+offsetof(DataflowRoleMailbox,stopRequest);
        fIO.Q38PanelWrite(address,token);
        return fGuard.ProveSelectedRoute(core) && fIO.Q38PanelRead(address)==token
            && fGuard.ProveSelectedRoute(core);
    }
    bool StopRequestObserved(unsigned core,uint32_t token) {
        using namespace PioneerQwen35;
        if (!token || !SelectForStop(core)) return false;
        DataflowRoleMailbox snapshot{};
        if (!ReadStableRoleMailbox(core,snapshot) || !DataflowRoleMailboxAllStarted(snapshot)
            || DataflowLoadAcquire(snapshot.stopRequest)!=token) return false;
        return DataflowRoleMailboxStopStateValid(snapshot,DataflowLoadAcquire(snapshot.acknowledged),
            DataflowLoadAcquire(snapshot.command),token);
    }
    // Receipts precede the role shims' final fence and park loop. Verify both
    // receipts and two PC samples, then separately require the scope's fleet
    // and incoming-NoC proof. Speculative PC snapshots alone are not proof.
    bool RolesStoppedAndParked(unsigned core,uint32_t token,uint32_t readerPark,uint32_t writerPark) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !token || (readerPark&3) || (writerPark&3)
            || readerPark<DataflowRoleReaderShimAddress || readerPark>=DataflowRoleReaderShimLimit
            || writerPark<DataflowRoleWriterShimAddress || writerPark>=DataflowRoleWriterShimLimit
            || !fScope.ProveRolePostStopSafe(fIO,core) || !SelectForStop(core)) return false;
        DataflowRoleMailbox first{},second{};
        if (!ReadStableRoleMailbox(core,first) || !DataflowRoleMailboxAllStarted(first)
            || DataflowLoadAcquire(first.stopRequest)!=token || !DataflowRoleMailboxStopReceipts(first)) return false;
        const uint32_t reader1=fIO.SharedRolePcRead(DataflowRoleReader);
        const uint32_t writer1=fIO.SharedRolePcRead(DataflowRoleWriter);
        fIO.Pause();
        const uint32_t reader2=fIO.SharedRolePcRead(DataflowRoleReader);
        const uint32_t writer2=fIO.SharedRolePcRead(DataflowRoleWriter);
        return reader1==readerPark && reader2==readerPark && writer1==writerPark && writer2==writerPark
            && ReadStableRoleMailbox(core,second) && !memcmp(&first,&second,sizeof(first))
            && fScope.ProveRolePostStopSafe(fIO,core) && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // The stop-to-restore image parks in its code tail, not in the startup
    // shim. Both fresh restore receipts and both role stop receipts must be
    // stable around two matching parked-PC observations. The scope must
    // independently protect the source, images, reset and all writers.
    bool RolesAutoRestoredAndParked(unsigned core,uint32_t token,uint32_t readerPark,uint32_t writerPark) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !token || (readerPark&3) || (writerPark&3)
            || readerPark<DataflowRoleReaderCodeAddress || readerPark>=DataflowRoleReaderCodeLimit
            || writerPark<DataflowRoleWriterCodeAddress || writerPark>=DataflowRoleWriterCodeLimit
            || !fScope.ProveRolePostStopSafe(fIO,core)
            || !fScope.ProveRoleAutoRestoreCompletionSafe(fIO,core) || !SelectForStop(core)) return false;
        DataflowRoleMailbox first{},second{};
        DataflowRoleSnapshotHandoff handoff{},after{};
        if (!ReadStableRoleMailbox(core,first) || !DataflowRoleMailboxAllStarted(first)
            || DataflowLoadAcquire(first.stopRequest)!=token || !DataflowRoleMailboxStopReceipts(first)
            || !ReadStableStackHandoff(core,handoff)
            || handoff.readerReady!=PIONEER_Q35_ROLE_RESTORE_READER_DONE
            || handoff.writerReady!=PIONEER_Q35_ROLE_RESTORE_WRITER_DONE
            || handoff.release!=DataflowRoleSnapshotRelease) return false;
        const uint32_t reader1=fIO.SharedRolePcRead(DataflowRoleReader);
        const uint32_t writer1=fIO.SharedRolePcRead(DataflowRoleWriter);
        fIO.Pause();
        const uint32_t reader2=fIO.SharedRolePcRead(DataflowRoleReader);
        const uint32_t writer2=fIO.SharedRolePcRead(DataflowRoleWriter);
        return reader1==readerPark && reader2==readerPark && writer1==writerPark && writer2==writerPark
            && ReadStableRoleMailbox(core,second) && !memcmp(&first,&second,sizeof(first))
            && ReadStableStackHandoff(core,after) && !memcmp(&handoff,&after,sizeof(handoff))
            && fScope.ProveRolePostStopSafe(fIO,core)
            && fScope.ProveRoleAutoRestoreCompletionSafe(fIO,core) && fGuard.ProveSelectedRoute(core) && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
#endif
    bool RequestComputeStop(unsigned core,bool& attempted) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || attempted || !fScope.ProveExclusiveRewrite(fIO)
            || !fScope.ProveComputeStopSafe(fIO,core) || !SelectForStop(core)) return false;
        DataflowWorkerMailbox box{};
        if (!ReadStableWorkerMailbox(core,box) || !DataflowPciMailboxReadyForStop(box,core)
            || box.shutdownSequence || !fScope.ProveComputeStopSafe(fIO,core)
            || !fGuard.ProveSelectedRoute(core) || fIO.ResetRead()!=0x6000) return false;
        attempted=true;
        const unsigned address=DataflowWorkerMailboxAddress+offsetof(DataflowWorkerMailbox,shutdownSequence);
        fIO.Q38PanelWrite(address,DataflowWorkerShutdownRequest);
        return fGuard.ProveSelectedRoute(core) && fIO.Q38PanelRead(address)==DataflowWorkerShutdownRequest
            && fGuard.ProveSelectedRoute(core);
    }
    bool ComputeStopObserved(unsigned core) {
        using namespace PioneerQwen35;
        if (!SelectForStop(core)) return false;
        DataflowWorkerMailbox box{};
        return ReadStableWorkerMailbox(core,box) && box.shutdownSequence==DataflowWorkerShutdownRequest
            && (DataflowPciMailboxReadyForStop(box,core) || DataflowPciMailboxStopped(box,core));
    }
    bool ComputeStoppedAndParked(unsigned core,uint32_t park) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || (park&3) || park<DataflowServiceEntryAddress
            || park>=DataflowServiceTextLimit || !fScope.ProveComputePostStopSafe(fIO,core)
            || !SelectForStop(core)) return false;
        DataflowWorkerMailbox first{},second{};
        if (!ReadStableWorkerMailbox(core,first) || !DataflowPciMailboxStopped(first,core)
            || !QueuesCleared(core)) return false;
        const uint32_t pc1=fIO.RuntimePcRead(); fIO.Pause(); const uint32_t pc2=fIO.RuntimePcRead();
        return pc1==park && pc2==park && ReadStableWorkerMailbox(core,second)
            && !memcmp(&first,&second,sizeof(first)) && QueuesCleared(core)
            && fScope.ProveComputePostStopSafe(fIO,core) && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
    bool HoldCore(unsigned core,bool& attempted) {
        if (core>=PioneerQwen35::DataflowBarrierWorkers || attempted
            || !fScope.ProveExclusiveRewrite(fIO) || !fScope.ProveRoleHoldSafe(fIO,core)
            || !SelectForStop(core) || !fScope.ProveRoleHoldSafe(fIO,core)
            || !fGuard.ProveSelectedRoute(core)) return false;
        attempted=true;
        if (!fIO.SharedRoleHoldReset()) return false;
        fIO.Pause();
        return fGuard.ProveSelectedRoute(core) && fIO.ResetRead()==0x47800 && fIO.ResetRead()==0x47800;
    }
    bool HoldObserved(unsigned core) {
        if (core>=PioneerQwen35::DataflowBarrierWorkers || !fScope.ProveExclusiveRewrite(fIO)
            || !fScope.ProveRoleHeldAfterAttempt(fIO,core) || !fIO.SelectRuntimeCore(core)
            || !fIO.MapQ38Panel() || !fIO.DataflowPanelMappingStable() || !fGuard.ProveSelectedRoute(core)
            || !PioneerQwen35WithSelectedClock(fIO,core,[]() { return true; })) return false;
        return fIO.ResetRead()==0x47800 && fIO.ResetRead()==0x47800
            && fScope.ProveRoleHeldAfterAttempt(fIO,core) && fGuard.ProveSelectedRoute(core);
    }
private:
    bool RunningReadySample(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& expected,
        unsigned textWords,unsigned dispatchWords,bool observation) {
        using namespace PioneerQwen35;
        auto allowed=[&]() {
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
            if (observation) return SnapshotObservationAllowed(core);
#else
            (void)observation;
#endif
            return RuntimeInspectionAllowed(core);
        };
        if (core>=DataflowBarrierWorkers || !DataflowWorkerSnapshotReady(expected)
            || !allowed() || !SelectForStop(core)) return false;
        DataflowWorkerMailbox worker{};
        DataflowWorkerSnapshot observed{};
        DataflowRoleMailbox roles{};
        if (!fIO.ReadWorkerSnapshot(core,observed) || memcmp(&expected,&observed,sizeof(expected))
            || !ReadStableWorkerMailbox(core,worker) || !DataflowPciMailboxMatchesReadySnapshot(worker,core,expected)
            || !ReadStableRoleMailbox(core,roles) || !DataflowRoleMailboxIdle(roles)
            || !QueuesCleared(core)) return false;
        const uint32_t pc1=fIO.RuntimePcRead(); fIO.Pause(); const uint32_t pc2=fIO.RuntimePcRead();
        return DataflowServicePcInImage(pc1,textWords,dispatchWords)
            && DataflowServicePcInImage(pc2,textWords,dispatchWords)
            && allowed() && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
public:
    bool RunningReady(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& expected,
        unsigned textWords,unsigned dispatchWords) {
        return RunningReadySample(core,expected,textWords,dispatchWords,false);
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool InspectSnapshotRunningReady(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& expected,
        unsigned textWords,unsigned dispatchWords) {
        return RunningReadySample(core,expected,textWords,dispatchWords,true);
    }
#endif
    bool RuntimeStable(unsigned core,unsigned textWords,unsigned dispatchWords) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !RuntimeInspectionAllowed(core)
            || !SelectForStop(core)) return false;
        const uint32_t pc1=fIO.RuntimePcRead(); fIO.Pause(); const uint32_t pc2=fIO.RuntimePcRead();
        return DataflowServicePcInImage(pc1,textWords,dispatchWords)
            && DataflowServicePcInImage(pc2,textWords,dispatchWords)
            && RuntimeInspectionAllowed(core) && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
    // Two snapshots of each B/NC PC must be in its own exported image and
    // outside the stop/failure park loop. A speculative PC is only a sample,
    // never a substitute for the independent runtime/NoC/ownership proof.
private:
    bool RunningRolePcsSample(unsigned core,const PioneerQwen35::DataflowRoleProgramBounds& bounds,
        bool fault,bool observation) {
        using namespace PioneerQwen35;
        auto allowed=[&]() {
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
            if (observation) return !fault && SnapshotObservationAllowed(core);
#else
            (void)observation;
#endif
            return fault?fScope.ProveRoleFaultAuditSafe(fIO,core):RuntimeInspectionAllowed(core);
        };
        if (core>=DataflowBarrierWorkers || !bounds.Valid() || !allowed() || !SelectForStop(core)) return false;
        uint32_t first[DataflowRoleCount]{},second[DataflowRoleCount]{};
        for (unsigned role=0;role<DataflowRoleCount;role++) first[role]=fIO.SharedRolePcRead(role);
        fIO.Pause();
        for (unsigned role=0;role<DataflowRoleCount;role++) second[role]=fIO.SharedRolePcRead(role);
        for (unsigned role=0;role<DataflowRoleCount;role++)
            if (!bounds.ContainsRunning(role,first[role]) || !bounds.ContainsRunning(role,second[role]))
                return false;
        return allowed() && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
public:
    bool RunningRolePcs(unsigned core,const PioneerQwen35::DataflowRoleProgramBounds& bounds,bool fault) {
        return RunningRolePcsSample(core,bounds,fault,false);
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool InspectSnapshotRunningRolePcs(unsigned core,const PioneerQwen35::DataflowRoleProgramBounds& bounds) {
        return RunningRolePcsSample(core,bounds,false,true);
    }
#endif
    // Compare the actual B/NC boot words, shims and text against the exported
    // images twice while the selected running tile and route remain owned.
    // This is a startup observation, not proof against a later writer.
    bool InspectRunningRoleImages(unsigned core,const PioneerQwen35::DataflowRoleProgramImage& image) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !image.Valid() || !fViews.Opened()
            || !fScope.ProveExclusiveRewrite(fIO) || !SelectForStop(core)
            || !fScope.ProveRoleImageInspectionAllowed(fIO,core)) return false;
        ImageReadGuard guard(*this);
        for (unsigned pass=0;pass<2;pass++) {
            for (unsigned role=0;role<DataflowRoleCount;role++) {
                const unsigned shim=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
                const unsigned code=role==DataflowRoleReader?DataflowRoleReaderCodeAddress:DataflowRoleWriterCodeAddress;
                const unsigned bootWindow=role==DataflowRoleReader?DataflowRolePhysicalBriscBoot:DataflowRolePhysicalNcriscBoot;
                uint32_t boot=0;
                if (!DataflowRoleBootWord(role,shim,boot)
                    || !fViews.Matches(core,bootWindow,0,&boot,1,guard)
                    || !fViews.Matches(core,DataflowRolePhysicalShim,shim-DataflowRoleReaderShimAddress,
                        image.shim[role],image.bounds.shimWords[role],guard)
                    || !fViews.Matches(core,DataflowRolePhysicalExtraCode,code-DataflowRoleExtraL1Address,
                        image.code[role],image.bounds.codeWords[role],guard)) return false;
            }
            fIO.Pause();
        }
        return fScope.ProveRoleImageInspectionAllowed(fIO,core) && SelectForStop(core)
            && fScope.ProveRoleImageInspectionAllowed(fIO,core);
    }
    // Read the two L1 copies made by the startup shims before their first
    // private-stack store. This compares against snapshots already retained
    // by the owner; it does not replace the still-missing physical timing,
    // ownership, release or recovery proofs.
    bool InspectRunningRoleRamCopies(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !reader || !writer || !fViews.Opened()
            || !fScope.ProveExclusiveRewrite(fIO) || !SelectForStop(core)
            || !fScope.ProveRoleImageInspectionAllowed(fIO,core)) return false;
        ImageReadGuard guard(*this);
        for (unsigned pass=0;pass<2;pass++) {
            if (!fViews.Matches(core,DataflowRolePhysicalExtraCode,
                    DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address,reader,DataflowRoleLocalRamWords,guard)
                || !fViews.Matches(core,DataflowRolePhysicalExtraCode,
                    DataflowRoleWriterRamCopy-DataflowRoleExtraL1Address,writer,DataflowRoleLocalRamWords,guard)) return false;
            fIO.Pause();
        }
        return fScope.ProveRoleImageInspectionAllowed(fIO,core) && SelectForStop(core)
            && fScope.ProveRoleImageInspectionAllowed(fIO,core);
    }
    bool StartupHandoffWriteAllowed(unsigned core) {
        using namespace PioneerQwen35;
        if (!fScope.ProveRoleStartupHandoffSafe(fIO,core) || !SelectForStop(core)) return false;
        DataflowRoleSnapshotHandoff first{},second{};
        if (!fIO.ReadCoreMemory(core,DataflowRoleSnapshotHandoffAddress,&first,sizeof(first))) return false;
        fIO.Pause();
        return fIO.ReadCoreMemory(core,DataflowRoleSnapshotHandoffAddress,&second,sizeof(second))
            && !memcmp(&first,&second,sizeof(first))
            && second.readerReady==DataflowRoleSnapshotReaderReady
            && second.writerReady==DataflowRoleSnapshotWriterReady && !second.release
            && fScope.ProveRoleStartupHandoffSafe(fIO,core) && SelectForStop(core);
    }
    // Read role-local snapshots only through their owned L1 copy windows.
    // The real scope does not yet authorize this as an original snapshot.
    bool StacklessCopiesSafe(unsigned core) {
        return fScope.ProveRoleStacklessSnapshotSafe(fIO,core)
            && StartupHandoffWriteAllowed(core)
            && fScope.ProveRoleImageInspectionAllowed(fIO,core)
            && fScope.ProveRoleStacklessSnapshotSafe(fIO,core);
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // These checks admit no hardware unless Scope establishes lifetime
    // ownership and freshness independently of the sampled words.
    bool AutoRestoreArmSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return reader && writer && StacklessCopiesSafe(core)
            && fScope.ProveRoleAutoRestoreArmSafe(fIO,core,reader,writer)
            && InspectRunningRoleRamCopies(core,reader,writer)
            && StacklessCopiesSafe(core)
            && fScope.ProveRoleAutoRestoreArmSafe(fIO,core,reader,writer);
    }
    bool AutoRestoreHeldSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return StacklessHeldCopySourceSafe(core,reader,writer)
            && fScope.ProveRoleAutoRestoreHeldSafe(fIO,core,reader,writer)
            && AllRolesHeld(core) && DefaultBootRouting(core)
            && fScope.ProveRoleAutoRestoreHeldSafe(fIO,core,reader,writer);
    }
    bool AutoRestoreCancelHeldSafe(unsigned core,const uint32_t* reader,const uint32_t* writer,bool releaseAttempted) {
        return StacklessHeldCopySourceSafe(core,reader,writer)
            && fScope.ProveRoleAutoCancelHeldSafe(fIO,core,reader,writer,releaseAttempted)
            && AllRolesHeld(core) && DefaultBootRouting(core)
            && fScope.ProveRoleAutoCancelHeldSafe(fIO,core,reader,writer,releaseAttempted);
    }
    bool AutoRestoreRetirementSafe(unsigned core,const uint32_t* reader,const uint32_t* writer,bool cancelled) {
        const uint32_t* r=nullptr;const uint32_t* w=nullptr;bool captured=false;
        return reader && writer && RetirementRamSnapshotSources(core,r,w,captured)
            && captured && r==reader && w==writer
            && fScope.ProveRoleAutoRestoreRetirementSafe(fIO,core,reader,writer,cancelled)
            && fGuard.ProveSelectedRoute(core)
            && fScope.ProveRoleAutoRestoreRetirementSafe(fIO,core,reader,writer,cancelled);
    }
#endif
    bool StacklessFleetPreparationSafe(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers
            && fScope.ProveExclusiveRewrite(fIO)
            && fScope.ProveRoleStacklessFleetPreparationSafe(fIO,core);
    }
    bool StacklessUncapturedRamProtected(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers
            && fScope.ProveExclusiveRewrite(fIO)
            && fScope.ProveRoleUncapturedRamProtected(fIO,core);
    }
    // Copy-aware fleet restaging needs an independently protected ledger of
    // original private-RAM snapshots. Scope supplies that proof and the
    // immutable pointers; this adapter additionally checks the held route.
    // The real witness scope currently denies this proof.
    bool StacklessHeldCopySources(unsigned core,const uint32_t*& reader,const uint32_t*& writer,bool& captured) {
        using namespace PioneerQwen35;
        reader=writer=nullptr;captured=false;
        const uint32_t* r=nullptr;const uint32_t* w=nullptr;bool hasCopies=false;
        const uint32_t* r2=nullptr;const uint32_t* w2=nullptr;bool hasCopies2=false;
        if (core>=DataflowBarrierWorkers || !Select(core)
            || !fScope.ProveRoleHeldCopySources(fIO,core,r,w,hasCopies)
            || (hasCopies?(!r || !w):(r || w)) || !fGuard.ProveSelectedRoute(core)
            || !fScope.ProveRoleHeldCopySources(fIO,core,r2,w2,hasCopies2)
            || r!=r2 || w!=w2 || hasCopies!=hasCopies2 || !fGuard.ProveSelectedRoute(core)) return false;
        reader=r;writer=w;captured=hasCopies;return true;
    }
    bool StacklessHeldCopySourceSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        const uint32_t* r=nullptr;const uint32_t* w=nullptr;bool captured=false;
        return reader && writer && StacklessHeldCopySources(core,r,w,captured)
            && captured && reader==r && writer==w;
    }
    // These retained host pointers remain meaningful after L1 copy windows
    // are retired. Scope must bind them to the exact RAM recovery ledger and
    // exclude firmware, resets, misroutes and competing writes. No real
    // Haiku scope currently provides this authority.
    bool RetirementRamSnapshotSources(unsigned core,const uint32_t*& reader,const uint32_t*& writer,bool& captured) {
        using namespace PioneerQwen35;
        reader=writer=nullptr;captured=false;
        const uint32_t* r=nullptr;const uint32_t* w=nullptr;bool has=false;
        const uint32_t* r2=nullptr;const uint32_t* w2=nullptr;bool has2=false;
        if (core>=DataflowBarrierWorkers || !Select(core)
            || !fScope.ProveRoleRetirementRamSources(fIO,core,r,w,has)
            || (has?(!r || !w):(r || w)) || !fGuard.ProveSelectedRoute(core)
            || !fScope.ProveRoleRetirementRamSources(fIO,core,r2,w2,has2)
            || r!=r2 || w!=w2 || has!=has2 || !fGuard.ProveSelectedRoute(core)) return false;
        reader=r;writer=w;captured=has;return true;
    }
    bool FleetRamImageRetirementSafe() {
        return fScope.ProveExclusiveRewrite(fIO) && fScope.ProveRoleFleetImageRetirementSafe(fIO)
            && fScope.ProveRoleFleetImageRetirementSafe(fIO);
    }
    bool FleetRamImageRetirementRetrySafe() {
        return FleetRamImageRetirementSafe() && fScope.ProveRoleFleetImageRetirementRetrySafe(fIO)
            && fScope.ProveRoleFleetImageRetirementRetrySafe(fIO);
    }
    bool CoreRamImageRetirementSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        const uint32_t* r=nullptr;const uint32_t* w=nullptr;bool captured=false;
        return reader && writer && FleetRamImageRetirementSafe()
            && RetirementRamSnapshotSources(core,r,w,captured) && captured && r==reader && w==writer
            && fScope.ProveRoleCoreImageRetirementSafe(fIO,core,reader,writer)
            && fGuard.ProveSelectedRoute(core)
            && fScope.ProveRoleCoreImageRetirementSafe(fIO,core,reader,writer);
    }
    bool CoreRamImageRetirementRetrySafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return FleetRamImageRetirementRetrySafe() && CoreRamImageRetirementSafe(core,reader,writer)
            && fScope.ProveRoleCoreImageRetirementRetrySafe(fIO,core,reader,writer);
    }
    // These fixed handoff operations additionally require lifetime-spanning
    // ownership, firmware exclusion and write-drain proof from Scope. The
    // actual Haiku scope denies each proof; samples alone cannot supply it.
    bool RestoreHandoffHeldSafe(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers
            && fScope.ProveRoleRestoreHandoffHeldSafe(fIO,core) && AllRolesHeld(core)
            && DefaultBootRouting(core) && fScope.ProveRoleRestoreHandoffHeldSafe(fIO,core)
            && fGuard.ProveSelectedRoute(core);
    }
    bool RestoreCompletionSafe(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers
            && fScope.ProveRoleRestoreCompletionSafe(fIO,core)
            && fScope.ProveExclusiveRewrite(fIO) && fIO.SelectRuntimeCore(core)
            && fIO.MapQ38Panel() && fIO.DataflowPanelMappingStable() && fGuard.ProveSelectedRoute(core)
            && PioneerQwen35WithSelectedClock(fIO,core,[&]() {
                return fIO.ResetRead()==0x7000 && fIO.ResetRead()==0x7000;
            }) && fScope.ProveRoleRestoreCompletionSafe(fIO,core) && fGuard.ProveSelectedRoute(core);
    }
    bool RestoreCompletedAndHeld(unsigned core) {
        return RestoreHandoffHeldSafe(core) && fScope.ProveRoleRestoreCompletedAndHeld(fIO,core)
            && RestoreHandoffHeldSafe(core) && fScope.ProveRoleRestoreCompletedAndHeld(fIO,core);
    }
    bool RestoreHandoffRetrySafe(unsigned core) {
        return RestoreHandoffHeldSafe(core) && fScope.ProveRoleRestoreHandoffRetrySafe(fIO,core)
            && RestoreHandoffHeldSafe(core) && fScope.ProveRoleRestoreHandoffRetrySafe(fIO,core);
    }
    bool ReadRestoreHandoff(unsigned core,PioneerQwen35::DataflowRoleSnapshotHandoff& result) {
        const bool held=RestoreHandoffHeldSafe(core);
        if (!held && !RestoreCompletionSafe(core)) return false;
        PioneerQwen35::DataflowRoleSnapshotHandoff observed{};
        if (!ReadStableStackHandoff(core,observed)
            || !(held?RestoreHandoffHeldSafe(core):RestoreCompletionSafe(core))) return false;
        result=observed;return true;
    }
    bool WriteRestoreHandoffWord(unsigned core,unsigned word,uint32_t value) {
        using namespace PioneerQwen35;
        if (word>=3 || !RestoreHandoffHeldSafe(core)) return false;
        const unsigned address=DataflowRoleSnapshotHandoffAddress+word*4;
        fIO.Q38PanelWrite(address,value); // the owner records the attempt first
        return fGuard.ProveSelectedRoute(core) && fIO.Q38PanelRead(address)==value
            && RestoreHandoffHeldSafe(core);
    }
    bool StacklessCoreRestoreReleaseReady(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        PioneerQwen35::DataflowRoleSnapshotHandoff handoff{};
        return StacklessHeldCopySourceSafe(core,reader,writer) && RestoreHandoffHeldSafe(core)
            && fScope.ProveRoleRestoreReleaseReady(fIO,core,reader,writer)
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
            && fScope.ProveRoleRestorePhysicalReleaseSafe(fIO,core)
#endif
            && ReadStableStackHandoff(core,handoff)
            && !handoff.readerReady && !handoff.writerReady && !handoff.release
            && fScope.ProveRoleRestoreReleaseReady(fIO,core,reader,writer)
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
            && fScope.ProveRoleRestorePhysicalReleaseSafe(fIO,core)
#endif
            && StacklessHeldCopySourceSafe(core,reader,writer) && RestoreHandoffHeldSafe(core);
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // The caller consumes its fleet and handoff ledgers before this one-shot
    // write. Scope must independently bind the exact shims, fresh receipt,
    // release ownership and the recoverability of an uncertain write.
    bool ReleaseCoreLocalRestore(unsigned core,bool& attempted) {
        using namespace PioneerQwen35;
        if (attempted || core>=DataflowBarrierWorkers || !fViews.Opened()
            || !fScope.ProveRoleRestorePhysicalReleaseSafe(fIO,core)
            || !RestoreHandoffHeldSafe(core)) return false;
        uint32_t first=0,second=0;DataflowRoleSnapshotHandoff handoff{};
        if (!fViews.Read(core,DataflowRolePhysicalControl,0x224,first,fGuard)
            || !fViews.Read(core,DataflowRolePhysicalControl,0x224,second,fGuard)
            || first!=second || (first&DataflowRolePreserveResetMask)!=DataflowRolePreserveResetMask
            || !ReadStableStackHandoff(core,handoff) || handoff.readerReady || handoff.writerReady || handoff.release
            || !fScope.ProveRoleRestorePhysicalReleaseSafe(fIO,core) || !RestoreHandoffHeldSafe(core)) return false;
        attempted=true;
        if (!fIO.SharedRolePrestackReleaseReset()) return false;
        fIO.Pause();
        return fScope.ProveExclusiveRewrite(fIO) && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x7000 && fIO.ResetRead()==0x7000;
    }
    // Scope must prove the exact restore images and fresh, parked completion;
    // the two status words and PC samples alone are not ownership proof.
    bool HoldCoreLocalRestore(unsigned core,bool& attempted) {
        using namespace PioneerQwen35;
        if (attempted || core>=DataflowBarrierWorkers || !fScope.ProveRoleRestorePhysicalHoldSafe(fIO,core)
            || !RestoreCompletionSafe(core)) return false;
        DataflowRoleSnapshotHandoff handoff{};
        if (!ReadStableStackHandoff(core,handoff)
            || handoff.readerReady!=PIONEER_Q35_ROLE_RESTORE_READER_DONE
            || handoff.writerReady!=PIONEER_Q35_ROLE_RESTORE_WRITER_DONE || handoff.release
            || !fScope.ProveRoleRestorePhysicalHoldSafe(fIO,core) || !RestoreCompletionSafe(core)) return false;
        attempted=true;
        if (!fIO.SharedRoleHoldReset()) return false;
        fIO.Pause();
        return fGuard.ProveSelectedRoute(core) && fIO.ResetRead()==0x47800 && fIO.ResetRead()==0x47800;
    }
    bool RestoreCoreHoldObserved(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers
            && fScope.ProveRoleRestoreHeldAfterAttempt(fIO,core) && RestoreHandoffHeldSafe(core)
            && fScope.ProveRoleRestoreHeldAfterAttempt(fIO,core);
    }
#endif
    // A matching L1 copy alone does not prove the core restored private RAM.
    // Scope must additionally prove either the exact one-shot restore and
    // fresh result, or that stackless cancellation preserved the original
    // RAM, plus held roles, preserved sources and writer exclusion. It
    // currently rejects both proofs in the real Haiku witness.
    bool StacklessCoreRestoreProven(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return StacklessHeldCopySourceSafe(core,reader,writer)
            && fScope.ProveRoleCoreRestore(fIO,core,reader,writer)
            && fGuard.ProveSelectedRoute(core)
            && fScope.ProveRoleCoreRestore(fIO,core,reader,writer)
            && StacklessHeldCopySourceSafe(core,reader,writer);
    }
    bool ReadStacklessRamCopy(unsigned core,unsigned role,unsigned offset,uint32_t* output,unsigned bytes) {
        using namespace PioneerQwen35;
        if (role>=DataflowRoleCount || !output || !bytes || (offset&3) || (bytes&3)
            || offset>DataflowRoleLocalRamBytes || bytes>DataflowRoleLocalRamBytes-offset
            || !fViews.Opened() || !StacklessCopiesSafe(core)) return false;
        const unsigned base=role==DataflowRoleReader?DataflowRoleReaderRamCopy:DataflowRoleWriterRamCopy;
        ImageReadGuard guard(*this);
        return fViews.CopyWords(core,DataflowRolePhysicalExtraCode,
                base-DataflowRoleExtraL1Address+offset,output,bytes/4,guard)
            && StacklessCopiesSafe(core);
    }
    bool CompletedForAcknowledgement(unsigned core,unsigned textWords,unsigned dispatchWords) {
        using namespace PioneerQwen35;
        if (!RuntimeStable(core,textWords,dispatchWords)) return false;
        DataflowWorkerMailbox worker{}; DataflowRoleMailbox roles{};
        if (!ReadStableWorkerMailbox(core,worker) || worker.magic!=DataflowWorkerMailboxMagic
            || worker.version!=DataflowWorkerMailboxVersion || worker.logicalCore!=core
            || worker.shutdownSequence || worker.commandSequence!=worker.completedSequence
            || worker.commandSequence<=worker.acknowledgedSequence
            || worker.state!=DataflowWorkerComplete || worker.failure || worker.phase!=2
            || !ReadStableRoleMailbox(core,roles) || !DataflowRoleMailboxIdle(roles)
            || !QueuesSettled(core,false)) return false;
        return RuntimeStable(core,textWords,dispatchWords);
    }
    bool FaultRuntimeStable(unsigned core,unsigned textWords,unsigned dispatchWords) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !fScope.ProveRoleFaultAuditSafe(fIO,core)
            || !SelectForStop(core)) return false;
        const uint32_t pc1=fIO.RuntimePcRead(); fIO.Pause(); const uint32_t pc2=fIO.RuntimePcRead();
        return DataflowServicePcInImage(pc1,textWords,dispatchWords)
            && DataflowServicePcInImage(pc2,textWords,dispatchWords)
            && fScope.ProveRoleFaultAuditSafe(fIO,core) && fGuard.ProveSelectedRoute(core)
            && fIO.ResetRead()==0x6000 && fIO.ResetRead()==0x6000;
    }
    bool FaultSettled(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& expected,
        unsigned textWords,unsigned dispatchWords) {
        using namespace PioneerQwen35;
        if (!FaultRuntimeStable(core,textWords,dispatchWords)) return false;
        DataflowWorkerSnapshot observed{}; DataflowWorkerMailbox worker{}; DataflowRoleMailbox roles{};
        if (!fIO.ReadWorkerSnapshot(core,observed) || memcmp(&expected,&observed,sizeof(expected))
            || !ReadStableWorkerMailbox(core,worker) || worker.magic!=DataflowWorkerMailboxMagic
            || worker.version!=DataflowWorkerMailboxVersion || worker.logicalCore!=core || worker.shutdownSequence
            || worker.commandSequence!=expected.commandSequence || worker.acknowledgedSequence!=expected.acknowledgedSequence
            || worker.completedSequence!=expected.completedSequence || worker.state!=expected.state
            || worker.failure!=expected.failure || worker.phase!=expected.phase) return false;
        const bool terminal=expected.commandSequence==expected.completedSequence
            && expected.commandSequence>expected.acknowledgedSequence
            && ((expected.state==DataflowWorkerComplete && !expected.failure && expected.phase==2)
                || (expected.state==DataflowWorkerFailed && expected.failure && expected.phase==5));
        return (DataflowWorkerSnapshotReady(expected) || terminal)
            && ReadStableRoleMailbox(core,roles) && DataflowRoleMailboxFaultSettled(roles)
            && QueuesSettled(core,false,true,terminal && expected.state==DataflowWorkerFailed)
            && FaultRuntimeStable(core,textWords,dispatchWords);
    }
    bool FaultCompletedForAcknowledgement(unsigned core,unsigned textWords,unsigned dispatchWords) {
        using namespace PioneerQwen35;
        if (!fScope.ProveRoleFaultAckSafe(fIO,core)) return false;
        DataflowWorkerSnapshot snapshot{};
        if (!fIO.ReadWorkerSnapshot(core,snapshot) || snapshot.commandSequence!=snapshot.completedSequence
            || snapshot.commandSequence<=snapshot.acknowledgedSequence
            || !((snapshot.state==DataflowWorkerComplete && !snapshot.failure && snapshot.phase==2)
                || (snapshot.state==DataflowWorkerFailed && snapshot.failure && snapshot.phase==5))) return false;
        return FaultSettled(core,snapshot,textWords,dispatchWords) && fScope.ProveRoleFaultAckSafe(fIO,core);
    }
    bool AllRolesHeld(unsigned core) {
        return Select(core)
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            && (fScope.ProveRoleDedicatedObservedStage(fIO) || fScope.ProveRoleFleetHeldAndQuiet(fIO,core))
#else
            && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
#endif
            && SelectedHeldAndQuiet(core)
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            && (fScope.ProveRoleDedicatedObservedStage(fIO) || fScope.ProveRoleFleetHeldAndQuiet(fIO,core))
#else
            && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
#endif
            && fGuard.ProveSelectedRoute(core);
    }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    // Final shutdown audit is read-only and happens after staging admission
    // was revoked. Samples are not a fleet-wide exclusion guarantee.
    bool AllRolesHeldForStop(unsigned core) {
        return fScope.ProveRoleDedicatedHeldStatusAllowed(fIO,core)
            && fIO.SelectRuntimeCore(core) && fIO.MapQ38Panel()
            && fIO.DataflowPanelMappingStable() && fGuard.ProveSelectedRoute(core)
            && SelectedHeldAndQuiet(core)
            && fScope.ProveRoleDedicatedHeldStatusAllowed(fIO,core)
            && fGuard.ProveSelectedRoute(core);
    }
#endif
    bool DefaultBootRouting(unsigned core) {
        using namespace PioneerQwen35;
        if (!Select(core) || !fScope.ProveRoleBootRoutingInspectionAllowed(fIO,core)
            || fIO.ResetRead()!=0x47800 || fIO.ResetRead()!=0x47800) return false;
        uint32_t trisc1=0,trisc2=0,ncrisc1=0,ncrisc2=0;
        return fViews.Read(core,DataflowRolePhysicalControl,0x234,trisc1,fGuard)
            && fViews.Read(core,DataflowRolePhysicalControl,0x23c,ncrisc1,fGuard)
            && fViews.Read(core,DataflowRolePhysicalControl,0x234,trisc2,fGuard)
            && fViews.Read(core,DataflowRolePhysicalControl,0x23c,ncrisc2,fGuard)
            && trisc1==trisc2 && ncrisc1==ncrisc2 && !(trisc1&1U) && !(ncrisc1&1U)
            && fIO.ResetRead()==0x47800 && fIO.ResetRead()==0x47800
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            && (fScope.ProveRoleDedicatedObservedStage(fIO) || fScope.ProveRoleFleetHeldAndQuiet(fIO,core))
#else
            && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
#endif
            && fScope.ProveRoleBootRoutingInspectionAllowed(fIO,core)
            && fGuard.ProveSelectedRoute(core);
    }
    bool LocalRamStable(unsigned core) {
        return Select(core) && fScope.ProveRoleLocalRamStable(fIO,core)
            && fGuard.ProveSelectedRoute(core);
    }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        using namespace PioneerQwen35;
        return DataflowRoleExtraL1Range(address,bytes)
            && ReadWords(core,DataflowRolePhysicalExtraCode,address-DataflowRoleExtraL1Address,out,bytes);
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        using namespace PioneerQwen35;
        return DataflowRoleExtraL1Range(address,bytes)
            && WriteWords(core,DataflowRolePhysicalExtraCode,address-DataflowRoleExtraL1Address,in,bytes);
    }
    bool ReadBoot(unsigned core,unsigned address,uint32_t& value) {
        using namespace PioneerQwen35;
        const unsigned which=address==DataflowRoleBriscBootAddress?DataflowRolePhysicalBriscBoot
            :address==DataflowRoleNcriscBootAddress?DataflowRolePhysicalNcriscBoot:DataflowRolePhysicalWindowCount;
        return ReadWords(core,which,0,&value,sizeof(value));
    }
    bool WriteBoot(unsigned core,unsigned address,uint32_t value) {
        using namespace PioneerQwen35;
        const unsigned which=address==DataflowRoleBriscBootAddress?DataflowRolePhysicalBriscBoot
            :address==DataflowRoleNcriscBootAddress?DataflowRolePhysicalNcriscBoot:DataflowRolePhysicalWindowCount;
        return WriteWords(core,which,0,&value,sizeof(value));
    }
    bool ReadShim(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        using namespace PioneerQwen35;
        return address>=DataflowRoleReaderShimAddress && address<DataflowRoleWriterShimLimit
            && bytes<=DataflowRoleWriterShimLimit-address
            && ReadWords(core,DataflowRolePhysicalShim,address-DataflowRoleReaderShimAddress,out,bytes);
    }
    bool WriteShim(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        using namespace PioneerQwen35;
        return address>=DataflowRoleReaderShimAddress && address<DataflowRoleWriterShimLimit
            && bytes<=DataflowRoleWriterShimLimit-address
            && WriteWords(core,DataflowRolePhysicalShim,address-DataflowRoleReaderShimAddress,in,bytes);
    }
    bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* out,unsigned bytes) {
        using namespace PioneerQwen35;
        if (!DataflowRoleLocalRamRange(role,address,bytes)) {
#if defined(PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY) && defined(_KERNEL_MODE)
            dprintf("P100A_Q35_PRESTACK_READ_FAILURE stage=range core=%u role=%u bytes=%u\n",core,role,bytes);
#endif
            return false;
        }
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        return ReadWords(core,role==DataflowRoleReader?DataflowRolePhysicalBriscRam:DataflowRolePhysicalNcriscRam,
            address-base,out,bytes);
    }
    bool WriteRam(unsigned core,unsigned role,uint32_t address,const uint32_t* in,unsigned bytes) {
        using namespace PioneerQwen35;
        if (!DataflowRoleLocalRamRange(role,address,bytes)) return false;
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        return WriteWords(core,role==DataflowRoleReader?DataflowRolePhysicalBriscRam:DataflowRolePhysicalNcriscRam,
            address-base,in,bytes);
    }
    bool ReadResetDisable(unsigned core,uint32_t& value) {
        return ReadWords(core,PioneerQwen35::DataflowRolePhysicalControl,0x224,&value,sizeof(value));
    }
    bool WriteResetDisable(unsigned core,uint32_t desired) {
        uint32_t expected=0;
        return ReadResetDisable(core,expected) && Select(core)
            && fViews.WriteResetDisable(core,expected,desired,fGuard);
    }
};
#endif
