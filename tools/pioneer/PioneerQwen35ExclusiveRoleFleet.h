/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_EXCLUSIVE_ROLE_FLEET_H
#define PIONEER_QWEN35_EXCLUSIVE_ROLE_FLEET_H
#include "PioneerQwen35DataflowRoleBoot.h"

namespace PioneerQwen35 {
// Dedicated takeover staging. Old private RAM and code are intentionally not
// saved. IO must own an idle fleet and guarantee that roles remain held and
// routed throughout every operation; observations alone do not provide that
// guarantee. A potentially partial write requires a verified device reset
// and reinitialization before the containing owner can be retired.
class DataflowExclusiveRoleFleet {
    enum Phase { Empty, Faulted, Staged, Armed, Releasing, Running, Retired };
    Phase fPhase=Empty;
    bool fTouched=false;
    unsigned fNextCore=0;
    DataflowRoleProgramImage fImage{}; // trusted immutable generated images
    static constexpr unsigned ChunkWords=64;
    template<class IO> static bool Held(IO& io,unsigned core) {
        return io.AllRolesHeld(core) && io.DefaultBootRouting(core);
    }
    template<class IO> static bool FleetHeld(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!Held(io,core)) return false;
        return true;
    }
    template<class IO> bool WriteRegion(IO& io,unsigned core,unsigned address,
        const uint32_t* input,unsigned words,bool shim) {
        for (unsigned offset=0;offset<words;) {
            const unsigned count=words-offset<ChunkWords?words-offset:ChunkWords;
            if (!Held(io,core)) return false;
            fTouched=true; // an unsuccessful call can have changed the device
            if (!(shim?io.WriteShim(core,address+offset*4,input+offset,count*4)
                :io.Write(core,address+offset*4,input+offset,count*4))) return false;
            offset+=count;
        }
        return true;
    }
    template<class IO> static bool MatchRegion(IO& io,unsigned core,unsigned address,
        const uint32_t* expected,unsigned words,bool shim) {
        uint32_t readback[ChunkWords];
        for (unsigned offset=0;offset<words;) {
            const unsigned count=words-offset<ChunkWords?words-offset:ChunkWords;
            if (!Held(io,core) || !(shim?io.ReadShim(core,address+offset*4,readback,count*4)
                :io.Read(core,address+offset*4,readback,count*4))) return false;
            for (unsigned i=0;i<count;i++) if (readback[i]!=expected[offset+i]) return false;
            offset+=count;
        }
        return true;
    }
    template<class IO> static bool VerifyPayloads(const DataflowRoleProgramImage& image,IO& io) {
        if (!FleetHeld(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            for (unsigned role=0;role<DataflowRoleCount;role++) {
                const unsigned code=role==DataflowRoleReader?DataflowRoleReaderCodeAddress:DataflowRoleWriterCodeAddress;
                const unsigned shim=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
                if (!MatchRegion(io,core,shim,image.shim[role],image.bounds.shimWords[role],true)
                    || !MatchRegion(io,core,code,image.code[role],image.bounds.codeWords[role],false)) return false;
            }
        }
        return FleetHeld(io);
    }
    template<class IO> static bool Verify(const DataflowRoleProgramImage& image,IO& io) {
        if (!VerifyPayloads(image,io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            for (unsigned role=0;role<DataflowRoleCount;role++) {
                const unsigned shim=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
                const unsigned boot=role==DataflowRoleReader?DataflowRoleBriscBootAddress:DataflowRoleNcriscBootAddress;
                uint32_t expected=0,actual=0;
                if (!DataflowRoleBootWord(role,shim,expected) || !Held(io,core)
                    || !io.ReadBoot(core,boot,actual) || actual!=expected) return false;
            }
        }
        return FleetHeld(io);
    }
public:
    DataflowExclusiveRoleFleet()=default;
    DataflowExclusiveRoleFleet(const DataflowExclusiveRoleFleet&)=delete;
    DataflowExclusiveRoleFleet& operator=(const DataflowExclusiveRoleFleet&)=delete;
    bool Untouched() const { return !fTouched; }
    bool ResetRequired() const { return fTouched && fPhase!=Retired; }
    bool StagedAndVerified() const { return fPhase==Staged; }
    template<class IO> bool Stage(const DataflowRoleProgramImage& image,IO& io) {
        if (fPhase!=Empty || !image.Valid()) return false;
        fImage=image;
        fPhase=Faulted; // never retry a partial or uncertain stage
        if (!FleetHeld(io)) return false;
        // Code and shims for the full fleet precede every boot-word write.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned role=0;role<DataflowRoleCount;role++) {
                const unsigned address=role==DataflowRoleReader?DataflowRoleReaderCodeAddress:DataflowRoleWriterCodeAddress;
                if (!WriteRegion(io,core,address,image.code[role],image.bounds.codeWords[role],false)) return false;
            }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned role=0;role<DataflowRoleCount;role++) {
                const unsigned address=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
                if (!WriteRegion(io,core,address,image.shim[role],image.bounds.shimWords[role],true)) return false;
            }
        // Verify the entire payload before installing any boot entry; a
        // dropped or misrouted write must not leave a bootable partial image.
        if (!VerifyPayloads(image,io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned role=0;role<DataflowRoleCount;role++) {
                const unsigned shim=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
                const unsigned boot=role==DataflowRoleReader?DataflowRoleBriscBootAddress:DataflowRoleNcriscBootAddress;
                uint32_t value=0;
                if (!DataflowRoleBootWord(role,shim,value) || !Held(io,core)) return false;
                fTouched=true;
                if (!io.WriteBoot(core,boot,value)) return false;
            }
        if (!Verify(image,io)) return false;
        fPhase=Staged;return true;
    }
    template<class IO> bool VerifyStaged(IO& io) {
        if (fPhase!=Staged || !fImage.Valid()) return false;
        if (!Verify(fImage,io)) { fPhase=Faulted;return false; }
        return true;
    }
    // Arm only the software ledger. The binding must separately prove that
    // the complete compute/role fleet can be safely stopped or reset.
    template<class IO> bool Arm(IO& io) {
        if (!VerifyStaged(io)) return false;
        fPhase=Armed;return true;
    }
    bool BeginRelease(unsigned core) {
        if (fPhase!=Armed || core!=fNextCore || core>=DataflowBarrierWorkers) return false;
        fPhase=Releasing;return true; // an uncertain attempt cannot be retried
    }
    bool ConfirmRelease(unsigned core) {
        if (fPhase!=Releasing || core!=fNextCore) return false;
        ++fNextCore;fPhase=fNextCore==DataflowBarrierWorkers?Running:Armed;return true;
    }
    // Only a caller that has validated the driver's reset/firmware ticket
    // and retired the associated mappings may invoke this transition.
    void RetireAfterVerifiedReset() { fPhase=Retired; }
};
}
#endif
