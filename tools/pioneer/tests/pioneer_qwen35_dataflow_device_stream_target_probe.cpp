/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowDeviceStream.h"

// Compile the real NoC-backed stream fetch for the worker ISA. The resident
// service does not invoke it until stream ownership and graph execution exist.
extern "C" bool PioneerQwen35DeviceStreamCompileProbe(
	const PioneerQwen35::DataflowStreamBinding& binding,const PioneerQwen35::Range& lease,
	const unsigned (&routes)[2][PioneerQwen35::Banks],unsigned core,unsigned index)
{
	return PioneerQwen35::DataflowBlackholeStreamFetch(binding,lease,routes,core,index);
}
