/* SPDX-License-Identifier: MIT */
#define PIONEER_Q35_FABRIC_QUEUE_MODEL_ONLY
#include "pioneer_qwen35_fabric_arc_queue_test.cpp"
using uint32=uint32_t;using uint64=uint64_t;using area_id=int;
enum { B_OK=0,B_PAGE_SIZE=4096,B_ANY_KERNEL_ADDRESS=0,B_UNCACHED_MEMORY=1,
    B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,PIONEER_POST_RESET_CORES=120 };
static FabricQueue* device=nullptr;
static uint32 msiPage[1024]{};
static uint32 resetPage[1024]{};
static uint32 tilePage[1024]{},nocPage[1024]{},tileReset[120]{};
static unsigned tileReads[120]{};
static int dropReset=-1;
static int dropTile=-1;
static int wrongTileId=-1,changeOnAudit=-1;
static unsigned PioneerWorkerX(unsigned core) {
    const unsigned x[]={1,2,3,4,5,6,7,10,11,12,13,14};return x[core/10];
}
static unsigned PioneerWorkerY(unsigned core) { return 2+core%10; }
static uint32 BlackholeDramSourceX(const uint32*,unsigned x) { return x; }
static uint32 BlackholeDramFixedTlbWord(uint32 base,uint32 x,uint32 y,bool readOnly,unsigned w) {
    return w==0?base>>21:w==1?(x<<11)|(y<<17)|(x<<23)|((y&7)<<29):(y>>3)|(readOnly?0xa40:0x40);
}
static unsigned SelectedCore() {
    for (unsigned core=0;core<120;core++) {
        const unsigned x=PioneerWorkerX(core),y=PioneerWorkerY(core);
        if (device->tlb[0]==BlackholeDramFixedTlbWord(0xffa00000U,x,y,false,0)
            && device->tlb[1]==BlackholeDramFixedTlbWord(0xffa00000U,x,y,false,1)
            && device->tlb[2]==BlackholeDramFixedTlbWord(0xffa00000U,x,y,false,2)) return core;
    }
    assert(false);return 120;
}
static unsigned mapCount=0;static int failMap=-1;
static int map_physical_memory(const char*,uint64 address,size_t size,int,uint32,void** view) {
    unsigned n=mapCount++;
    if (int(n)==failMap) return -1;
    if (address==0x100000000ULL+0x19200000 && size==TT_ARC_QUEUE_CSM_BYTES)
        *view=device->csm.data();
    else if (address==0x100000000ULL+0x192b0000 && size==B_PAGE_SIZE) *view=msiPage;
    else if (address==0x100000000ULL+0x19230000 && size==B_PAGE_SIZE) *view=resetPage;
    else if (address==0x100000000ULL+0x19312000 && size==B_PAGE_SIZE) *view=tilePage;
    else assert(false);
    return int(n);
}
static uint32 PioneerRead32(const volatile uint32* p) {
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)device->csm.data();
    if (a>=b && a<b+TT_ARC_QUEUE_CSM_BYTES)
        return device->CsmRead(0x10000000+uint32(a-b));
    const uintptr_t rb=(uintptr_t)resetPage;
    if (a>=rb && a<rb+sizeof(resetPage)) { device->Check(true);device->io++;return *p; }
    const uintptr_t nb=(uintptr_t)nocPage;
    if (a>=nb && a<nb+sizeof(nocPage)) {
        const unsigned core=SelectedCore();device->io++;
        if (p==nocPage+0x44/4) return (PioneerWorkerX(core)^(int(core)==wrongTileId?1U:0U))
            |(PioneerWorkerY(core)<<6)|(17U<<12)|(12U<<19);
        assert(p==nocPage+0x48/4);return 0x10000;
    }
    const uintptr_t tb=(uintptr_t)tilePage;
    if (a>=tb && a<tb+sizeof(tilePage)) {
        assert(p==tilePage+0x1b0/4);const unsigned core=SelectedCore();device->io++;
        if (++tileReads[core]==3 && int(core)==changeOnAudit) return 0;
        return tileReset[core];
    }
    assert(p==msiPage+2);return device->MsiStatusRead();
}
static void PioneerWrite32(volatile uint32* p,uint32 value) {
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)device->csm.data();
    if (a>=b && a<b+TT_ARC_QUEUE_CSM_BYTES) {
        const uint32 address=0x10000000+uint32(a-b);
        if (address!=device->base && address!=device->base+4
            && (address<device->request || address>=device->request+32)) {
            device->request=device->base+32+(device->At(device->base)%4)*32;
            device->response=device->base+160+(device->At(device->base+4)%4)*32;
        }
        device->QueueWrite(address,value);return;
    }
    const uintptr_t rb=(uintptr_t)resetPage;
    if (a>=rb && a<rb+sizeof(resetPage)) {
        device->Check(true);device->io++;
        const unsigned index=unsigned(p-resetPage);
        assert(index>=0x40/4 && index<0x40/4+8);
        if (int(index-0x40/4)!=dropReset) *p=value;
        return;
    }
    const uintptr_t tb=(uintptr_t)tilePage;
    if (a>=tb && a<tb+sizeof(tilePage)) {
        assert(p==tilePage+0x1b0/4 && value==0x47800);const unsigned core=SelectedCore();device->io++;
        if (int(core)!=dropTile) tileReset[core]=value;
        return;
    }
    assert(p==msiPage && value==0);device->NotifyArc();
}
static int snooze(int) { device->Pause();return B_OK; }
struct Profile { uint64 bar0_host_start=0x100000000ULL; };
struct PioneerPostResetMapping {
    Profile profile;bool ready=true,firmwareReady=true,topologyReady=true,savedValid=true;
    bool queueAttempted=false,queueUnsafe=false,dmaModified=false;
    area_id areas[7]={20,21,22,23,24,25,26};
    area_id queueArea=-1,queueMsiArea=-1;
    volatile uint32 *queueView=nullptr,*queueMsi=nullptr;
    const volatile uint32* noc=nocPage;
    struct { uint32 source[16]{}; } topology;
    uint32 queueBase=0,queueRequest=0;
    uint32 saved[3]={1,2,3};
    tt_arc_telemetry_result firmwareTelemetry=FabricQueue::Health();
    bool pciReady=true;
    bool PciReady() const { return pciReady; }
    bool SourceStable() const { return pciReady; }
    uint32 TlbRead(unsigned w) { return device->TlbRead(w); }
    void TlbWrite(unsigned w,uint32 value) { device->TlbWrite(w,value); }
};
struct Guard { int (*validate)(uint64); };
static bool ticketValid=true;
static int Validate(uint64 ticket) { return ticket==123 && ticketValid?B_OK:-1; }
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricArcOwner.h"
using Owner=PioneerQ35FabricArcOwner<Guard>;
struct Scope {
    Owner* expected=nullptr;PioneerPostResetMapping* mapping=nullptr;
    bool allowed=true,phaseReady=true,resetAllowed=true,tilesHeld=true;
    unsigned calls=0,denyAt=0;
    bool FabricArcAllowed(const Owner& owner,const PioneerPostResetMapping& m,uint64 ticket,Owner::Phase phase) {
        if (++calls==denyAt) allowed=false;
        return allowed && expected==&owner && mapping==&m && ticket==123 && phase!=Owner::Failed;
    }
    bool FabricArcPhaseReady(const Owner& owner,const PioneerPostResetMapping& m,uint64 ticket,Owner::Phase phase) {
        return phaseReady && FabricArcAllowed(owner,m,ticket,phase);
    }
    bool FabricResetUnitAllowed(const Owner& owner,const PioneerPostResetMapping& m,uint64 ticket,Owner::Phase phase,bool) {
        return resetAllowed && FabricArcAllowed(owner,m,ticket,phase);
    }
    bool FabricTileCoverage(const Owner& owner,const PioneerPostResetMapping& m,uint64 ticket) {
        return tilesHeld && FabricArcAllowed(owner,m,ticket,owner.CurrentPhase());
    }
    bool FabricTileResetAllowed(const Owner& owner,const PioneerPostResetMapping& m,uint64 ticket,unsigned core) {
        return core<120 && tilesHeld && FabricArcAllowed(owner,m,ticket,owner.CurrentPhase());
    }
};
static void ResetUnit() {
    memset(resetPage,0,sizeof(resetPage));resetPage[0x408/4]=5;
    for (unsigned i=0;i<8;i++) resetPage[0x40/4+i]=UINT32_MAX;
    memset(tileReset,0,sizeof(tileReset));memset(tileReads,0,sizeof(tileReads));
    dropReset=dropTile=wrongTileId=changeOnAudit=-1;
}
int main() {
    Guard guard{Validate};const auto arc=FabricQueue::Arc();tt_arc_queue_result result{};
    FabricQueue fake(0,0,0);device=&fake;PioneerPostResetMapping mapping;Owner owner(mapping,123);
    Scope scope{&owner,&mapping};mapCount=0;ResetUnit();
    ticketValid=false;assert(!owner.Open(&guard,scope) && !mapCount);ticketValid=true;
    assert(owner.Open(&guard,scope) && owner.Opened() && mapping.queueUnsafe && mapCount==4);
    assert(!owner.Open(&guard,scope) && mapCount==4);
    scope.phaseReady=false;assert(!owner.SendNext(&guard,scope,arc,result) && !fake.io);
    scope.phaseReady=true;
    assert(owner.SendNext(&guard,scope,arc,result) && owner.CurrentPhase()==Owner::ClockForced
        && fake.observedMessage==0x33 && fake.observedArgument==250);
    assert(!owner.SendNext(&guard,scope,arc,result));
    scope.resetAllowed=false;assert(!owner.SetRiscReset(&guard,scope,true));scope.resetAllowed=true;
    assert(owner.SetRiscReset(&guard,scope,true) && owner.CurrentPhase()==Owner::RiscsHeld
        && owner.ResetWrites()==0xff);
    for (unsigned i=0;i<8;i++) assert(resetPage[0x40/4+i]==0);
    assert(owner.SendNext(&guard,scope,arc,result) && owner.CurrentPhase()==Owner::TensixToggled
        && fake.observedMessage==0xaf && fake.observedArgument==0);
    assert(owner.SendNext(&guard,scope,arc,result) && owner.CurrentPhase()==Owner::NocReinitialized
        && fake.observedMessage==0x20);
    scope.tilesHeld=false;assert(!owner.HoldAllTiles(&guard,scope));scope.tilesHeld=true;
    assert(owner.HoldAllTiles(&guard,scope) && owner.CurrentPhase()==Owner::TilesHeld
        && owner.FailedTile()==120 && owner.TileWrites(0)==UINT32_MAX
        && owner.TileWrites(1)==UINT32_MAX && owner.TileWrites(2)==UINT32_MAX
        && owner.TileWrites(3)==0xffffff);
    for (unsigned i=0;i<120;i++) assert(tileReset[i]==0x47800);
    assert(owner.SetRiscReset(&guard,scope,false) && owner.CurrentPhase()==Owner::RiscsReleased
        && owner.ResetWrites()==0xff);
    for (unsigned i=0;i<8;i++) assert(resetPage[0x40/4+i]==UINT32_MAX);
    assert(owner.SendNext(&guard,scope,arc,result) && owner.CurrentPhase()==Owner::ClockReleased
        && fake.observedMessage==0x33 && fake.observedArgument==0 && fake.Restored());
    const unsigned io=fake.io;assert(!owner.SendNext(&guard,scope,arc,result) && fake.io==io);
    // Partial mapping and uncertain command outcomes cannot be replayed or unmapped.
    for (int failed=0;failed<4;failed++) {
        FabricQueue partial(0,0,0);device=&partial;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};
        mapCount=0;failMap=failed;assert(!o.Open(&guard,s) && o.CurrentPhase()==Owner::Failed && m.queueUnsafe);
        assert(!o.Open(&guard,s) && mapCount==unsigned(failed+1));
    }
    failMap=-1;
    {
        FabricQueue alias(0,0,0);device=&alias;PioneerPostResetMapping m;m.areas[3]=0;
        Owner o(m,123);Scope s{&o,&m};mapCount=0;
        assert(!o.Open(&guard,s) && o.CurrentPhase()==Owner::Failed && m.queueUnsafe && mapCount==4);
    }
    {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;
        assert(o.Open(&guard,s));assert(o.SendNext(&guard,s,arc,result));
        ResetUnit();assert(o.SetRiscReset(&guard,s,true));
        fault.timeout=true;assert(!o.SendNext(&guard,s,arc,result) && o.CurrentPhase()==Owner::Failed && m.queueUnsafe);
        const unsigned before=fault.io;assert(!o.SendNext(&guard,s,arc,result) && fault.io==before);
    }
    for (int word=0;word<8;word++) {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;ResetUnit();
        assert(o.Open(&guard,s) && o.SendNext(&guard,s,arc,result));
        dropReset=word;assert(!o.SetRiscReset(&guard,s,true) && o.CurrentPhase()==Owner::Failed && m.queueUnsafe);
        assert(o.ResetWrites()==((1U<<(word+1))-1U));
        const unsigned before=fault.io;assert(!o.SetRiscReset(&guard,s,true) && fault.io==before);
    }
    for (int core:{0,31,32,63,64,95,96,119}) {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;ResetUnit();
        assert(o.Open(&guard,s) && o.SendNext(&guard,s,arc,result) && o.SetRiscReset(&guard,s,true));
        assert(o.SendNext(&guard,s,arc,result) && o.SendNext(&guard,s,arc,result));
        dropTile=core;assert(!o.HoldAllTiles(&guard,s) && o.CurrentPhase()==Owner::Failed
            && o.FailedTile()==unsigned(core) && (o.TileWrites(core/32)&(1U<<(core%32))) && m.queueUnsafe);
        const unsigned before=fault.io;assert(!o.HoldAllTiles(&guard,s) && fault.io==before);
    }
    for (bool audit:{false,true}) {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;ResetUnit();
        assert(o.Open(&guard,s) && o.SendNext(&guard,s,arc,result) && o.SetRiscReset(&guard,s,true));
        assert(o.SendNext(&guard,s,arc,result) && o.SendNext(&guard,s,arc,result));
        if (audit) changeOnAudit=17;else wrongTileId=17;
        assert(!o.HoldAllTiles(&guard,s) && o.CurrentPhase()==Owner::Failed && o.FailedTile()==17 && m.queueUnsafe);
        if (audit) assert(o.TileWrites(3)==0xffffff);
        else assert(!(o.TileWrites(0)&(1U<<17)));
    }
    for (int word=0;word<8;word++) {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;ResetUnit();
        assert(o.Open(&guard,s) && o.SendNext(&guard,s,arc,result) && o.SetRiscReset(&guard,s,true));
        assert(o.SendNext(&guard,s,arc,result) && o.SendNext(&guard,s,arc,result)
            && o.HoldAllTiles(&guard,s));
        dropReset=word;assert(!o.SetRiscReset(&guard,s,false) && o.CurrentPhase()==Owner::Failed && m.queueUnsafe);
        assert(o.ResetWrites()==((1U<<(word+1))-1U));
        const unsigned before=fault.io;assert(!o.SetRiscReset(&guard,s,false) && fault.io==before);
    }
    {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;ResetUnit();
        assert(o.Open(&guard,s) && o.SendNext(&guard,s,arc,result));resetPage[0x40/4+3]=0;
        assert(!o.SetRiscReset(&guard,s,true) && o.CurrentPhase()==Owner::Failed && !o.ResetWrites());
    }
    {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;ResetUnit();
        assert(o.Open(&guard,s) && o.SendNext(&guard,s,arc,result));resetPage[0x408/4]=UINT32_MAX;
        assert(!o.SetRiscReset(&guard,s,true) && o.CurrentPhase()==Owner::Failed && !o.ResetWrites());
    }
    {
        FabricQueue fault(0,0,0);device=&fault;PioneerPostResetMapping m;Owner o(m,123);Scope s{&o,&m};mapCount=0;
        assert(o.Open(&guard,s));s.denyAt=s.calls+12;
        assert(!o.SendNext(&guard,s,arc,result) && !s.allowed && o.CurrentPhase()==Owner::Failed && m.queueUnsafe);
        const unsigned before=fault.io;assert(!o.SendNext(&guard,s,arc,result) && fault.io==before);
    }
    puts("PASS retained Q35 fabric ARC owner: fixed phases, scoped ticket, real mapping adapter and fault retention; SIMULATED ONLY");
}
