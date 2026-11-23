/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_PHYSICAL_PLAN_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_PHYSICAL_PLAN_H
#include "PioneerQwen35DataflowRoleLocalRamOwner.h"
#include "PioneerQwen35DataflowRoleMemoryPlan.h"
namespace PioneerQwen35 {
// Fixed BAR0 windows proposed for a future held-core role owner. TLB 0 maps
// local L1 at BAR offset 0x19000000; TLB 1 maps local 0xffa00000 at
// 0x19200000. This arithmetic alone does not map memory, select a core,
// prove a route or idle state, grant write access, or touch the card.
enum : unsigned {
    DataflowRoleBarL1=0x19000000,
    DataflowRoleBarLocal=0x19200000,
    DataflowRoleBarPageBytes=4096,
    DataflowRolePhysicalBriscBoot=0,
    DataflowRolePhysicalNcriscBoot,
    DataflowRolePhysicalShim,
    DataflowRolePhysicalExtraCode,
    DataflowRolePhysicalBriscRam,
    DataflowRolePhysicalNcriscRam,
    DataflowRolePhysicalControl,
    DataflowRolePhysicalPc,
    DataflowRolePhysicalNoc0,
    DataflowRolePhysicalNoc1,
    DataflowRolePhysicalWindowCount
};
struct DataflowRolePhysicalWindow { uint32_t offset,bytes; };
inline bool DataflowRolePhysicalWindowPlan(unsigned which,DataflowRolePhysicalWindow& window)
{
    DataflowRolePhysicalWindow result{};
    switch (which) {
        case DataflowRolePhysicalBriscBoot: result={DataflowRoleBarL1+DataflowRoleBriscBootAddress,DataflowRoleBarPageBytes}; break;
        case DataflowRolePhysicalNcriscBoot: result={DataflowRoleBarL1+DataflowRoleNcriscBootAddress,DataflowRoleBarPageBytes}; break;
        case DataflowRolePhysicalShim: result={DataflowRoleBarL1+DataflowRoleReaderShimAddress,DataflowRoleBarPageBytes}; break;
        case DataflowRolePhysicalExtraCode: result={DataflowRoleBarL1+DataflowRoleExtraL1Address,
            DataflowRoleExtraL1Limit-DataflowRoleExtraL1Address}; break;
        case DataflowRolePhysicalBriscRam: result={DataflowRoleBarLocal+(DataflowRoleBriscLocalRam-0xffa00000U),DataflowRoleLocalRamBytes}; break;
        case DataflowRolePhysicalNcriscRam: result={DataflowRoleBarLocal+(DataflowRoleNcriscLocalRam-0xffa00000U),DataflowRoleLocalRamBytes}; break;
        case DataflowRolePhysicalControl: result={DataflowRoleBarLocal+0x112000,DataflowRoleBarPageBytes}; break;
        case DataflowRolePhysicalPc: result={DataflowRoleBarLocal+0x113000,DataflowRoleBarPageBytes}; break;
        case DataflowRolePhysicalNoc0: result={DataflowRoleBarLocal+0x120000,2*DataflowRoleBarPageBytes}; break;
        case DataflowRolePhysicalNoc1: result={DataflowRoleBarLocal+0x130000,2*DataflowRoleBarPageBytes}; break;
        default:return false;
    }
    if ((result.offset&(DataflowRoleBarPageBytes-1)) || !result.bytes
        || (result.bytes&(DataflowRoleBarPageBytes-1))) return false;
    window=result; return true;
}
// Only known read-only status registers and the precise owned memory ranges
// are candidates for reads. Memory accesses additionally require a real
// held/stable proof from the caller; this list alone grants no authority.
inline bool DataflowRolePhysicalReadAllowed(unsigned which,unsigned offset)
{
    if (offset&3) return false;
    switch (which) {
        case DataflowRolePhysicalBriscBoot:
        case DataflowRolePhysicalNcriscBoot: return offset==0;
        case DataflowRolePhysicalShim:
            return offset<DataflowRoleWriterShimLimit-DataflowRoleReaderShimAddress;
        case DataflowRolePhysicalExtraCode:
            return offset<DataflowRoleExtraL1Limit-DataflowRoleExtraL1Address;
        case DataflowRolePhysicalBriscRam:
        case DataflowRolePhysicalNcriscRam: return offset<DataflowRoleLocalRamBytes;
        case DataflowRolePhysicalControl:
            // WALL_CLOCK_L (0x1f0) is deliberately excluded: Tenstorrent's
            // get_timestamp documents that reading it samples/freezes the
            // shared high-half readback. This is not an independent,
            // side-effect-free status read for an unowned tile.
            return offset==0x1b0 || offset==0x224 || offset==0x234
                || offset==0x238 || offset==0x23c;
        case DataflowRolePhysicalPc: return offset>=0x138 && offset<=0x148;
        case DataflowRolePhysicalNoc0:
        case DataflowRolePhysicalNoc1:
            if (offset<=0x24 || offset==0x64 || offset==0x68 || offset==0x100
                || offset==0x148 || offset==0x204 || offset==0x208
                || offset==0x210 || offset==0x214 || offset==0x220 || offset==0x228)
                return true;
            if (offset>=0x240 && offset<0x2c0) return true;
            for (unsigned i=0;i<4;i++) if (offset==0x40+i*0x800) return true;
            return false;
        default:return false;
    }
}
// Candidate write ranges for the role owner. The reset register, NoCs, PCs,
// and all other control registers remain excluded. Callers must separately
// prove exclusive ownership, held/quiet cores, snapshots and exact routing.
inline bool DataflowRolePhysicalWriteRange(unsigned which,unsigned offset,unsigned bytes)
{
    if (!bytes || (offset&3) || (bytes&3) || offset>UINT32_MAX-bytes) return false;
    if (which==DataflowRolePhysicalControl) return offset==0x224 && bytes==4;
    if (which>DataflowRolePhysicalNcriscRam) return false;
    return DataflowRolePhysicalReadAllowed(which,offset)
        && DataflowRolePhysicalReadAllowed(which,offset+bytes-4);
}
inline bool DataflowRolePhysicalAddress(uint64_t barStart,uint64_t barBytes,unsigned which,
    uint64_t& physical,unsigned& bytes)
{
    DataflowRolePhysicalWindow window{};
    if (!DataflowRolePhysicalWindowPlan(which,window) || window.offset>barBytes
        || window.bytes>barBytes-window.offset || barStart>UINT64_MAX-window.offset
        || barStart+window.offset>UINT64_MAX-window.bytes) return false;
    physical=barStart+window.offset; bytes=window.bytes; return true;
}
}
#endif
