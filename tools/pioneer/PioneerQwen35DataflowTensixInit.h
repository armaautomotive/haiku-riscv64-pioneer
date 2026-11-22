/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_TENSIX_INIT_H
#define PIONEER_QWEN35_DATAFLOW_TENSIX_INIT_H

#include <stdint.h>

namespace PioneerQwen35 {
enum : unsigned {
	DataflowTensixSourceAHeader=0x70700,
	DataflowTensixSourceBHeader=0x70910,
	DataflowTensixSourceBTile=0x70920
};
static_assert(DataflowTensixSourceAHeader>=0x6e000
	&& DataflowTensixSourceBHeader+16==DataflowTensixSourceBTile
	&& DataflowTensixSourceBTile+256<=0x72000,
	"source windows stay in the saved L1 region before the service image");
// VectorUnpack restores the ordinary SrcA selector (3), whereas the
// qualified integer SFPU accumulator loads its FP32 metadata to Dst and
// assumes the fixed-format selector (0x13). The matrix has drained before
// this transition; every later unpack re-establishes its own format.
template<class E>
inline void DataflowTensixPrepareAffineMetadata(E& emitter,bool metadata)
{
	if (metadata) emitter.Config(73,0x13);
}

// The UNPACK source windows include 16-byte headers and a 256-byte SrcB
// tile. Each panel replaces only its first 32 bytes. Define every remaining
// datum before the first panel, as required by the qualified matrix setup.
inline void DataflowTensixInitializeSourceWindows(volatile uint32_t* sourceAHeader,
	volatile uint32_t* sourceBHeader,volatile uint32_t* sourceBTile)
{
	for (unsigned word=0;word<4;word++) {
		sourceAHeader[word]=0;
		sourceBHeader[word]=0;
	}
	for (unsigned word=0;word<64;word++) sourceBTile[word]=0;
}
}
#endif
