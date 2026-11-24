/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_ARC_OWNER_H
#define PIONEER_QWEN35_FABRIC_ARC_OWNER_H

// The real post-reset mapping and v2 guard are supplied by the retained Q35
// recovery context. This owner never unmaps or restores device state in its
// destructor. Its four phases cover only the ARC messages; the scope must
// independently prove the intervening RISC reset-unit and tile transitions.
// A successful message is not a fabric-reset or recovery proof.
template<class Guard> class PioneerQ35FabricArcOwner {
public:
    enum Phase { Fresh=0,ClockForced=1,RiscsHeld=2,TensixToggled=3,NocReinitialized=4,
        TilesHeld=5,RiscsReleased=6,ClockReleased=7,Failed=8 };
private:
    PioneerPostResetMapping& fMapping;
    const uint64 fTicket;
    Phase fPhase=Fresh;
    bool fOpenAttempted=false,fOpen=false;
    area_id fResetArea=-1;
    volatile uint32* fResetView=NULL;
    uint32 fResetWrites=0;

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
    uint32 ResetWrites() const { return fResetWrites; }

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
        for (unsigned i=0;i<7;i++) {
            if (fMapping.areas[i]<B_OK || fMapping.areas[i]==fMapping.queueArea
                || fMapping.areas[i]==fMapping.queueMsiArea || fMapping.areas[i]==fResetArea) {
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
        if (!fOpen || (fPhase!=Fresh && fPhase!=RiscsHeld && fPhase!=TensixToggled
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

    // Assert the eight reset-unit words only after forcing the clock, then
    // release them only after a separate all-tile soft-reset audit. Route,
    // boot and each word are checked; a partially applied write or route
    // change is retained and is never automatically replayed.
    template<class Scope> bool SetRiscReset(Guard* guard,Scope& scope,bool hold) {
        if (!fOpen || !fResetView || (hold?fPhase!=ClockForced:fPhase!=TilesHeld)
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

    template<class Scope> bool ConfirmTilesHeld(Guard* guard,Scope& scope) {
        if (!fOpen || fPhase!=NocReinitialized || !Allowed(guard,scope)
            || !scope.FabricTilesHeld(*this,fMapping,fTicket) || !Allowed(guard,scope)) return false;
        fPhase=TilesHeld;return true;
    }
};
#endif
