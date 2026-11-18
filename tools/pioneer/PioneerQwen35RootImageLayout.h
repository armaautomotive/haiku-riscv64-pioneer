/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ROOT_IMAGE_LAYOUT_H
#define PIONEER_QWEN35_ROOT_IMAGE_LAYOUT_H

#include "PioneerQwen35DeviceLayout.h"

namespace PioneerQwen35 {
enum : unsigned {
	RootBootMagic=0x31573551, RootProgressMagic=0x31503542,
	RootDone=0x31443551, RootFault=0x31463551,
	RootControlAddress=0x74800, RootProgressAddress=0x75000,
	RootScratchAddress=0xb0000, RootScratchEnd=0xe2000,
	RootBounceAddress=RootScratchEnd, RootBounceBytes=16384,
	RootLayoutAddress=0xea000, RootPromptAddress=0x100000,
	RootOutputAddress=RootPromptAddress+MaxPositions*4, RootEnd=RootOutputAddress+MaxPositions*4,
	RootCompletionAddress=0x17ffc0
};
struct RootControl {
	unsigned magic, promptCount, outputLimit, context, eos, endTurn;
	Range rope;
	unsigned reserved[7];
};
static_assert(sizeof(RootControl)==64 && RootControlAddress+sizeof(RootControl)<=RootProgressAddress,
	"control and progress have dedicated cache lines");
static_assert(RootBounceAddress+RootBounceBytes<=RootLayoutAddress
	&& RootLayoutAddress+sizeof(DeviceLayout)<=RootPromptAddress && RootEnd<RootCompletionAddress,
	"dedicated local image regions are disjoint");
}
#endif
