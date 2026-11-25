/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_NOC_READ_PIPELINE_H
#define PIONEER_QWEN35_DATAFLOW_NOC_READ_PIPELINE_H

#include "PioneerQwen35DataflowNocProtocol.h"

namespace PioneerQwen35 {

// A two-page NoC 0 reader for a single resident role. IO must own this NIU,
// its initiator and transaction IDs for the entire pipeline, keep its
// configuration stable and provide cache-bypassed, disjoint local pages.
// Counters, route samples and a host lock alone do not establish ownership.
// Transaction-ID and FIFO behavior: Blackhole NoC Counters.md and MemoryMap.md
// at https://github.com/tenstorrent/tt-isa-documentation/tree/main/BlackholeA0/NoC
// An uncertain issue or failed completion poisons the pipeline: the caller
// must retain the role/session and cannot publish or reuse an in-flight page.
template<class IO> class DataflowNocReadPipeline {
    static constexpr unsigned Depth=2;
    static constexpr uint32_t FifoEnabled=1U<<16,ClockDisabled=1U<<12;
    IO& fIO;
    // Start initializes every counter before fActive permits an issue. Avoid
    // an aggregate clear here: on freestanding RV32 it can emit a libc
    // memset dependency in the resident role image.
    uint32_t fSelf=0,fConfig=0,fExpected[8];
    uint32_t fIssued=0,fRetired=0;
    bool fStarted=false,fActive=false,fFault=false,fInFlight[Depth]{};

    bool Fail() { fFault=true;return false; }
    bool StableConfig() {
        const uint32_t config=fIO.Read(0x100);
        return config!=UINT32_MAX && !(config&ClockDisabled)
            && !((config^fConfig)&(FifoEnabled|ClockDisabled));
    }
    bool WaitInitiator(unsigned polls) {
        for (unsigned n=0;n<polls;n++) {
            if (!StableConfig()) return false;
            if (!fIO.Read(0x40)) return true;
        }
        return false;
    }
    // A dropped doorbell can leave an ID counter at zero, which must not be
    // confused with a fast completion. Prove this particular command was
    // accepted before permitting another issue or publishing its page.
    bool WaitAccepted(unsigned id,unsigned polls) {
        for (unsigned n=0;n<polls;n++) {
            uint32_t observed=0;
            if (!StableConfig()) return false;
            const auto fifo=DataflowNocCheckFifo([&](unsigned off) { return fIO.Read(off); },observed);
            if (fifo==DataflowNocFifoBadConfig || fifo==DataflowNocFifoOverflow) return false;
            if (fifo!=DataflowNocFifoReady) continue;
            // The ID was idle before this submission. A nonzero outstanding
            // count proves the new request was received; if it completed
            // immediately, the total response count reaches the issued count.
            // An earlier response alone cannot satisfy the latter condition.
            const uint32_t outstanding=fIO.Read(0x240+id*4)&255U;
            const uint32_t responses=fIO.Counter(0);
            if (outstanding>1 || fExpected[0]-responses>2U) return false;
            if (responses==fExpected[0] || outstanding) return true;
        }
        return false;
    }
    bool WaitSpace(unsigned polls) {
        for (unsigned n=0;n<polls;n++) {
            if (!StableConfig()) return false;
            if (!(fConfig&FifoEnabled)) return true;
            const uint32_t available=fIO.Read(0x64),overflow=fIO.Read(0x68);
            const unsigned slots=available&255U;
            if (overflow || slots>16) return false;
            if (slots) return true;
        }
        return false;
    }
public:
    explicit DataflowNocReadPipeline(IO& io):fIO(io) {}
    DataflowNocReadPipeline(const DataflowNocReadPipeline&)=delete;
    DataflowNocReadPipeline& operator=(const DataflowNocReadPipeline&)=delete;
    bool Start() {
        if (fStarted || fActive || fFault) return Fail();
        fStarted=true;
        if (!fIO.Quiet()) return Fail();
        fConfig=fIO.Read(0x100);
        if (fConfig==UINT32_MAX || (fConfig&ClockDisabled)
            || !DataflowNocDecodeLocalRoute(fIO.Read(DataflowNocLogicalIdOffset),fSelf)
            || !StableConfig()) return Fail();
        for (unsigned i=0;i<8;i++)
            fExpected[i]=DataflowNocCounterSupported(i)?fIO.Counter(i):0;
        fActive=true;return true;
    }
    // The caller proves the source is in its assigned weight segment and the
    // destination is the unowned queue page for this sequence.
    bool Issue(uint32_t sequence,uint32_t remote,uint32_t route,uint32_t local,
        unsigned bytes,unsigned polls) {
        const unsigned id=sequence%Depth;
        if (!fActive || fFault || !polls || sequence!=fIssued
            || fIssued-fRetired>=Depth || fInFlight[id] || !bytes || bytes>16384
            || (bytes&63) || (remote&63) || (local&63) || !route || route>0xfff
            || !WaitInitiator(polls) || !WaitSpace(polls)
            || (fIO.Read(0x240+id*4)&255U)) return Fail();
        uint32_t command[10];
        DataflowNocBuildCommand(false,remote,route,local,fSelf,bytes,command);
        command[6]=id<<10;
        for (unsigned i=0;i<10;i++) {
            fIO.Write(i*4,command[i]);
            if (!DataflowNocCommandWordMatches(i,command[i],fIO.Read(i*4))) return Fail();
        }
        if (!StableConfig() || !WaitSpace(polls)) return Fail();
        // Set the ledger before the doorbell: even an uncertain write is an
        // attempted request and is never retried or treated as completed.
        fInFlight[id]=true;
        for (unsigned i=0;i<8;i++)
            if (DataflowNocCounterSupported(i)) fExpected[i]+=DataflowNocCounterDelta(false,bytes,i);
        fIssued++;
        fIO.Fence();fIO.Write(0x40,1);(void)fIO.Read(0x40);fIO.Fence();
        return (WaitInitiator(polls) && WaitAccepted(id,polls)) || Fail();
    }
    // FIFO entries can be invisible to counters. Wait for all initiator FIFOs
    // to empty before using this ID's outstanding count as completion.
    bool Complete(uint32_t sequence,unsigned polls) {
        const unsigned id=sequence%Depth;
        if (!fActive || fFault || !polls || sequence!=fRetired
            || fRetired==fIssued || !fInFlight[id]) return Fail();
        for (unsigned n=0;n<polls;n++) {
            uint32_t observed=0;
            if (!StableConfig()) return Fail();
            const auto fifo=DataflowNocCheckFifo([&](unsigned off) { return fIO.Read(off); },observed);
            if (fifo==DataflowNocFifoBadConfig || fifo==DataflowNocFifoOverflow) return Fail();
            if (fifo!=DataflowNocFifoReady || fIO.Read(0x40)
                || (fIO.Read(0x240+id*4)&255U)) continue;
            fIO.Fence();fInFlight[id]=false;fRetired++;return true;
        }
        return Fail();
    }
    bool Finish() {
        if (!fActive || fFault || fIssued!=fRetired || fInFlight[0] || fInFlight[1]
            || !StableConfig() || !fIO.Quiet()) return Fail();
        auto counter=[&](unsigned i) { return fIO.Counter(i); };
        if (DataflowNocMatchingCounters(counter,fExpected)!=DataflowNocSupportedCounterMask)
            return Fail();
        fActive=false;return true;
    }
    bool Faulted() const { return fFault; }
    uint32_t Issued() const { return fIssued; }
    uint32_t Retired() const { return fRetired; }
};
}
#endif
