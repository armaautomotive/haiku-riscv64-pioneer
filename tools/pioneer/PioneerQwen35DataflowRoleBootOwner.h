/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_BOOT_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_BOOT_OWNER_H
#include "PioneerQwen35DataflowRoleBoot.h"
namespace PioneerQwen35 {
// Offline per-tile boot/shim transaction only. IO must serialize exclusive
// ownership, prove every role held and NoCs settled, and prove that the
// default NCRISC reset-PC routing is active. The source shim buffers must
// remain immutable until restoration. Local RAM, extra code L1, reset, caches,
// queue state and physical PC audits are NOT covered by this owner.
class DataflowRoleBootOwner {
    enum Phase { Empty, Captured, Faulted, Staged, Restored };
    enum : unsigned { ShimWords=(DataflowRoleWriterShimLimit-DataflowRoleReaderShimAddress)/4,
        ChunkWords=16 };
    Phase fPhase=Empty;
    unsigned fCore=DataflowBarrierWorkers;
    uint32_t fBoot[DataflowRoleCount]{},fShim[ShimWords]{};
    const uint32_t* fReader=nullptr;
    const uint32_t* fWriter=nullptr;
    unsigned fReaderWords=0,fWriterWords=0;
    uint32_t fJump[DataflowRoleCount]{};

