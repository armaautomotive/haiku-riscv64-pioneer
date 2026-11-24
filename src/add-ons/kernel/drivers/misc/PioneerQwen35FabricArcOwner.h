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
    enum Phase { Fresh=0,ClockForced=1,TensixToggled=2,NocReinitialized=3,
        ClockReleased=4,Failed=5 };
private:
    PioneerPostResetMapping& fMapping;
    const uint64 fTicket;
    Phase fPhase=Fresh;
    bool fOpenAttempted=false,fOpen=false;

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
        for (unsigned i=0;i<7;i++) {
            if (fMapping.areas[i]<B_OK || fMapping.areas[i]==fMapping.queueArea
                || fMapping.areas[i]==fMapping.queueMsiArea) { fPhase=Failed;return false; }
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
        if (!fOpen || fPhase>=ClockReleased || !Allowed(guard,scope)
            || !scope.FabricArcPhaseReady(*this,fMapping,fTicket,fPhase)) return false;
        const uint32 command=fPhase==Fresh?0x33U:fPhase==ClockForced?0xafU:
            fPhase==TensixToggled?0x20U:0x33U;
        const uint32 argument=fPhase==Fresh?250U:0U;
        Transport<Scope> transport{*this,guard,scope};
        PioneerQ38ArcQueue<Transport<Scope>,false,false,true> queue(transport);
        const bool okay=queue.Message(arc,fMapping.firmwareTelemetry,result,command,argument);
        if (!okay || queue.Unsafe() || !Allowed(guard,scope)
            || !scope.FabricArcPhaseReady(*this,fMapping,fTicket,fPhase)) {
            fPhase=Failed;return false;
        }
        fPhase=Phase(unsigned(fPhase)+1);return true;
    }
};
#endif
