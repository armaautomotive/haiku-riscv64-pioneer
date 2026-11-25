/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_SENDER_RECEIPT_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_SENDER_RECEIPT_H

#include "PioneerQwen35DataflowResidentDispatch.h"
#include "PioneerQwen35DataflowResidentStreamCache.h"

namespace PioneerQwen35 {
enum : unsigned { DataflowResidentSenderReceiptAddress=0xab550,
    DataflowResidentSenderReceiptMagic=0x31535335 };
struct alignas(16) DataflowResidentSenderReceipt {
    uint32_t trigger,epoch,core,checksum;
};
static_assert(sizeof(DataflowResidentSenderReceipt)==16
    && DataflowResidentSenderReceiptAddress>=PIONEER_Q35_ROLE_SNAPSHOT_HANDOFF+12
    && DataflowResidentSenderReceiptAddress+sizeof(DataflowResidentSenderReceipt)<=DataflowResidentStreamCacheAddress,
    "sender settlement receipt must not overlap the role handoff or stream cache");
inline uint32_t DataflowResidentSenderReceiptChecksum(unsigned trigger,unsigned epoch,unsigned core) {
    return DataflowResidentSenderReceiptMagic^trigger^(epoch<<7)^(epoch>>25)^(core*0x9e3779b9U);
}
inline bool DataflowResidentSenderReceiptMatches(const DataflowResidentSenderReceipt& r,
    unsigned core,const DataflowResidentCommand& command) {
    return DataflowResidentCommandValid(command) && core<DataflowCoreCount && r.trigger==command.trigger
        && r.epoch==command.epoch && r.core==core
        && r.checksum==DataflowResidentSenderReceiptChecksum(command.trigger,command.epoch,core);
}
// Written locally only after the worker's final gather send has returned
// success. No further NoC work is issued by that worker until the next token.
// This receipt is not itself proof against external writers or route faults.
__attribute__((noinline)) inline bool DataflowResidentSenderReceiptPublish(volatile uint32_t* words,
    unsigned core,unsigned trigger,unsigned epoch) {
    if (!words || core>=DataflowCoreCount || !trigger || trigger>DataflowResidentMarkerLimit || !epoch) return false;
    DataflowStoreRelease(words[0],0);
    DataflowStoreRelease(words[1],epoch);DataflowStoreRelease(words[2],core);
    DataflowStoreRelease(words[3],DataflowResidentSenderReceiptChecksum(trigger,epoch,core));
    DataflowFence();DataflowStoreRelease(words[0],trigger);
    DataflowL1StoresVisible(words);return true;
}
inline void DataflowResidentSenderReceiptClear(volatile uint32_t* words) {
    DataflowStoreRelease(words[0],0);DataflowL1StoresVisible(words);
}
}
#endif
