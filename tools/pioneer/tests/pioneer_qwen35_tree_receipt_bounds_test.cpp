/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowResidentSenderReceipt.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
int main() {
    const unsigned columns[12]={1,2,3,4,5,6,7,10,11,12,13,14};
    for (unsigned worker=0;worker<DataflowBarrierWorkers;worker++) {
        const unsigned expected=columns[worker/10]|((2+worker%10)<<6);
        assert(DataflowBarrierWorkerRoute(worker)==expected && DataflowNocRouteMatchesWorker(expected,worker));
        assert(!DataflowNocRouteMatchesWorker(expected^1,worker));
    }
    assert(DataflowBarrierWorkerRoute(120)==UINT32_MAX && !DataflowNocRouteMatchesWorker(0,120));
    for (unsigned parent=0;parent<DataflowBarrierWorkers;parent++)
        for (unsigned child=0;child<DataflowBarrierWorkers;child++) {
            const unsigned route=DataflowBarrierWorkerRoute(child);
            const bool edge=DataflowBarrierReleaseEdge(parent,child);
            assert(DataflowBarrierPeerReceiptReadAllowed(parent,child,route,DataflowBarrierSenderReceiptBlock,
                DataflowBarrierOutbox,64)==edge);
            assert(!DataflowBarrierPeerReceiptReadAllowed(parent,child,route^1,DataflowBarrierSenderReceiptBlock,
                DataflowBarrierOutbox,64));
            assert(!DataflowBarrierPeerReceiptReadAllowed(parent,child,route,DataflowBarrierSenderReceiptBlock+64,
                DataflowBarrierOutbox,64));
            assert(!DataflowBarrierPeerReceiptReadAllowed(parent,child,route,DataflowBarrierSenderReceiptBlock,
                DataflowBarrierOutbox+64,64));
            assert(!DataflowBarrierPeerReceiptReadAllowed(parent,child,route,DataflowBarrierSenderReceiptBlock,
                DataflowBarrierOutbox,32));
        }
    uint32_t command[10]{};
    const auto parentRoute=DataflowBarrierWorkerRoute(29),childRoute=DataflowBarrierWorkerRoute(119);
    assert(DataflowBarrierReleaseEdge(29,119));
    DataflowNocBuildCommand(false,DataflowBarrierSenderReceiptBlock,childRoute,
        DataflowBarrierOutbox,parentRoute,64,command);
    assert(command[0]==DataflowBarrierSenderReceiptBlock && !command[1] && command[2]==childRoute
        && command[3]==DataflowBarrierOutbox && !command[4] && command[5]==parentRoute
        && !command[6] && command[7]==DataflowNocReadControl && command[8]==64 && !command[9]);
    uint32_t before[4]={0,0,0,0},after[4]={0,0,0,0};
    assert(DataflowResidentSenderReceiptReadResult(before,after,119,9,12)==DataflowBarrierWaiting);
    after[0]=9;assert(DataflowResidentSenderReceiptReadResult(before,after,119,9,12)==DataflowBarrierWaiting);
    after[1]=12;after[2]=119;after[3]=DataflowResidentSenderReceiptChecksum(9,12,119);
    for (unsigned i=0;i<4;i++) before[i]=after[i];
    assert(DataflowResidentSenderReceiptReadResult(before,after,119,9,12)==DataflowBarrierReady);
    after[3]^=1;assert(DataflowResidentSenderReceiptReadResult(before,after,119,9,12)==DataflowBarrierWaiting);
    before[3]^=1;assert(DataflowResidentSenderReceiptReadResult(before,after,119,9,12)==DataflowBarrierWaiting);
    before[0]=after[0]=10;assert(DataflowResidentSenderReceiptReadResult(before,after,119,9,12)==DataflowBarrierInvalid);
    before[0]=after[0]=8;assert(DataflowResidentSenderReceiptReadResult(before,after,119,9,12)==DataflowBarrierWaiting);
    assert(DataflowResidentSenderReceiptReadResult(before,after,120,9,12)==DataflowBarrierInvalid);
    assert(DataflowResidentSenderReceiptReadResult(before,after,119,0,12)==DataflowBarrierInvalid);
    puts("PASS fixed parent/child receipt read bounds; OFFLINE ONLY");
}
