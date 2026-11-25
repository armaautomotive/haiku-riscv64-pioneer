/* SPDX-License-Identifier: MIT */
// Compile the local sender receipt for the Tensix RV32 worker ISA.
#include "../PioneerQwen35DataflowResidentSenderReceipt.h"
using namespace PioneerQwen35;
bool PioneerQ35ResidentSenderPublishProbe(volatile uint32_t* words,unsigned core,const DataflowResidentCommand& command) {
    return DataflowResidentCommandValid(command)
        && DataflowResidentSenderReceiptPublish(words,core,command.trigger,command.epoch);
}
void PioneerQ35ResidentSenderClearProbe(volatile uint32_t* words) { DataflowResidentSenderReceiptClear(words); }
