/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35ProgramSchedule.h"
#include <cstdio>
#include <vector>

static bool Check(const PioneerQ35WorkFragment* fragments,unsigned fragmentCount)
{
	PioneerQ35ProjectionProgram program{};
	if (!PioneerQ35BuildProjectionProgram(fragments,fragmentCount,program)) return false;
	std::vector<unsigned> base(fragmentCount+1),hits(program.groups);
	for (unsigned f=0;f<fragmentCount;f++) base[f+1]=base[f]+fragments[f].groups;
	unsigned active=0,minGroups=0xffffffffU,maxGroups=0;
	for (unsigned core=0;core<PioneerQ35ProgramCoreCount;core++) {
		const auto& assignment=program.core[core];
		unsigned sum=0;
		if (assignment.segmentCount>fragmentCount) return false;
		for (unsigned n=0;n<assignment.segmentCount;n++) {
			const auto& segment=assignment.segment[n];
			if (segment.fragment>=fragmentCount || !segment.groups
				|| segment.groups>fragments[segment.fragment].groups
				|| segment.firstGroup<fragments[segment.fragment].firstGroup
				|| segment.firstGroup-fragments[segment.fragment].firstGroup
					>fragments[segment.fragment].groups-segment.groups) return false;
			sum+=segment.groups;
			const unsigned offset=segment.firstGroup-fragments[segment.fragment].firstGroup;
			for (unsigned g=0;g<segment.groups;g++)
				hits[base[segment.fragment]+offset+g]++;
		}
		if (sum!=assignment.groups) return false;
		if (sum) {
			active++;
			if (sum<minGroups) minGroups=sum;
			if (sum>maxGroups) maxGroups=sum;
		}
	}
	if (active!=program.activeCores || active!=(program.groups<PioneerQ35ProgramCoreCount
		?program.groups:PioneerQ35ProgramCoreCount) || maxGroups-minGroups>1) return false;
	for (unsigned count:hits) if (count!=1) return false;
	return true;
}

int main()
{
	const PioneerQ35WorkFragment tiny[]={{0,3},{64,4},{256,1}};
	const PioneerQ35WorkFragment many[]={{0,143},{1000,257},{5000,19},{6000,2048},
		{9000,1},{10000,6553},{20000,13}};
	if (!Check(tiny,3) || !Check(many,7)) return 1;
	PioneerQ35ProjectionProgram program{};
	const PioneerQ35WorkFragment invalidZero[]={{0,0}};
	const PioneerQ35WorkFragment invalidRange[]={{0xffffffffU,1}};
	const PioneerQ35WorkFragment invalidTotal[]={{0,0xffffffffU},{0,1}};
	if (PioneerQ35BuildProjectionProgram(nullptr,1,program)
		|| PioneerQ35BuildProjectionProgram(tiny,0,program)
		|| PioneerQ35BuildProjectionProgram(invalidZero,1,program)
		|| PioneerQ35BuildProjectionProgram(invalidRange,1,program)
		|| PioneerQ35BuildProjectionProgram(invalidTotal,2,program)) return 2;
	std::puts("PASS Qwen35 static 120-core projection partition; exact fragment coverage, balanced stable shards, invalid-range refusals");
	return 0;
}
