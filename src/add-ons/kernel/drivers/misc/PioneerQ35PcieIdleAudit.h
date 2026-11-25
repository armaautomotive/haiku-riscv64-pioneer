/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q35_PCIE_IDLE_AUDIT_H
#define PIONEER_Q35_PCIE_IDLE_AUDIT_H
#include "PioneerPostResetPci.h"
#include "PioneerPcieRootAerSnapshot.h"
#include "BlackholeHostDmaAudit.h"

// Read-only, bounded PCIe evidence, not lifetime exclusion or a device-reset
// proof. Register fields follow Linux's PCIe definitions:
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/pci_regs.h
struct PioneerQ35PcieIdleAuditResult {
    unsigned samples=0,speed=0,width=0;bool endpointAer=false;
};

template<class IO> struct PioneerQ35PcieAuditAccess {
    IO& io;
    uint32_t HostPciRead(unsigned peer,uint16_t offset,uint8_t width) {
        return peer==0?io.CardRead(offset,width):peer==1?io.RootRead(offset,width):UINT32_MAX;
    }
    uint32_t RootRead(uint16_t offset,uint8_t width) { return io.RootRead(offset,width); }
};

template<class IO>
bool PioneerQ35PcieLinkSample(IO& io,unsigned peer,unsigned& speed,unsigned& width) {
    uint32_t cap=io.HostPciRead(peer,0x34,1);uint64_t visited=0;bool found=false;
    speed=width=0;
    for (unsigned hop=0;cap && hop<48;hop++) {
        if (cap<0x40 || cap>0xec || (cap&3) || (visited&(UINT64_C(1)<<(cap/4)))) return false;
        visited|=UINT64_C(1)<<(cap/4);
        const uint32_t header=io.HostPciRead(peer,cap,2);
        if (header>0xffff) return false;
        if ((header&0xff)==0x10) {
            if (found) return false;
            const uint32_t flags=io.HostPciRead(peer,cap+2,2);
            const unsigned type=(flags>>4)&15,version=flags&15;
            if (flags>0xffff || (version!=1 && version!=2)
                || (peer==0?(type!=0 && type!=1):type!=4)) return false;
            const uint32_t caps=io.HostPciRead(peer,cap+0xc,4);
            const uint32_t status=io.HostPciRead(peer,cap+0x12,2);
            if (caps==UINT32_MAX || status>0xffff) return false;
            const unsigned maxSpeed=caps&15,maxWidth=(caps>>4)&63;
            speed=status&15;width=(status>>4)&63;
            if (!speed || speed>6 || !maxSpeed || speed>maxSpeed || !width || !maxWidth
                || width>maxWidth || (width&(width-1)) || (status&0x0800)
                || ((caps&0x00100000U) && !(status&0x2000))) return false;
            found=true;
        }
        cap=(header>>8)&0xff;
    }
    return !cap && found;
}

// Endpoint AER is optional. If present, require its uncorrectable status
// clear; an unavailable or malformed capability list is never treated as an
// absent capability. Root AER is mandatory on this pinned Pioneer port.
template<class IO>
bool PioneerQ35PcieEndpointAer(IO& io,bool& present) {
    present=false;uint32_t cap=0x100,visited[32]={};
    for (unsigned hop=0;cap && hop<256;hop++) {
        if (cap<0x100 || cap>0xffc || (cap&3)) return false;
        const unsigned bit=cap/4;
        if (visited[bit/32]&(1U<<(bit%32))) return false;
        visited[bit/32]|=1U<<(bit%32);
        const uint32_t header=io.HostPciRead(0,cap,4);
        if (header==UINT32_MAX) return false;
        if (!header) return hop==0; // no extended capabilities
        if (!((header>>16)&15)) return false;
        if ((header&0xffff)==1) {
            if (cap>0xff8 || io.HostPciRead(0,cap+4,4)!=0) return false;
            present=true;return true;
        }
        cap=(header>>20)&0xfff;
    }
    return !cap;
}

template<class IO>
bool PioneerQ35PcieIdleLayout(IO& io,const tt_pioneer_mmio_result& saved) {
    if (saved.version!=1 || saved.commands_before[0] || saved.commands_before[1]
        || saved.bridge_before[0]!=0x0001fff1 || saved.bridge_before[1] || saved.bridge_before[2]
        || io.CardRead(0,4)!=0xb1401e52 || io.CardRead(0x2c,4)!=0x00431e52
        || io.RootRead(0,4)!=0x20421e30 || (io.RootRead(8,4)>>8)!=0x060400
        || io.RootRead(0x18,4)!=0x003f0100 || io.RootRead(0x20,4)!=0x0000fff0
        || io.CardRead(4,2)!=0 || io.RootRead(4,2)!=0 || !io.Quiet() || !BlackholeCardControlsQuiet(io)) return false;
    for (unsigned i=0;i<6;i++) if (saved.bars_before[i]!=(i%2==0?0x0cU:0U)
        || io.CardRead(0x10+4*i,4)!=saved.bars_before[i]) return false;
    for (unsigned i=0;i<3;i++) if (io.RootRead(0x24+4*i,4)!=saved.bridge_before[i]) return false;
    return true;
}

template<class IO,class Check>
__attribute__((noinline)) bool PioneerQ35PcieIdleAudit(IO& raw,const tt_pioneer_mmio_result& saved,
    Check& check,PioneerQ35PcieIdleAuditResult& result) {
    result={};PioneerGuardedPciFinishIO<IO,Check> guarded(raw,check);
    PioneerQ35PcieAuditAccess<PioneerGuardedPciFinishIO<IO,Check>> access{guarded};
    if (!guarded.Valid() || !PioneerQ35PcieIdleLayout(guarded,saved)) return false;
    for (unsigned sample=0;sample<3;sample++) {
        uint32_t cardStatus=0,rootStatus=0,errors=0,source=0;
        unsigned cardSpeed=0,cardWidth=0,rootSpeed=0,rootWidth=0;bool endpointAer=false;
        if (!BlackholeHostPcieStatus(access,0,cardStatus) || !BlackholeHostPcieStatus(access,1,rootStatus)
            || !PioneerQ35PcieLinkSample(access,0,cardSpeed,cardWidth)
            || !PioneerQ35PcieLinkSample(access,1,rootSpeed,rootWidth)
            || cardSpeed!=rootSpeed || cardWidth!=rootWidth
            || !PioneerPcieRootAerSnapshot(access,errors,source) || errors
            || !PioneerQ35PcieEndpointAer(access,endpointAer) || !guarded.Valid()) return false;
        if (sample && (result.speed!=rootSpeed || result.width!=rootWidth || result.endpointAer!=endpointAer)) return false;
        result.speed=rootSpeed;result.width=rootWidth;result.endpointAer=endpointAer;result.samples=sample+1;
        if (sample<2) { if (!guarded.Valid()) return false;raw.Pause();if (!guarded.Valid()) return false; }
    }
    return PioneerQ35PcieIdleLayout(guarded,saved) && guarded.Valid();
}
#endif
