/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_CODE_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_CODE_OWNER_H

#include "PioneerQwen35DataflowRoleMemoryPlan.h"
#include <stdint.h>

namespace PioneerQwen35 {
// Offline contract for the additional code region only. It cannot start a
// RISC or establish safe release: boot words, local RAM, reset, cache, NoC,
// queues and every other touched resource require independent ownership and
// recovery audits. IO must serialize reset control and ensure roles cannot
// start between a held check and a memory operation. No physical driver
// currently binds this interface.
class DataflowRoleCodeOwner {
    enum Phase { Empty, Captured, Touched, Faulted, Restored };
    enum : unsigned { ChunkWords=64, SnapshotWords=(DataflowRoleExtraL1Limit
        -DataflowRoleExtraL1Address)/sizeof(uint32_t) };
    Phase fPhase=Empty;
    uint32_t* fSnapshot=nullptr;
    // Callers must keep the trusted host-side image buffers immutable until
    // restoration; they are never dereferenced as device-side inference.
    const uint32_t* fReaderImage=nullptr;
    const uint32_t* fWriterImage=nullptr;
    unsigned fReaderWords=0,fWriterWords=0;
    bool fCodeVerified=false;
    unsigned fCore=DataflowBarrierWorkers;

    template<class IO> bool Verify(IO& io,unsigned address,
        const uint32_t* expected,unsigned words) const {
        uint32_t readback[ChunkWords];
        while (words) {
            const unsigned count=words<ChunkWords?words:ChunkWords;
            if (!io.AllRolesHeld(fCore)
                || !io.Read(fCore,address,readback,count*4)) return false;
            for (unsigned i=0;i<count;i++) if (readback[i]!=expected[i]) return false;
            address+=count*4; expected+=count; words-=count;
        }
        return true;
    }
public:
    DataflowRoleCodeOwner()=default;
    DataflowRoleCodeOwner(const DataflowRoleCodeOwner&)=delete;
    DataflowRoleCodeOwner& operator=(const DataflowRoleCodeOwner&)=delete;
    static constexpr unsigned RequiredWords() { return SnapshotWords; }
    bool CapturedSnapshot() const { return fPhase==Captured || fPhase==Touched || fPhase==Faulted; }
    bool NeedsRestore() const { return fPhase==Touched || fPhase==Faulted; }
    bool CodeVerified() const { return fPhase==Touched && fCodeVerified; }
    bool RegionRestored() const { return fPhase==Restored; }

    template<class IO> bool Capture(unsigned core,uint32_t* storage,
        unsigned storageWords,IO& io) {
        if (fPhase!=Empty || core>=DataflowBarrierWorkers || !storage
            || storageWords!=SnapshotWords || !io.AllRolesHeld(core)) return false;
        uint32_t repeated[ChunkWords];
        for (unsigned offset=0;offset<SnapshotWords;offset+=ChunkWords) {
            const unsigned count=SnapshotWords-offset<ChunkWords
                ?SnapshotWords-offset:ChunkWords;
            const unsigned address=DataflowRoleExtraL1Address+offset*4;
            if (!io.AllRolesHeld(core) || !io.Read(core,address,storage+offset,count*4)
                || !io.Read(core,address,repeated,count*4)) return false;
            for (unsigned i=0;i<count;i++)
                if (storage[offset+i]!=repeated[i]) return false;
        }
        if (!io.AllRolesHeld(core)) return false;
        fCore=core; fSnapshot=storage; fPhase=Captured;
        return true;
    }

    template<class IO> bool Stage(bool reader,const uint32_t* code,
        unsigned words,IO& io) {
        const unsigned address=reader?DataflowRoleReaderCodeAddress:DataflowRoleWriterCodeAddress;
        const unsigned limit=reader?DataflowRoleReaderCodeLimit:DataflowRoleWriterCodeLimit;
        if ((fPhase!=Captured && fPhase!=Touched) || !code || !words
            || words>(limit-address)/4) return false;
        // Keep the captured state for recovery even before the first write.
        if (!io.AllRolesHeld(fCore)) { fPhase=Faulted; return false; }
        // A write may partially succeed even if IO reports failure.
        fCodeVerified=false;
        fPhase=Faulted; // no further staging after an uncertain operation
        for (unsigned offset=0;offset<words;) {
            const unsigned count=words-offset<ChunkWords?words-offset:ChunkWords;
            if (!io.AllRolesHeld(fCore)
                || !io.Write(fCore,address+offset*4,code+offset,count*4)
                || !Verify(io,address+offset*4,code+offset,count)) return false;
            offset+=count;
        }
        if (!io.AllRolesHeld(fCore)) return false;
        if (reader) { fReaderImage=code; fReaderWords=words; }
        else { fWriterImage=code; fWriterWords=words; }
        fPhase=Touched;
        return true;
    }

