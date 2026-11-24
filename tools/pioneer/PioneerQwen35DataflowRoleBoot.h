/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_BOOT_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_BOOT_H
#include "PioneerQwen35DataflowRoleMemoryPlan.h"
#include "PioneerQwen35DataflowRoleMailbox.h"
namespace PioneerQwen35 {
// Bounds of the two exported startup shims and role text images. A parked
// shim is not a running role: it is reached after stop or an invalid tick.
// PC snapshots can be speculative; callers must separately prove images,
// mailbox/queue state, NoC settlement, ownership and safe recovery.
struct DataflowRoleProgramBounds {
    unsigned shimWords[DataflowRoleCount];
    unsigned codeWords[DataflowRoleCount];
    unsigned park[DataflowRoleCount];
    // Optional prefix of codeWords that contains the running service. A
    // zero entry preserves the legacy whole-code range. Snapshot startup can
    // append a restore-only tail that must never count as a live role PC.
    unsigned runtimeCodeWords[DataflowRoleCount]{};
    // Optional exact linked service-loop intervals. Without both bounds the
    // shim is never accepted as a running PC; startup and park are excluded.
    unsigned loopStart[DataflowRoleCount]{};
    unsigned loopEnd[DataflowRoleCount]{};
    bool Valid() const {
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            if (runtimeCodeWords[role]>codeWords[role]) return false;
            const unsigned shim=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
            const unsigned shimLimit=role==DataflowRoleReader?DataflowRoleReaderShimLimit:DataflowRoleWriterShimLimit;
            const unsigned code=role==DataflowRoleReader?DataflowRoleReaderCodeAddress:DataflowRoleWriterCodeAddress;
            const unsigned codeLimit=role==DataflowRoleReader?DataflowRoleReaderCodeLimit:DataflowRoleWriterCodeLimit;
            if (!shimWords[role] || shimWords[role]>(shimLimit-shim)/4
                || !codeWords[role] || codeWords[role]>(codeLimit-code)/4
                || (park[role]&3) || park[role]<shim || park[role]-shim>=shimWords[role]*4) return false;
            if ((loopStart[role] || loopEnd[role])
                && ((loopStart[role]|loopEnd[role])&3 || loopStart[role]<shim
                    || loopStart[role]>=loopEnd[role] || loopEnd[role]>park[role])) return false;
        }
        return true;
    }
    bool ContainsRunning(unsigned role,uint32_t pc) const {
        if (role>=DataflowRoleCount || !Valid() || (pc&3) || pc==park[role]) return false;
        const unsigned code=role==DataflowRoleReader?DataflowRoleReaderCodeAddress:DataflowRoleWriterCodeAddress;
        const unsigned runtime=runtimeCodeWords[role]?runtimeCodeWords[role]:codeWords[role];
        return (loopStart[role] && pc>=loopStart[role] && pc<loopEnd[role])
            || (pc>=code && pc-code<runtime*4);
    }
};
struct DataflowRoleProgramImage {
    DataflowRoleProgramBounds bounds;
    const uint32_t* shim[DataflowRoleCount];
    const uint32_t* code[DataflowRoleCount];
    bool Valid() const {
        return bounds.Valid() && shim[DataflowRoleReader] && shim[DataflowRoleWriter]
            && code[DataflowRoleReader] && code[DataflowRoleWriter];
    }
};
// Link-only boot-word planning. The target must be the audited startup shim
// inside its low-L1 shim slot; encoding a jump does not establish that a
// shim exists or that reset, boot memory, or local RAM are owned.
inline bool DataflowRoleBootWord(unsigned role,unsigned entry,uint32_t& word)
{
    if (role>=DataflowRoleCount || (entry&3)) return false;
    const unsigned first=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
    const unsigned last=role==DataflowRoleReader?DataflowRoleReaderShimLimit:DataflowRoleWriterShimLimit;
    const unsigned boot=role==DataflowRoleReader?DataflowRoleBriscBootAddress:DataflowRoleNcriscBootAddress;
    if (entry<first || entry>=last) return false;
    const int64_t difference=int64_t(entry)-int64_t(boot);
    if (difference < -1048576 || difference > 1048574 || (difference&1)) return false;
    const uint32_t immediate=uint32_t(difference)&0x1fffffU;
    word=0x6fU | ((immediate>>20)&1U)<<31 | ((immediate>>1)&0x3ffU)<<21
        | ((immediate>>11)&1U)<<20 | ((immediate>>12)&0xffU)<<12;
    return true;
}
}
#endif