    template<class IO> bool Held(IO& io) const {
        return io.AllRolesHeld(fCore) && io.DefaultBootRouting(fCore);
    }
    template<class IO> bool ReadBoot(IO& io,unsigned role,uint32_t& value) const {
        const unsigned address=role==DataflowRoleReader?DataflowRoleBriscBootAddress:DataflowRoleNcriscBootAddress;
        return Held(io) && io.ReadBoot(fCore,address,value);
    }
    template<class IO> bool CompareShim(IO& io,bool staged) const {
        if (staged && (!fReader || !fReaderWords || !fWriter || !fWriterWords)) return false;
        uint32_t shim[ChunkWords];
        const unsigned writerStart=(DataflowRoleWriterShimAddress-DataflowRoleReaderShimAddress)/4;
        for (unsigned first=0;first<ShimWords;first+=ChunkWords) {
            const unsigned count=ShimWords-first<ChunkWords?ShimWords-first:ChunkWords;
            if (!Held(io) || !io.ReadShim(fCore,DataflowRoleReaderShimAddress+first*4,
                    shim,count*4)) return false;
            for (unsigned n=0;n<count;n++) {
                const unsigned i=first+n;
                const uint32_t expected=staged && i<fReaderWords?fReader[i]
                    :staged && i>=writerStart && i-writerStart<fWriterWords?fWriter[i-writerStart]:fShim[i];
                if (shim[n]!=expected) return false;
            }
        }
        return Held(io);
    }
    template<class IO> bool CompareOriginal(IO& io) const {
        uint32_t value=0;
        for (unsigned role=0;role<DataflowRoleCount;role++)
            if (!ReadBoot(io,role,value) || value!=fBoot[role]) return false;
        return CompareShim(io,false);
    }
    template<class IO> bool VerifyImages(IO& io) const {
        if (!fReader || !fReaderWords || !fWriter || !fWriterWords) return false;
        uint32_t value=0;
        for (unsigned role=0;role<DataflowRoleCount;role++)
            if (!ReadBoot(io,role,value) || value!=fJump[role]) return false;
        return CompareShim(io,true);
    }
public:
    DataflowRoleBootOwner()=default;
    DataflowRoleBootOwner(const DataflowRoleBootOwner&)=delete;
    DataflowRoleBootOwner& operator=(const DataflowRoleBootOwner&)=delete;
    bool RestorationRequired() const { return fPhase==Faulted || fPhase==Staged; }
    bool StagedAndVerified() const { return fPhase==Staged; }
    bool RestoredOriginal() const { return fPhase==Restored; }
    template<class IO> bool Capture(unsigned core,IO& io) {
        if (fPhase!=Empty || core>=DataflowBarrierWorkers) return false;
        fCore=core;
        if (!Held(io)) return false;
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            uint32_t second=0;
            if (!ReadBoot(io,role,fBoot[role]) || !ReadBoot(io,role,second)
                || fBoot[role]!=second) return false;
        }
        if (!io.ReadShim(core,DataflowRoleReaderShimAddress,fShim,sizeof(fShim))
            || !CompareShim(io,false)) return false;
        if (!Held(io)) return false;
        fPhase=Captured;
        return true;
    }
    template<class IO> bool Stage(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (fPhase!=Captured || !reader || !readerWords || !writer || !writerWords
            || readerWords>(DataflowRoleReaderShimLimit-DataflowRoleReaderShimAddress)/4
            || writerWords>(DataflowRoleWriterShimLimit-DataflowRoleWriterShimAddress)/4)
            return false;
        // The snapshot must remain available to a fleet recovery even if
        // held/reset routing becomes uncertain before the first write.
        if (!Held(io)) { fPhase=Faulted; return false; }
        if (!DataflowRoleBootWord(DataflowRoleReader,DataflowRoleReaderShimAddress,fJump[0])
            || !DataflowRoleBootWord(DataflowRoleWriter,DataflowRoleWriterShimAddress,fJump[1]))
            return false;
        fReader=reader; fReaderWords=readerWords; fWriter=writer; fWriterWords=writerWords;
        // Any failed write/readback can be partial; never stage again until
        // both original boot words and the entire shim range are restored.
        fPhase=Faulted;
        if (!Held(io) || !io.WriteShim(fCore,DataflowRoleReaderShimAddress,reader,readerWords*4)
            || !Held(io) || !io.WriteShim(fCore,DataflowRoleWriterShimAddress,writer,writerWords*4))
            return false;
        // Before changing either boot word, verify both shims and every
        // untouched shim byte with the original boot words still present.
        if (!CompareShim(io,true)) return false;
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            uint32_t original=0;
            if (!ReadBoot(io,role,original) || original!=fBoot[role]) return false;
        }
        if (!Held(io) || !io.WriteBoot(fCore,DataflowRoleBriscBootAddress,fJump[0])
            || !Held(io) || !io.WriteBoot(fCore,DataflowRoleNcriscBootAddress,fJump[1])
            || !VerifyImages(io)) return false;
        fPhase=Staged;
        return true;
    }
    template<class IO> bool VerifyStaged(IO& io) {
        if (fPhase!=Staged) return false;
        if (!VerifyImages(io)) { fPhase=Faulted; return false; }
        return true;
    }
    // Replace held roles' shims while retaining the original boot/shim
    // snapshot. This is a building block for the separate local-RAM restore
    // witness, not permission to release a role. IO must independently own
    // every relevant writer and keep the fleet held. New source images must
    // remain immutable until restoration. An uncertain write is not retried.
    template<class IO> bool Restage(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (fPhase!=Staged || !reader || !readerWords || !writer || !writerWords
            || readerWords>(DataflowRoleReaderShimLimit-DataflowRoleReaderShimAddress)/4
            || writerWords>(DataflowRoleWriterShimLimit-DataflowRoleWriterShimAddress)/4)
            return false;
        fPhase=Faulted;
        if (!Held(io) || !VerifyImages(io)) return false;
        // Reinstall saved padding before the new images so a shorter shim
        // cannot leave stale instructions after its last word.
        if (!io.WriteShim(fCore,DataflowRoleReaderShimAddress,fShim,sizeof(fShim))
            || !Held(io) || !io.WriteShim(fCore,DataflowRoleReaderShimAddress,reader,readerWords*4)
            || !Held(io) || !io.WriteShim(fCore,DataflowRoleWriterShimAddress,writer,writerWords*4))
            return false;
        fReader=reader; fReaderWords=readerWords; fWriter=writer; fWriterWords=writerWords;
        if (!VerifyImages(io)) return false;
        fPhase=Staged; return true;
    }
    // Fleet-level final readback also checks captured tiles which were not
    // intentionally written, since a later transfer may have been misrouted.
    template<class IO> bool VerifyOriginal(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
        if (!CompareOriginal(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool Restore(IO& io) {
        if (fPhase!=Staged && fPhase!=Faulted) return false;
        fPhase=Faulted;
        // Restore reset entry points before overwriting either shim.
        if (!Held(io) || !io.WriteBoot(fCore,DataflowRoleBriscBootAddress,fBoot[0])
            || !Held(io) || !io.WriteBoot(fCore,DataflowRoleNcriscBootAddress,fBoot[1])
            || !Held(io) || !io.WriteShim(fCore,DataflowRoleReaderShimAddress,fShim,sizeof(fShim))
            || !CompareOriginal(io)) return false;
        fPhase=Restored;
        return true;
    }
};
}
#endif
