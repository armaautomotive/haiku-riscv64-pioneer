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
// Only these thread-configuration indices below 68 contain Blackhole
// fields. A blanket SETC16 sweep also touches uninstantiated indices, which
// the Tenstorrent reference simulator rejects. Reset the implemented fields
// explicitly; index zero is issued and drained separately by the caller.
inline bool DataflowTensixThreadConfigPresent(unsigned index)
{
	return index<=3 || index==5 || index==7 || (index>=11 && index<=19)
		|| (index>=28 && index<=35) || (index>=37 && index<=41)
		|| (index>=47 && index<=54);
}
// VectorUnpack restores the ordinary SrcA selector (3), whereas the
// qualified integer SFPU accumulator loads its FP32 metadata to Dst and
// assumes the fixed-format selector (0x13). The matrix has drained before
// this transition; every later unpack re-establishes its own format.
template<class E>
inline void DataflowTensixPrepareAffineMetadata(E& emitter,bool metadata)
{
	if (metadata) emitter.Config(73,0x13);
}

// Register 3 controls the core's Dst readback conversion. Matrix dot
// checks require converted signed integers; SFPU results require raw FP32.
// Call only after the engine work for the affected rows has drained.
template<class E>
inline void DataflowTensixSelectReadback(E& emitter,bool floatingPoint)
{
	emitter.Config(3,floatingPoint?0U:0x10000U);
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
