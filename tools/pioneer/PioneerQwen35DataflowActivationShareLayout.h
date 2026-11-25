/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_LAYOUT_H
#define PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_LAYOUT_H
#include "PioneerQwen35DataflowBarrierLayout.h"
namespace PioneerQwen35 {
enum : unsigned {
    DataflowActivationShareReadyAddress=0xabf00,
    DataflowActivationShareScratchAddress=0xabf40,
    DataflowActivationSharePacketBytes=64,
    DataflowActivationShareMaxReadBytes=16384,
    DataflowActivationShareReadyMagic=0x31534835
};
static_assert(DataflowActivationShareReadyAddress+DataflowActivationSharePacketBytes==DataflowActivationShareScratchAddress
    && DataflowActivationShareScratchAddress+DataflowActivationSharePacketBytes<=DataflowBarrierArrivalBase,
    "sharing packets must fit in the saved L1 area below the barriers");
}
#endif
