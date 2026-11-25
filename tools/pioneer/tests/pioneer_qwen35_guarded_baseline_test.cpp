/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedBaseline.h"
#include <cassert>
#include <cstdio>

struct BaselineIO {
    uint32_t tlb[2][3]={{1,2,3},{4,5,6}};
    uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
        0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
    bool revoked=false;unsigned accesses=0,payloadAccesses=0;
    void Touch() { assert(!revoked);++accesses; }
    uint32_t TlbRead(unsigned i) { Touch();assert(i<3);return tlb[1][i]; }
    void TlbWrite(unsigned i,uint32_t value) { Touch();assert(i<3);tlb[1][i]=value; }
    uint32_t ScratchRead(unsigned i) {
        Touch();for (unsigned w=0;w<3;w++) assert(tlb[1][w]==BlackholeArcScratchTlbWord(w));
        switch (i) { case 2:return 5;case 4:return 0;case 11:return 0x10003000;
            case 12:return 0x10002000;case 13:return 0x10001000;default:assert(false);return 0; }
    }
    uint32_t CsmRead(uint32_t address) {
        Touch();for (unsigned w=0;w<3;w++) assert(tlb[1][w]==BlackholeTelemetryTlbWord(w));
        if (address==0x10001000) return 0x10000;
        if (address==0x10001004) return TT_ARC_TELEMETRY_COUNT;
        if (address>=0x10001008 && address<0x10001008+TT_ARC_TELEMETRY_COUNT*4) {
            unsigned i=(address-0x10001008)/4;return (i<<16)|BlackholeTelemetryTag(i);
        }
        assert(address>=0x10002000 && address<0x10002000+TT_ARC_TELEMETRY_COUNT*4);
        const uint32_t values[TT_ARC_TELEMETRY_COUNT]={0x12050001,0,0,0,0,0,0xf7,0x5515,0,0xfff,0,0};
        return values[(address-0x10002000)/4];
    }
    uint32_t NiuRead() { Touch();return source[0]; }
    uint32_t ResetRead() { Touch();return 0x47800; }
    uint32_t SourceRead(unsigned i) { Touch();assert(i<16);return source[i]; }
    uint32_t DramTlbRead(unsigned s,unsigned w) { Touch();assert(s<2 && w<3);return tlb[s][w]; }
    void DramTlbWrite(unsigned s,unsigned w,uint32_t value) { Touch();assert(s<2 && w<3);tlb[s][w]=value; }
    uint32_t DramIdRead(unsigned word) {
        Touch();assert(word<TT_DRAM_ID_WORDS);
        const unsigned ys[7]={12,15,18,12,15,18,21};
        for (unsigned b=0;b<7;b++) {
            bool match=true;for (unsigned w=0;w<3;w++)
                match &= tlb[1][w]==BlackholeDramFixedTlbWord(0xffa00000,b<3?18:17,ys[b],true,w);
            if (!match) continue;
            if (word==0) return 0x1c611000|BlackholeDramPhysicalX(b)|(BlackholeDramPhysicalY(b)<<6);
            if (word==1) return 0x80000|BlackholeDramBank(b);
            return word==2?0xe005:BlackholeDramPhysicalX(b)|(BlackholeDramPhysicalY(b)<<6);
        }
        assert(false);return 0;
    }
    uint32_t DramRead(unsigned) { Touch();++payloadAccesses;return 0; }
    void DramWrite(unsigned,uint32_t) { Touch();++payloadAccesses; }
    uint32_t ComputeNocRead(uint32_t offset) {
        Touch();if (offset==0x44) return 0x1c611001|(2<<6);
        if (offset==0x48) return 0x10000;
        for (unsigned i=0;i<16;i++) if (offset==BlackholeDramSourceOffset(i)) return source[i];
        return 0;
    }
    uint32_t HostInboundRead(unsigned w) { Touch();assert(w<2);return w?0:0x2f; }
    uint32_t HostSiiRead(unsigned w) {
        Touch();const uint32_t words[8]={0x400000,0x400200,0x400100,0x400300,0,0x200,0x100,0x300};
        assert(w<8);return words[w];
    }
    uint32_t HostHdmaRead(unsigned w) { Touch();assert(w<4);return 0; }
    uint32_t HostPciRead(unsigned p,uint16_t offset,uint8_t size) {
        Touch();assert(p<2);
        if (offset==4) { assert(size==2);return 0x402; }
        if (offset==0x34) { assert(size==1);return 0x70; }
        if (offset==0x70) { assert(size==2);return 0x10; }
        if (offset==0x72) { assert(size==2);return 2; }
        if (offset==0x7a) { assert(size==2);return 0; }
        assert(false);return 0;
    }
};
struct Results {
    tt_arc_scratch_result arc{};tt_arc_telemetry_result telemetry{};tt_tensix_reset_result tensix{};
    tt_dram_samples_result dram{};tt_compute_noc_result noc{};tt_host_path_result path{};tt_host_dma_audit_result audit{};
    bool telemetryAttempted=false,resetAttempted=false,dramAttempted=false,nocAttempted=false,pathAttempted=false,auditAttempted=false;
};
template<class Check> static bool Baseline(BaselineIO& io,Results& r,Check& check,
    uint32_t node=2,bool operating=false,bool writes=false,unsigned y=2) {
    return PioneerQ35GuardedBaseline(io,node,operating,writes,y,r.arc,r.telemetry,r.tensix,r.dram,r.noc,
        r.path,r.audit,r.telemetryAttempted,r.resetAttempted,r.dramAttempted,r.nocAttempted,
        r.pathAttempted,r.auditAttempted,check);
}
static unsigned Run(unsigned failAt) {
    BaselineIO io;Results r;unsigned checks=0;
    auto check=[&]() { ++checks;if (checks==failAt) { io.revoked=true;return false; }return true; };
    const bool okay=Baseline(io,r,check);
    assert(!io.payloadAccesses);
    if (!failAt) {
        assert(okay && r.auditAttempted && r.audit.state==TT_HOST_DMA_AUDIT_PASSED
            && r.arc.tlb_restored && r.telemetry.tlb_restored && r.tensix.tlb_restored
            && r.dram.tlb_restored && r.noc.tlb_restored && r.path.tlb_restored && r.audit.tlb_restored);
    } else {
        assert(!okay && io.revoked);
        if (r.telemetryAttempted) assert(!r.telemetry.tlb_restored);
        if (r.resetAttempted) assert(!r.tensix.tlb_restored);
        if (r.dramAttempted) assert(!r.dram.tlb_restored && r.dram.recovery_required);
        if (r.nocAttempted) assert(!r.noc.tlb_restored && r.noc.recovery_required);
        if (r.pathAttempted) assert(!r.path.tlb_restored && r.path.recovery_required);
        if (r.auditAttempted) assert(!r.audit.tlb_restored && r.audit.recovery_required);
    }
    return checks;
}
int main() {
    const unsigned checks=Run(0);
    for (unsigned failure=1;failure<=checks;failure++) Run(failure);
    BaselineIO io;Results r;auto allow=[]() { return true; };
    assert(!Baseline(io,r,allow,11) && !io.accesses);
    assert(!Baseline(io,r,allow,2,true) && !io.accesses);
    assert(!Baseline(io,r,allow,2,false,true) && !io.accesses);
    assert(!Baseline(io,r,allow,2,false,false,3) && !io.accesses);
    std::printf("PASS guarded ARC, Tensix, DRAM, NoC and host baseline: every ticket boundary; SIMULATED ONLY (%u checks)\n",checks);
}