    // Read back both complete staged images after all writes. The caller must
    // still establish boot, cache, NoC, reset and fleet-wide launch proofs.
    template<class IO> bool VerifyStaged(IO& io) {
        if (fPhase!=Touched || !fReaderImage || !fReaderWords
            || !fWriterImage || !fWriterWords) return false;
        fCodeVerified=false;
        const unsigned readerOffset=(DataflowRoleReaderCodeAddress-DataflowRoleExtraL1Address)/4;
        const unsigned writerOffset=(DataflowRoleWriterCodeAddress-DataflowRoleExtraL1Address)/4;
        const unsigned readerSlot=(DataflowRoleReaderCodeLimit-DataflowRoleReaderCodeAddress)/4;
        const unsigned writerSlot=(DataflowRoleWriterCodeLimit-DataflowRoleWriterCodeAddress)/4;
        if (!io.AllRolesHeld(fCore)
            || !Verify(io,DataflowRoleReaderCodeAddress,fReaderImage,fReaderWords)
            || !Verify(io,DataflowRoleReaderCodeAddress+fReaderWords*4,
                fSnapshot+readerOffset+fReaderWords,readerSlot-fReaderWords)
            || !Verify(io,DataflowRoleWriterCodeAddress,fWriterImage,fWriterWords)
            || !Verify(io,DataflowRoleWriterCodeAddress+fWriterWords*4,
                fSnapshot+writerOffset+fWriterWords,writerSlot-fWriterWords)
            || !Verify(io,DataflowRoleWriterCodeLimit,
                fSnapshot+(DataflowRoleWriterCodeLimit-DataflowRoleExtraL1Address)/4,
                (DataflowRoleExtraL1Limit-DataflowRoleWriterCodeLimit)/4)
            || !io.AllRolesHeld(fCore)) { fPhase=Faulted; return false; }
        fCodeVerified=true;
        return true;
    }

    // A fleet owner calls this after every core has restored its snapshot,
    // catching later writes that were misrouted onto an earlier core.
    template<class IO> bool VerifyOriginal(IO& io) {
        if ((fPhase!=Restored && fPhase!=Captured) || !fSnapshot) return false;
        if (!io.AllRolesHeld(fCore)
            || !Verify(io,DataflowRoleExtraL1Address,fSnapshot,SnapshotWords)
            || !io.AllRolesHeld(fCore)) { fPhase=Faulted; return false; }
        return true;
    }

    template<class IO> bool Restore(IO& io) {
        if ((fPhase!=Touched && fPhase!=Faulted) || !fSnapshot) return false;
        // Any failed restoration remains faulted and forbids further staging.
        fCodeVerified=false;
        fPhase=Faulted;
        if (!io.AllRolesHeld(fCore)) return false;
        for (unsigned offset=0;offset<SnapshotWords;offset+=ChunkWords) {
            const unsigned count=SnapshotWords-offset<ChunkWords
                ?SnapshotWords-offset:ChunkWords;
            const unsigned address=DataflowRoleExtraL1Address+offset*4;
            if (!io.AllRolesHeld(fCore) || !io.Write(fCore,address,fSnapshot+offset,count*4)
                || !Verify(io,address,fSnapshot+offset,count)) return false;
        }
        // Re-read the complete region after all writes: a later transfer
        // could otherwise damage a chunk that was already verified.
        if (!io.AllRolesHeld(fCore)
            || !Verify(io,DataflowRoleExtraL1Address,fSnapshot,SnapshotWords)
            || !io.AllRolesHeld(fCore)) return false;
        fPhase=Restored;
        return true;
    }
};
}
#endif
