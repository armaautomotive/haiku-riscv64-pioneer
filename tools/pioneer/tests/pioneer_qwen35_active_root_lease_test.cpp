/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ActiveRootLease.h"

struct Lease {
    uint32_t* saved=nullptr;
    uint32_t config=0;
    bool controls_saved=true,configured=true,ready=true,pending=false,recovery_required=false,niu_restored=false;
};
struct IO {
    uint32_t source[TT_DRAM_SOURCE_WORDS]{};
    uint32_t reset=0x47800;
    uint32_t ResetRead() { return reset; }
    uint32_t ResidentNocRead(uint32_t offset) {
        for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)
            if (BlackholeDramSourceOffset(i)==offset) return source[i];
        assert(false);return UINT32_MAX;
    }
};
int main() {
    IO io;tt_compute_noc_result noc{};Lease lease;uint32_t snapshot=0;lease.saved=&snapshot;
    for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++) noc.source[0][i]=io.source[i]=0x40+i;
    lease.config=noc.source[0][0];
    assert(!PioneerQwen35ActiveRootLeaseMatches(io,noc,lease)); // old baseline lacks active bit
    io.source[0]|=0x10000;
    assert(PioneerQwen35ActiveRootLeaseMatches(io,noc,lease));
    for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++) {
        io.source[i]^=1;assert(!PioneerQwen35ActiveRootLeaseMatches(io,noc,lease));io.source[i]^=1;
    }
    lease.pending=true;assert(!PioneerQwen35ActiveRootLeaseMatches(io,noc,lease));lease.pending=false;
    lease.recovery_required=true;assert(!PioneerQwen35ActiveRootLeaseMatches(io,noc,lease));lease.recovery_required=false;
    lease.niu_restored=true;assert(!PioneerQwen35ActiveRootLeaseMatches(io,noc,lease));lease.niu_restored=false;
    lease.config^=1;assert(!PioneerQwen35ActiveRootLeaseMatches(io,noc,lease));lease.config^=1;
    io.reset=0;assert(!PioneerQwen35ActiveRootLeaseMatches(io,noc,lease));
    std::puts("PASS active root NoC lease accepts configured value and rejects baseline, drift and fault; SIMULATED ONLY");
}
