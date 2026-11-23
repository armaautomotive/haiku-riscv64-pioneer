/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_LOCAL_RAM_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_LOCAL_RAM_OWNER_H
#include "PioneerQwen35DataflowRoleMailbox.h"
namespace PioneerQwen35 {
// Slow NoC-visible mappings of B and NC private RAM on Blackhole. Releasing a
// RISC can clear its private RAM; snapshot and mark it dirty BEFORE release.
// This offline owner does not release/reset a processor. IO must serialize
// the fleet, prove all roles held and NoCs settled, and independently prove
// that the post-reset RAM initialization window has finished before any NoC
// access. https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/README.md#local-data-ram
enum : unsigned {
    DataflowRoleBriscLocalRam=0xffb14000,
    DataflowRoleNcriscLocalRam=0xffb16000,
    DataflowRoleLocalRamBytes=8192,
    DataflowRoleLocalRamWords=DataflowRoleLocalRamBytes/4
};
static_assert(DataflowRoleBriscLocalRam+DataflowRoleLocalRamBytes==DataflowRoleNcriscLocalRam,
    "BRISC and NCRISC slow private RAM mappings are adjacent and disjoint");
inline bool DataflowRoleLocalRamRange(unsigned role,uint32_t address,unsigned bytes)
{
    if (role>=DataflowRoleCount || !bytes || (address&3) || (bytes&3)) return false;
    const uint32_t first=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
    return address>=first && address<first+DataflowRoleLocalRamBytes
        && bytes<=first+DataflowRoleLocalRamBytes-address;
}
class DataflowRoleLocalRamOwner {
    enum Phase { Empty, Captured, Dirty, Restored };
    enum : unsigned { ChunkWords=64, TotalWords=2*DataflowRoleLocalRamWords };
    Phase fPhase=Empty;
    unsigned fCore=DataflowBarrierWorkers;
    uint32_t* fSnapshot=nullptr;
    template<class IO> bool Stable(IO& io) const {
        return io.AllRolesHeld(fCore) && io.LocalRamStable(fCore);
    }
public:
    enum OriginalAudit { OriginalMatches, OriginalMismatch, OriginalUnreadable };
private:
    template<class IO> OriginalAudit CompareOriginal(IO& io) const {
        uint32_t readback[ChunkWords];
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
            for (unsigned offset=0;offset<DataflowRoleLocalRamWords;offset+=ChunkWords) {
                const unsigned count=DataflowRoleLocalRamWords-offset<ChunkWords
                    ?DataflowRoleLocalRamWords-offset:ChunkWords;
                if (!Stable(io) || !io.ReadRam(fCore,role,base+offset*4,readback,count*4)) return OriginalUnreadable;
                for (unsigned i=0;i<count;i++)
                    if (readback[i]!=fSnapshot[role*DataflowRoleLocalRamWords+offset+i])
                        return Stable(io)?OriginalMismatch:OriginalUnreadable;
            }
        }
        return Stable(io)?OriginalMatches:OriginalUnreadable;
    }
public:
    DataflowRoleLocalRamOwner()=default;
    DataflowRoleLocalRamOwner(const DataflowRoleLocalRamOwner&)=delete;
    DataflowRoleLocalRamOwner& operator=(const DataflowRoleLocalRamOwner&)=delete;
    static constexpr unsigned RequiredWords() { return TotalWords; }
    bool RestorationRequired() const { return fPhase==Dirty; }
    bool RestoredOriginal() const { return fPhase==Restored; }
    const uint32_t* OriginalWords(unsigned role) const {
        return role<DataflowRoleCount && fPhase!=Empty && fSnapshot
            ?fSnapshot+role*DataflowRoleLocalRamWords:nullptr;
    }
    template<class IO> bool Capture(unsigned core,uint32_t* storage,unsigned words,IO& io) {
        if (fPhase!=Empty || core>=DataflowBarrierWorkers || !storage || words!=TotalWords) return false;
        fCore=core;
        if (!Stable(io)) return false;
        uint32_t repeated[ChunkWords];
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
            for (unsigned offset=0;offset<DataflowRoleLocalRamWords;offset+=ChunkWords) {
                const unsigned count=DataflowRoleLocalRamWords-offset<ChunkWords
                    ?DataflowRoleLocalRamWords-offset:ChunkWords;
                uint32_t* dest=storage+role*DataflowRoleLocalRamWords+offset;
                if (!Stable(io) || !io.ReadRam(core,role,base+offset*4,dest,count*4)
                    || !io.ReadRam(core,role,base+offset*4,repeated,count*4)) return false;
                for (unsigned i=0;i<count;i++) if (dest[i]!=repeated[i]) return false;
            }
        }
        if (!Stable(io)) return false;
        fSnapshot=storage; fPhase=Captured; return true;
    }
    // This changes only the software ledger. The future fleet owner must call
    // it before ANY release attempt, including one whose result is uncertain.
    template<class IO> bool MarkMayRun(IO& io) {
        if (fPhase!=Captured) return false;
        if (!Stable(io)) { fPhase=Dirty; return false; }
        fPhase=Dirty; return true;
    }
    // Ledger-only conservative retention after an uncertain fleet event.
    // This does not authorize releasing a processor.
    bool MarkPossiblyDirty() {
        if (fPhase==Dirty) return true;
        if (fPhase!=Captured || !fSnapshot) return false;
        fPhase=Dirty; return true;
    }
    template<class IO> OriginalAudit AuditOriginal(IO& io) {
        if ((fPhase!=Captured && fPhase!=Restored) || !fSnapshot) return OriginalUnreadable;
        const OriginalAudit result=CompareOriginal(io);
        if (result!=OriginalMatches) fPhase=Dirty;
        return result;
    }
    template<class IO> bool VerifyOriginal(IO& io) {
        return AuditOriginal(io)==OriginalMatches;
    }
    template<class IO> bool Restore(IO& io) {
        if (fPhase!=Dirty || !fSnapshot || !Stable(io)) return false;
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
            for (unsigned offset=0;offset<DataflowRoleLocalRamWords;offset+=ChunkWords) {
                const unsigned count=DataflowRoleLocalRamWords-offset<ChunkWords
                    ?DataflowRoleLocalRamWords-offset:ChunkWords;
                const uint32_t* source=fSnapshot+role*DataflowRoleLocalRamWords+offset;
                if (!Stable(io) || !io.WriteRam(fCore,role,base+offset*4,source,count*4)) return false;
            }
        }
        if (CompareOriginal(io)!=OriginalMatches) return false; // retain on mismatch or unreadable state
        fPhase=Restored; return true;
    }
};
}
#endif
