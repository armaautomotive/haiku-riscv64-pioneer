/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLASH_CODE_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_FLASH_CODE_OWNER_H
#if !defined(PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) || defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL)
#error Flash code ownership is an isolated non-loadable witness
#endif
#include "PioneerQwen35DataflowRolePhysicalPlan.h"
#include "PioneerQwen35RootImageLayout.h"
namespace PioneerQwen35 {
enum : unsigned {
    DataflowFlashCodeAddress=0xd0000,DataflowFlashCodeBytes=0x8000,
    DataflowFlashCodeLimit=DataflowFlashCodeAddress+DataflowFlashCodeBytes,
    DataflowFlashCodeWords=DataflowFlashCodeBytes/sizeof(uint32_t),
    DataflowFlashFleetSnapshotWords=DataflowBarrierWorkers*DataflowFlashCodeWords
};
static_assert(unsigned(DataflowRoleExtraL1Limit)<=unsigned(DataflowFlashCodeAddress)
    && unsigned(RootScratchAddress)<=unsigned(DataflowFlashCodeAddress)
    && unsigned(DataflowFlashCodeLimit)<=unsigned(RootScratchEnd)
    && DataflowFlashCodeLimit<=0x180000,
    "flash code is separate from role images but overlaps the legacy ROOT scratch");
// Fixed BAR arithmetic and bounds only: this neither maps the window nor
// authorizes access, route selection, reset, or competing-writer exclusion.
inline bool DataflowFlashCodeRange(unsigned address,unsigned bytes) {
    return bytes && !(address&3) && !(bytes&3) && address>=DataflowFlashCodeAddress
        && address<DataflowFlashCodeLimit && bytes<=DataflowFlashCodeLimit-address;
}
inline bool DataflowFlashCodePhysicalAddress(uint64_t barStart,uint64_t barBytes,uint64_t& physical) {
    constexpr uint64_t offset=uint64_t(DataflowRoleBarL1)+DataflowFlashCodeAddress;
    if ((barStart&(DataflowRoleBarPageBytes-1)) || offset>barBytes
        || DataflowFlashCodeBytes>barBytes-offset || barStart>UINT64_MAX-offset
        || barStart+offset>UINT64_MAX-DataflowFlashCodeBytes) return false;
    physical=barStart+offset;return true;
}

// Owns only the additional fixed L1 code window on all 120 workers. The
// caller must independently pin the BAR and all routes, exclude firmware,
// DMA and other writers, hold every relevant RISC and settle both NoCs.
// IO callbacks must maintain that authority throughout every operation and
// protect the snapshot and image host buffers from mutation/aliasing. This
// class never grants hardware authority or releases any worker.
class DataflowFlashCodeFleetOwner {
    enum Phase { Empty,Captured,Retained,Staged,Restored,Unavailable } fPhase=Empty;
    enum : unsigned { ChunkWords=64 };
    uint32_t* fSnapshot=nullptr;
    const uint32_t* fImage=nullptr;
    unsigned fImageWords=0;
    bool fRestoreAttempted=false;

    static bool HostRange(const uint32_t* data,unsigned words,uintptr_t& start,uintptr_t& end) {
        start=reinterpret_cast<uintptr_t>(data);
        if (!data || (start&3) || !words || uintptr_t(words)>UINTPTR_MAX/sizeof(uint32_t)
            || start>UINTPTR_MAX-uintptr_t(words)*sizeof(uint32_t)) return false;
        end=start+uintptr_t(words)*sizeof(uint32_t);return true;
    }

