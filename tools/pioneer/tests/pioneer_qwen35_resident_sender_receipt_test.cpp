/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowResidentSenderReceipt.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
static DataflowResidentSenderReceipt Read(const volatile uint32_t* words) {
    return {DataflowLoadAcquire(words[0]),DataflowLoadAcquire(words[1]),
        DataflowLoadAcquire(words[2]),DataflowLoadAcquire(words[3])};
}
int main() {
    DataflowResidentCommand command{};command.magic=DataflowResidentCommandMagic;
    command.version=DataflowResidentCommandVersion;command.trigger=7;command.epoch=12;
    command.firstSequence=10;command.count=DataflowGraphDecodeSteps;command.pollLimit=100;
    command.token=42;command.position=1;command.checksum=DataflowResidentCommandChecksum(command);
    volatile uint32_t receipt[4]={99,99,99,99};
    DataflowResidentSenderReceiptClear(receipt);
    assert(!DataflowResidentSenderReceiptMatches(Read(receipt),37,command));
    assert(DataflowResidentSenderReceiptPublish(receipt,37,command.trigger,command.epoch));
    assert(DataflowResidentSenderReceiptMatches(Read(receipt),37,command));
    assert(!DataflowResidentSenderReceiptMatches(Read(receipt),38,command));
    auto stale=command;stale.epoch++;stale.checksum=DataflowResidentCommandChecksum(stale);
    assert(!DataflowResidentSenderReceiptMatches(Read(receipt),37,stale));
    stale=command;stale.trigger++;stale.checksum=DataflowResidentCommandChecksum(stale);
    assert(!DataflowResidentSenderReceiptMatches(Read(receipt),37,stale));
    receipt[3]^=1;assert(!DataflowResidentSenderReceiptMatches(Read(receipt),37,command));
    DataflowResidentSenderReceiptClear(receipt);
    assert(!DataflowResidentSenderReceiptPublish(receipt,DataflowCoreCount,command.trigger,command.epoch));
    assert(!DataflowResidentSenderReceiptMatches(Read(receipt),37,command));
    puts("PASS per-core resident sender settlement marker and stale/corrupt rejection; SIMULATED ONLY");
}
