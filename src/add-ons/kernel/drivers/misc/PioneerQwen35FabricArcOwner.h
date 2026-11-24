/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_ARC_OWNER_H
#define PIONEER_QWEN35_FABRIC_ARC_OWNER_H
#include "PioneerQwen35DataflowNocSafetySample.h"

// The real post-reset mapping and v2 guard are supplied by the retained Q35
// recovery context. This owner never unmaps or restores device state in its
// destructor. Its phases cover the ARC messages and the fixed reset-unit and
// tile transitions; the scope must independently authorize those operations.
// A successful message is not a fabric-reset or recovery proof.
template<class Guard> class PioneerQ35FabricArcOwner {
    static_assert(PIONEER_POST_RESET_CORES==120,"fabric reset requires the admitted 120-worker topology");
public:
    enum Phase { Fresh=0,ClockForced=1,RiscsHeld=2,TensixToggled=3,NocReinitialized=4,
        TilesHeld=5,RiscsReleased=6,ClockReleased=7,Failed=8,Retired=9 };
private:
    PioneerPostResetMapping& fMapping;
    const uint64 fTicket;
    Phase fPhase=Fresh;
    bool fOpenAttempted=false,fOpen=false,fRetirementAttempted=false;
    bool fClockBaseline=false,fClockForced=false,fClockRestored=false;
    area_id fResetArea=-1;
    volatile uint32* fResetView=NULL;
    area_id fTileArea=-1;
    volatile uint32* fTileView=NULL;
    uint32 fResetWrites=0;
    uint32 fTileWrites[4]={};
    unsigned fFailedTile=120;
    uint32 fFinalDescriptorAddress=0,fFinalQueueBase=0;
    uint32 fFinalDescriptor[4]={},fFinalQueueHeader[8]={};

    template<class Scope> bool Allowed(Guard* guard,Scope& scope) const {
        return guard && guard->validate && fTicket && guard->validate(fTicket)==B_OK
            && scope.FabricArcAllowed(*this,fMapping,fTicket,fPhase)
            && guard->validate(fTicket)==B_OK;
    }
    template<class Scope> struct Transport {
        PioneerQ35FabricArcOwner& owner;
        Guard* guard;
        Scope& scope;
        bool delayFailed=false;
        bool Accessible() {
            return !delayFailed && owner.fOpen && owner.fPhase!=Failed
                && owner.Allowed(guard,scope) && owner.fMapping.PciReady()
                && owner.Allowed(guard,scope);
        }
        uint32 TlbRead(unsigned w) { return Accessible()?owner.fMapping.TlbRead(w):UINT32_MAX; }
        void TlbWrite(unsigned w,uint32 value) { if (Accessible() && w<3) owner.fMapping.TlbWrite(w,value); }
        uint32 CsmRead(uint32 address) {
            return Accessible() && BlackholeArcQueueRange(address,4)
                ?PioneerRead32(owner.fMapping.queueView+(address-0x10000000)/4):UINT32_MAX;
        }
        void SetQueueBounds(uint32 base,uint32 request,uint32) {
            if (!Accessible() || !BlackholeArcQueueRange(base,288) || request<base+32
                || request>base+128 || (request-base-32)%32) return;
            owner.fMapping.queueBase=base;owner.fMapping.queueRequest=request;
        }
        void QueueWrite(uint32 address,uint32 value) {
            auto& m=owner.fMapping;
            if (!Accessible() || !m.queueBase || !BlackholeArcQueueRange(address,4)) return;
            if (address!=m.queueBase && address!=m.queueBase+4
                && (address<m.queueRequest || address>=m.queueRequest+32)) return;
            PioneerWrite32(m.queueView+(address-0x10000000)/4,value);
        }
        uint32 MsiStatusRead() {
            return Accessible()?PioneerRead32(owner.fMapping.queueMsi+2):UINT32_MAX;
        }
        void NotifyArc() { if (Accessible()) PioneerWrite32(owner.fMapping.queueMsi,0); }
        void Pause() { if (!Accessible() || snooze(500)!=B_OK) delayFailed=true; }
    };
public:
    PioneerQ35FabricArcOwner(PioneerPostResetMapping& mapping,uint64 ticket)
        :fMapping(mapping),fTicket(ticket) {}
    PioneerQ35FabricArcOwner(const PioneerQ35FabricArcOwner&)=delete;
    PioneerQ35FabricArcOwner& operator=(const PioneerQ35FabricArcOwner&)=delete;
    ~PioneerQ35FabricArcOwner() = default;
    Phase CurrentPhase() const { return fPhase; }
    uint64 Ticket() const { return fTicket; }
    bool Opened() const { return fOpen; }
    bool ClockRestored() const { return fClockRestored; }
    uint32 ResetWrites() const { return fResetWrites; }
    uint32 TileWrites(unsigned word) const { return word<4?fTileWrites[word]:0; }
    unsigned FailedTile() const { return fFailedTile; }

    template<class Scope> bool Open(Guard* guard,Scope& scope) {
        if (fOpenAttempted || fPhase!=Fresh || !Allowed(guard,scope)
            || !fMapping.ready || !fMapping.firmwareReady || !fMapping.topologyReady
            || !fMapping.savedValid || fMapping.queueAttempted || fMapping.queueArea>=B_OK
            || fMapping.queueMsiArea>=B_OK || fMapping.dmaModified || !fMapping.PciReady()) return false;
        fOpenAttempted=true;fMapping.queueAttempted=true;fMapping.queueUnsafe=true;
        void* view=NULL;
        if (!Allowed(guard,scope)) { fPhase=Failed;return false; }
        fMapping.queueArea=map_physical_memory("P100A Q35 fabric recovery ARC queue",
            fMapping.profile.bar0_host_start+0x19200000,TT_ARC_QUEUE_CSM_BYTES,
            B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,&view);
        if (fMapping.queueArea<B_OK || !view) { fPhase=Failed;return false; }
        fMapping.queueView=(volatile uint32*)view;view=NULL;
        if (!Allowed(guard,scope) || !fMapping.PciReady()) { fPhase=Failed;return false; }
        fMapping.queueMsiArea=map_physical_memory("P100A Q35 fabric recovery ARC notify",
            fMapping.profile.bar0_host_start+0x192b0000,B_PAGE_SIZE,
            B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,&view);
        if (fMapping.queueMsiArea<B_OK || !view) { fPhase=Failed;return false; }
        fMapping.queueMsi=(volatile uint32*)view;
        if (fMapping.queueMsiArea==fMapping.queueArea) { fPhase=Failed;return false; }
        view=NULL;
        if (!Allowed(guard,scope) || !fMapping.PciReady()) { fPhase=Failed;return false; }
        fResetArea=map_physical_memory("P100A Q35 fabric recovery reset unit",
            fMapping.profile.bar0_host_start+0x19230000,B_PAGE_SIZE,
            B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,&view);
        if (fResetArea<B_OK || !view || fResetArea==fMapping.queueArea
            || fResetArea==fMapping.queueMsiArea) { fPhase=Failed;return false; }
        fResetView=(volatile uint32*)view;
        view=NULL;
        if (!Allowed(guard,scope) || !fMapping.PciReady()) { fPhase=Failed;return false; }
        fTileArea=map_physical_memory("P100A Q35 fabric recovery tile reset",
            fMapping.profile.bar0_host_start+0x19312000,B_PAGE_SIZE,
            B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,&view);
        if (fTileArea<B_OK || !view || fTileArea==fMapping.queueArea
            || fTileArea==fMapping.queueMsiArea || fTileArea==fResetArea) { fPhase=Failed;return false; }
        fTileView=(volatile uint32*)view;
        for (unsigned i=0;i<7;i++) {
            if (fMapping.areas[i]<B_OK || fMapping.areas[i]==fMapping.queueArea
                || fMapping.areas[i]==fMapping.queueMsiArea || fMapping.areas[i]==fResetArea
                || fMapping.areas[i]==fTileArea) {
                fPhase=Failed;return false;
            }
            for (unsigned j=0;j<i;j++) if (fMapping.areas[i]==fMapping.areas[j]) {
                fPhase=Failed;return false;
            }
        }
        if (!Allowed(guard,scope) || !fMapping.PciReady()) { fPhase=Failed;return false; }
        fOpen=true;return true;
    }

    // Exactly one of the four fixed firmware messages follows from the
    // retained phase. The external scope controls admission and must prove
    // the matching physical reset-unit/tile state before and during each
    // message. Queue uncertainty poisons the owner and retains all mappings.
    template<class Scope> bool SendNext(Guard* guard,Scope& scope,
        const tt_arc_scratch_result& arc,tt_arc_queue_result& result) {
        if (!fOpen || (fPhase==Fresh && !fClockBaseline)
            || (fPhase!=Fresh && fPhase!=RiscsHeld && fPhase!=TensixToggled
                && fPhase!=RiscsReleased) || !Allowed(guard,scope)
            || !scope.FabricArcPhaseReady(*this,fMapping,fTicket,fPhase)) return false;
        const uint32 command=fPhase==Fresh?0x33U:fPhase==RiscsHeld?0xafU:
            fPhase==TensixToggled?0x20U:0x33U;
        const uint32 argument=fPhase==Fresh?250U:0U;
        Transport<Scope> transport{*this,guard,scope};
        PioneerQ38ArcQueue<Transport<Scope>,false,false,true> queue(transport);
        const bool okay=queue.Message(arc,fMapping.firmwareTelemetry,result,command,argument);
        if (!okay || queue.Unsafe() || !Allowed(guard,scope)
            || !scope.FabricArcPhaseReady(*this,fMapping,fTicket,fPhase)) {
            fPhase=Failed;return false;
        }
        fPhase=fPhase==Fresh?ClockForced:fPhase==RiscsHeld?TensixToggled:
            fPhase==TensixToggled?NocReinitialized:ClockReleased;
        return true;
    }

    // Check the original automatic clock mode before forcing, the actual
    // forced 250 MHz before holding RISCs, and the restored automatic mode
    // after release. A successful firmware command alone does not establish
    // that the requested clock state was applied.
    template<class Scope> bool CheckClock(Guard* guard,Scope& scope,
        const tt_arc_scratch_result& arc,tt_arc_queue_result& result) {
        if (!fOpen || (fPhase!=Fresh && fPhase!=ClockForced && fPhase!=ClockReleased)
            || (fPhase==Fresh && fClockBaseline) || (fPhase==ClockForced && fClockForced)
            || (fPhase==ClockReleased && fClockRestored) || !Allowed(guard,scope)
            || !scope.FabricClockQueryAllowed(*this,fMapping,fTicket,fPhase)) return false;
        Transport<Scope> transport{*this,guard,scope};
        PioneerQ38ArcQueue<Transport<Scope>,false,false,true> queue(transport);
        if (!queue.Message(arc,fMapping.firmwareTelemetry,result,0x34U) || queue.Unsafe()
            || !Allowed(guard,scope)
            || !scope.FabricClockQueryAllowed(*this,fMapping,fTicket,fPhase)) {
            fPhase=Failed;return false;
        }
        const bool clockOkay=fPhase==ClockForced?(result.clock_mode==2 && result.clock_mhz==250)
            :(result.clock_mode==3 && result.clock_mhz>=200 && result.clock_mhz<=1400);
        if (!clockOkay) { fPhase=Failed;return false; }
        if (fPhase==Fresh) fClockBaseline=true;
        else if (fPhase==ClockForced) fClockForced=true;
        else {
            if (result.version!=1 || result.state!=TT_ARC_QUERY_PASSED || !result.submitted
                || !result.responded || !result.acknowledged || !result.request_restored
                || !result.tlb_restored || result.recovery_required || result.status
                || !BlackholeArcQueueRange(arc.message_queue,16)
                || !BlackholeArcQueueRange(result.base,288) || result.descriptor[0]!=result.base
                || fMapping.queueBase!=result.base || !BlackholeArcQueueHeaderValid(result.after)
                || result.after[0]!=result.after[4] || result.after[1]!=result.after[5]) {
                fPhase=Failed;return false;
            }
            fFinalDescriptorAddress=arc.message_queue;fFinalQueueBase=result.base;
            for (unsigned i=0;i<4;i++) fFinalDescriptor[i]=result.descriptor[i];
            for (unsigned i=0;i<8;i++) fFinalQueueHeader[i]=result.after[i];
            // Any pre-existing audits predate the completed reset sequence.
            // Fresh fleet and DMA audits are required before retirement.
            fMapping.fleetAudited=false;fMapping.dmaAudited=false;
            fClockRestored=true;
        }
        return true;
    }

    // Assert the eight reset-unit words only after forcing the clock, then
    // release them only after a separate all-tile soft-reset audit. Route,
    // boot and each word are checked; a partially applied write or route
    // change is retained and is never automatically replayed.
    template<class Scope> bool SetRiscReset(Guard* guard,Scope& scope,bool hold) {
        if (!fOpen || !fResetView || (hold?(fPhase!=ClockForced || !fClockForced):fPhase!=TilesHeld)
            || !Allowed(guard,scope)
            || !scope.FabricResetUnitAllowed(*this,fMapping,fTicket,fPhase,hold)) return false;
        auto access=[&]() {
            return Allowed(guard,scope) && fMapping.PciReady()
                && scope.FabricResetUnitAllowed(*this,fMapping,fTicket,fPhase,hold)
                && Allowed(guard,scope);
        };
        auto fail=[&]() { fPhase=Failed;return false; };
        fResetWrites=0;
        uint32 saved[3]{};
        for (unsigned i=0;i<3;i++) {
            if (!access()) return fail();
            saved[i]=fMapping.TlbRead(i);
            if (!access() || fMapping.TlbRead(i)!=saved[i] || saved[i]!=fMapping.saved[i]) return fail();
        }
        for (unsigned i=0;i<3;i++) {
            if (!access()) return fail();
            fMapping.TlbWrite(i,BlackholeArcQueueTlbWord(true,i));
            if (!access() || fMapping.TlbRead(i)!=BlackholeArcQueueTlbWord(true,i)) return fail();
        }
        auto route=[&]() {
            if (!access()) return false;
            for (unsigned i=0;i<3;i++)
                if (fMapping.TlbRead(i)!=BlackholeArcQueueTlbWord(true,i)) return false;
            return access();
        };
        if (!route()) return fail();
        const uint32 boot=PioneerRead32(fResetView+0x408/4);
        if ((boot&7U)!=5U || !route() || PioneerRead32(fResetView+0x408/4)!=boot) return fail();
        const uint32 expected=hold?UINT32_MAX:0U,desired=hold?0U:UINT32_MAX;
        for (unsigned i=0;i<8;i++) {
            if (!route() || PioneerRead32(fResetView+0x40/4+i)!=expected
                || !route() || PioneerRead32(fResetView+0x40/4+i)!=expected) return fail();
        }
        for (unsigned i=0;i<8;i++) {
            if (!route()) return fail();
            fResetWrites|=1U<<i; // before an uncertain write
            PioneerWrite32(fResetView+0x40/4+i,desired);
            if (!route() || PioneerRead32(fResetView+0x40/4+i)!=desired) return fail();
        }
        for (unsigned i=0;i<8;i++)
            if (!route() || PioneerRead32(fResetView+0x40/4+i)!=desired) return fail();
        if (!route() || PioneerRead32(fResetView+0x408/4)!=boot || !access()) return fail();
        for (unsigned i=0;i<3;i++) {
            if (!access()) return fail();
            fMapping.TlbWrite(i,saved[i]);
            if (!access() || fMapping.TlbRead(i)!=saved[i]) return fail();
        }
        if (!access()) return fail();
        fPhase=hold?RiscsHeld:RiscsReleased;return true;
    }

    // Unicast the upstream broadcast value to the exact 120 admitted worker
    // routes, then independently reread every tile. The scope must establish
    // that this list covers the installed card and that reset-unit holds,
    // firmware, NoC and the PCI reservation remain exclusively controlled.
    template<class Scope> bool HoldAllTiles(Guard* guard,Scope& scope) {
        if (!fOpen || !fTileView || !fMapping.noc || fPhase!=NocReinitialized || !Allowed(guard,scope)
            || !scope.FabricTileCoverage(*this,fMapping,fTicket)) return false;
        auto access=[&](unsigned core) {
            return Allowed(guard,scope) && fMapping.PciReady() && fMapping.SourceStable()
                && scope.FabricTileCoverage(*this,fMapping,fTicket)
                && scope.FabricTileResetAllowed(*this,fMapping,fTicket,core)
                && Allowed(guard,scope);
        };
        auto fail=[&]() { fPhase=Failed;return false; };
        uint32 saved[3]{};
        for (unsigned i=0;i<3;i++) {
            if (!access(0)) return fail();
            saved[i]=fMapping.TlbRead(i);
            if (!access(0) || fMapping.TlbRead(i)!=saved[i] || saved[i]!=fMapping.saved[i]) return fail();
        }
        for (unsigned w=0;w<4;w++) fTileWrites[w]=0;
        for (unsigned pass=0;pass<2;pass++) for (unsigned core=0;core<120;core++) {
            fFailedTile=core;
            if (!access(core)) return fail();
            const unsigned x=PioneerWorkerX(core),y=PioneerWorkerY(core);
            for (unsigned i=0;i<3;i++) {
                if (!access(core)) return fail();
                const uint32 word=BlackholeDramFixedTlbWord(0xffa00000U,x,y,false,i);
                fMapping.TlbWrite(i,word);
                if (!access(core) || fMapping.TlbRead(i)!=word) return fail();
            }
            auto route=[&]() {
                if (!access(core)) return false;
                for (unsigned i=0;i<3;i++)
                    if (fMapping.TlbRead(i)!=BlackholeDramFixedTlbWord(0xffa00000U,x,y,false,i)) return false;
                const uint32 id=PioneerRead32(fMapping.noc+0x44/4);
                const uint32 endpoint=PioneerRead32(fMapping.noc+0x48/4);
                const uint32 expected=BlackholeDramSourceX(fMapping.topology.source,x)|(y<<6);
                if ((id&0xfffU)!=expected || ((id>>12)&127U)!=17U
                    || ((id>>19)&127U)!=12U || (endpoint&0xffffff00U)!=0x10000U) return false;
                for (unsigned network=0;network<2;network++) {
                    uint32 local=0;
                    if (!PioneerQwen35::DataflowNocDecodeLocalRoute(PioneerRead32(fMapping.noc
                            +(network*0x10000U+PioneerQwen35::DataflowNocLogicalIdOffset)/4),local)
                        || !PioneerQwen35::DataflowNocRouteMatchesWorker(local,core)) return false;
                }
                return access(core);
            };
            auto tileClockAndFifos=[&]() {
                return route() && PioneerQwen35SampleNocFifosAndClock([&](unsigned offset) {
                    if (!route()) return uint32(UINT32_MAX);
                    const uint32 value=PioneerRead32(fMapping.noc+offset/4);
                    return route()?value:uint32(UINT32_MAX);
                }) && route();
            };
            // Never touch a tile RISC register unless both NIUs have stable,
            // enabled tile clocks and any enabled request FIFOs are empty.
            if (!tileClockAndFifos()) return fail();
            if (!pass) {
                fTileWrites[core/32]|=1U<<(core%32); // before the uncertain write
                PioneerWrite32(fTileView+0x1b0/4,0x47800U);
            }
            if (!tileClockAndFifos() || PioneerRead32(fTileView+0x1b0/4)!=0x47800U
                || !tileClockAndFifos() || PioneerRead32(fTileView+0x1b0/4)!=0x47800U) return fail();
        }
        for (unsigned i=0;i<3;i++) {
            if (!access(119)) return fail();
            fMapping.TlbWrite(i,saved[i]);
            if (!access(119) || fMapping.TlbRead(i)!=saved[i]) return fail();
        }
        if (!access(119) || !scope.FabricTileCoverage(*this,fMapping,fTicket)) return fail();
        fFailedTile=120;fPhase=TilesHeld;return true;
    }

    // The surrounding locked recovery transaction must run fresh fleet and
    // host-DMA audits after the last clock check. Its scope must separately
    // prove writer exclusion, ticket ownership and the
    // post-reset identities throughout retirement. No hardware is accessed
    // after an uncertain deletion or lost proof; the remaining mappings and
    // the original owner are retained for recovery, never retried here.
    template<class Scope> bool Retire(Guard* guard,Scope& scope) {
        if (!fOpen || fRetirementAttempted || fPhase!=ClockReleased || !fClockRestored
            || !fMapping.queueUnsafe || !fMapping.fleetAudited || !fMapping.dmaAudited
            || !fMapping.savedValid || fMapping.selected>=PIONEER_POST_RESET_CORES
            || !BlackholeArcQueueRange(fFinalDescriptorAddress,16)
            || !BlackholeArcQueueRange(fFinalQueueBase,288) || fMapping.queueBase!=fFinalQueueBase
            || !fResetView || !fTileView || !fMapping.queueView || !fMapping.queueMsi
            || fResetArea<B_OK || fTileArea<B_OK || fMapping.queueArea<B_OK || fMapping.queueMsiArea<B_OK
            || !Allowed(guard,scope) || !scope.FabricRetirementAllowed(*this,fMapping,fTicket)) return false;
        const unsigned core=fMapping.selected;
        auto access=[&]() {
            return fMapping.fleetAudited && fMapping.dmaAudited && fMapping.selected==core
                && Allowed(guard,scope) && scope.FabricRetirementAllowed(*this,fMapping,fTicket)
                && fMapping.SourceStable() && Allowed(guard,scope)
                && scope.FabricRetirementAllowed(*this,fMapping,fTicket);
        };
        fRetirementAttempted=true;
        auto fail=[&]() { fPhase=Failed;return false; };
        for (unsigned word=0;word<3;word++) {
            if (!access()) return fail();
            const uint32 expected=BlackholeDramFixedTlbWord(0xffa00000U,
                PioneerWorkerX(core),PioneerWorkerY(core),true,word);
            if (fMapping.TlbRead(word)!=expected || !access() || fMapping.TlbRead(word)!=expected)
                return fail();
        }
        if (!access()) return fail();
        // Reopen only the ARC CSM route and confirm the final acknowledged
        // clock-query receipt still describes an empty, unchanged queue.
        // A route or queue mismatch is uncertain and is never retried.
        uint32 workerRoute[3]{};
        for (unsigned word=0;word<3;word++) {
            workerRoute[word]=BlackholeDramFixedTlbWord(0xffa00000U,
                PioneerWorkerX(core),PioneerWorkerY(core),true,word);
            if (!access()) return fail();
            fMapping.TlbWrite(word,BlackholeArcQueueTlbWord(false,word));
            if (!access() || fMapping.TlbRead(word)!=BlackholeArcQueueTlbWord(false,word)) return fail();
        }
        auto csmRoute=[&]() {
            if (!access()) return false;
            for (unsigned word=0;word<3;word++)
                if (fMapping.TlbRead(word)!=BlackholeArcQueueTlbWord(false,word)) return false;
            return access();
        };
        for (unsigned pass=0;pass<2;pass++) {
            for (unsigned word=0;word<4;word++) {
                if (!csmRoute() || PioneerRead32(fMapping.queueView
                        +(fFinalDescriptorAddress-0x10000000U)/4+word)!=fFinalDescriptor[word]) return fail();
            }
            for (unsigned word=0;word<8;word++) {
                if (!csmRoute() || PioneerRead32(fMapping.queueView
                        +(fFinalQueueBase-0x10000000U)/4+word)!=fFinalQueueHeader[word]) return fail();
            }
            if (!csmRoute() || (pass==0 && snooze(500)!=B_OK)) return fail();
        }
        for (unsigned word=0;word<3;word++) {
            if (word==0 && !csmRoute()) return fail();
            if (!access()) return fail();
            fMapping.TlbWrite(word,workerRoute[word]);
            if (!access() || fMapping.TlbRead(word)!=workerRoute[word]) return fail();
        }
        if (!access()) return fail();
        area_id* areas[]={&fTileArea,&fResetArea,&fMapping.queueMsiArea,&fMapping.queueArea};
        for (unsigned i=0;i<4;i++) {
            if (!access() || delete_area(*areas[i])!=B_OK) return fail();
            *areas[i]=-1;
            if (i==0) fTileView=NULL;
            else if (i==1) fResetView=NULL;
            else if (i==2) fMapping.queueMsi=NULL;
            else fMapping.queueView=NULL;
        }
        if (!access()) return fail();
        fMapping.queueBase=fMapping.queueRequest=0;
        fMapping.queueUnsafe=false;fOpen=false;fPhase=Retired;return true;
    }
};
#endif
