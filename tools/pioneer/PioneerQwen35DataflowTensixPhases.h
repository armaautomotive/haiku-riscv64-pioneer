/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_TENSIX_PHASES_H
#define PIONEER_QWEN35_DATAFLOW_TENSIX_PHASES_H

#include "PioneerQ38Q4Phases.h"

namespace PioneerQwen35 {
// Q4_K and Q5_K decode to unsigned 0..15 and 0..31 respectively. Both
// occupy only the low-five-bit SrcA fidelity slice, so phases 0 and 2
// contribute exactly zero. Q6_K and Q8_0 can use upper magnitude bits and
// retain all four phases. This changes no quantization or CPU fallback.
inline unsigned DataflowTensixPhaseType(unsigned type)
{
	return type==13?12:type;
}
template<class E> inline void DataflowTensixPhaseSetup(E& emitter,unsigned type)
{
	PioneerQ38Q4PhasesSetup(emitter,DataflowTensixPhaseType(type));
}
template<class E> inline void DataflowTensixPhases(E& emitter,unsigned type)
{
	PioneerQ38Q4Phases(emitter,DataflowTensixPhaseType(type));
}
}
#endif