    template<class IO> bool Ready(IO& io,bool frozen) const {
        return fSnapshot && io.FlashFleetHeldAndQuiet()
            && io.FlashSnapshotSafe(fSnapshot,DataflowFlashFleetSnapshotWords,frozen);
    }
    template<class IO> bool ImageSafe(IO& io) const {
        return fImage && fImageWords && fImageWords<=DataflowFlashCodeWords
            && io.FlashImageSafe(fImage,fImageWords,fSnapshot,DataflowFlashFleetSnapshotWords);
    }
    template<class IO> bool Read(IO& io,unsigned core,unsigned offset,uint32_t* out,unsigned words,bool frozen) const {
        if (core>=DataflowBarrierWorkers || !out || !words || offset>DataflowFlashCodeWords
            || words>DataflowFlashCodeWords-offset) return false;
        const unsigned address=DataflowFlashCodeAddress+offset*4,bytes=words*4;
        return Ready(io,frozen) && io.FlashReadAllowed(core,address,bytes)
            && io.Read(core,address,out,bytes) && io.FlashReadAllowed(core,address,bytes) && Ready(io,frozen);
    }
    template<class IO> bool Write(IO& io,unsigned core,unsigned offset,const uint32_t* input,unsigned words) const {
        if (core>=DataflowBarrierWorkers || !input || !words || offset>DataflowFlashCodeWords
            || words>DataflowFlashCodeWords-offset) return false;
        const unsigned address=DataflowFlashCodeAddress+offset*4,bytes=words*4;
        return Ready(io,true) && io.FlashWriteAllowed(core,address,bytes)
            && io.Write(core,address,input,bytes) && io.FlashWriteAllowed(core,address,bytes) && Ready(io,true);
    }
    template<class IO> bool VerifyCore(IO& io,unsigned core,bool staged) const {
        uint32_t actual[ChunkWords];
        for (unsigned offset=0;offset<DataflowFlashCodeWords;offset+=ChunkWords) {
            unsigned count=DataflowFlashCodeWords-offset;if (count>ChunkWords) count=ChunkWords;
            if ((staged && !ImageSafe(io)) || !Read(io,core,offset,actual,count,true)
                || (staged && !ImageSafe(io))) return false;
            for (unsigned i=0;i<count;i++) {
                const unsigned at=offset+i;
                const uint32_t expected=staged && at<fImageWords?fImage[at]:fSnapshot[core*DataflowFlashCodeWords+at];
                if (actual[i]!=expected) return false;
            }
        }
        return !staged || ImageSafe(io);
    }
public:
    DataflowFlashCodeFleetOwner()=default;
    DataflowFlashCodeFleetOwner(const DataflowFlashCodeFleetOwner&)=delete;
    DataflowFlashCodeFleetOwner& operator=(const DataflowFlashCodeFleetOwner&)=delete;
    static constexpr unsigned RequiredWords() {return DataflowFlashFleetSnapshotWords;}
    bool StagedAndVerified() const {return fPhase==Staged;}
    bool RestorationRequired() const {return fPhase==Retained || fPhase==Staged;}
    bool RegionRestored() const {return fPhase==Restored;}
    template<class IO> bool CaptureAll(uint32_t* storage,unsigned words,IO& io) {
        uintptr_t start=0,end=0;
        if (fPhase!=Empty || words!=RequiredWords() || !HostRange(storage,words,start,end)) return false;
        fSnapshot=storage;fPhase=Unavailable;
        if (!Ready(io,false)) return false;
        uint32_t repeated[ChunkWords];
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned offset=0;offset<DataflowFlashCodeWords;offset+=ChunkWords) {
                unsigned count=DataflowFlashCodeWords-offset;if (count>ChunkWords) count=ChunkWords;
                uint32_t* target=storage+core*DataflowFlashCodeWords+offset;
                if (!Read(io,core,offset,target,count,false) || !Read(io,core,offset,repeated,count,false)) return false;
                for (unsigned i=0;i<count;i++) if (target[i]!=repeated[i]) return false;
            }
        if (!Ready(io,true)) return false;
        fPhase=Captured;return true;
    }
    template<class IO> bool StageAll(const uint32_t* image,unsigned words,IO& io) {
        uintptr_t imageStart=0,imageEnd=0,snapshotStart=0,snapshotEnd=0;
        if (fPhase!=Captured || words>DataflowFlashCodeWords || !HostRange(image,words,imageStart,imageEnd)
            || !HostRange(fSnapshot,RequiredWords(),snapshotStart,snapshotEnd)
            || !(imageEnd<=snapshotStart || snapshotEnd<=imageStart)) return false;
        fImage=image;fImageWords=words;
        if (!Ready(io,true) || !ImageSafe(io)) return false;
        // Each write can have taken effect on any core even if it reports a
        // failure. Never discard the full fleet snapshot or retry staging.
        fPhase=Retained;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned offset=0;offset<words;offset+=ChunkWords) {
                unsigned count=words-offset;if (count>ChunkWords) count=ChunkWords;
                uint32_t actual[ChunkWords];
                if (!ImageSafe(io) || !Write(io,core,offset,image+offset,count)
                    || !Read(io,core,offset,actual,count,true) || !ImageSafe(io)) return false;
                for (unsigned i=0;i<count;i++) if (actual[i]!=image[offset+i]) return false;
            }
        // Recheck all cores including the unmodified tail after all writes;
        // this catches a later write misrouted to an earlier captured core.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!VerifyCore(io,core,true)) return false;
        if (!Ready(io,true) || !ImageSafe(io)) return false;
        fPhase=Staged;return true;
    }
    template<class IO> bool VerifyStagedAll(IO& io) {
        if (fPhase!=Staged) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!VerifyCore(io,core,true)) {fPhase=Retained;return false;}
        if (!Ready(io,true) || !ImageSafe(io)) {fPhase=Retained;return false;}
        return true;
    }
    // Reaudit after every other owner has restored its own regions; a later
    // misrouted write can otherwise damage an already-restored flash window.
    template<class IO> bool VerifyOriginalAll(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!VerifyCore(io,core,false)) {fPhase=Retained;return false;}
        if (!Ready(io,true)) {fPhase=Retained;return false;}
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        // The initial call requires a proof of held, drained and exclusive
        // access; a retry additionally proves every uncertain write drained.
        if ((fPhase!=Staged && fPhase!=Retained) || !Ready(io,true)
            || !io.FlashRestoreSafe(fRestoreAttempted)) return false;
        fRestoreAttempted=true;fPhase=Retained;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned offset=0;offset<DataflowFlashCodeWords;offset+=ChunkWords) {
                unsigned count=DataflowFlashCodeWords-offset;if (count>ChunkWords) count=ChunkWords;
                const uint32_t* original=fSnapshot+core*DataflowFlashCodeWords+offset;
                uint32_t actual[ChunkWords];
                if (!io.FlashRestoreSafe(false) || !Write(io,core,offset,original,count)
                    || !Read(io,core,offset,actual,count,true) || !io.FlashRestoreSafe(false)) return false;
                for (unsigned i=0;i<count;i++) if (actual[i]!=original[i]) return false;
            }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) if (!VerifyCore(io,core,false)) return false;
        if (!Ready(io,true) || !io.FlashRestoreSafe(false)) return false;
        fPhase=Restored;return true;
    }
};
}
#endif
