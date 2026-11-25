/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PROGRAM_SCHEDULE_H
#define PIONEER_QWEN35_PROGRAM_SCHEDULE_H

#include <stdint.h>
#include "PioneerQwen35WorkSchedule.h"

enum {
	PioneerQ35ProgramCoreCount = 120,
	PioneerQ35ProgramMaxFragments = 7
};

struct PioneerQ35ProgramSegment {
	unsigned fragment, firstGroup, groups;
};

struct PioneerQ35ProgramCore {
	unsigned segmentCount, groups;
	PioneerQ35ProgramSegment segment[PioneerQ35ProgramMaxFragments];
};

struct PioneerQ35ProjectionProgram {
	unsigned fragmentCount, groups, activeCores;
	PioneerQ35ProgramCore core[PioneerQ35ProgramCoreCount];
};

// Build one stable SPMD row partition for an entire projection. Each segment
// stays inside one physical bank fragment; a core may own several segments.
// Unlike WorkSchedule waves, this layout has no per-core 16-group dispatch
// cap. Device reader/compute/writer kernels consume each core's segments with
// a local tile loop and publish completion at the program boundary.
inline bool PioneerQ35BuildProjectionProgram(
	const PioneerQ35WorkFragment* fragments, unsigned fragmentCount,
	PioneerQ35ProjectionProgram& program,unsigned coreLimit=PioneerQ35ProgramCoreCount)
{
	if (!fragments || !fragmentCount || fragmentCount>PioneerQ35ProgramMaxFragments
		|| !coreLimit || coreLimit>PioneerQ35ProgramCoreCount)
		return false;
	program={};
	unsigned total=0;
	for (unsigned f=0;f<fragmentCount;f++) {
		const auto& fragment=fragments[f];
		if (!fragment.groups || fragment.firstGroup>0xffffffffU-fragment.groups
			|| fragment.groups>0xffffffffU-total) return false;
		total+=fragment.groups;
	}
	if (!total) return false;
	program.fragmentCount=fragmentCount;
	program.groups=total;
	uint64_t prefix[PioneerQ35ProgramMaxFragments+1]={};
	for (unsigned f=0;f<fragmentCount;f++) prefix[f+1]=prefix[f]+fragments[f].groups;
	for (unsigned core=0;core<coreLimit;core++) {
		const uint64_t begin=uint64_t(total)*core/coreLimit;
		const uint64_t end=uint64_t(total)*(core+1)/coreLimit;
		PioneerQ35ProgramCore& assignment=program.core[core];
		for (unsigned f=0;f<fragmentCount;f++) {
			const uint64_t first=begin>prefix[f]?begin:prefix[f];
			const uint64_t last=end<prefix[f+1]?end:prefix[f+1];
			if (first>=last) continue;
			if (assignment.segmentCount>=PioneerQ35ProgramMaxFragments) return false;
			const uint64_t offset=first-prefix[f],count=last-first;
			if (offset>0xffffffffU-fragments[f].firstGroup || count>0xffffffffU
				|| offset+count>fragments[f].groups) return false;
			assignment.segment[assignment.segmentCount++]={f,
				fragments[f].firstGroup+unsigned(offset),unsigned(count)};
			assignment.groups+=unsigned(count);
		}
		const unsigned expected=unsigned(end-begin);
		if (assignment.groups!=expected) return false;
		if (expected) program.activeCores++;
	}
	return program.activeCores && program.core[coreLimit-1].groups
		==unsigned(uint64_t(total)-uint64_t(total)*(coreLimit-1)/coreLimit);
}

#endif
