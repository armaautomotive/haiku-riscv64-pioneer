/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_WORK_SCHEDULE_H
#define PIONEER_QWEN35_WORK_SCHEDULE_H

struct PioneerQ35WorkSchedule {
	unsigned rootGroups, workerCount, scheduledGroups, groupsPerWorker, extraWorkers;

	unsigned Groups(unsigned worker) const
	{
		return worker<workerCount?groupsPerWorker+unsigned(worker<extraWorkers):0;
	}
};

inline bool PioneerQ35BuildWorkSchedule(unsigned remaining,PioneerQ35WorkSchedule& schedule)
{
	if (!remaining) return false;
	schedule={};
	schedule.rootGroups=remaining>1?1:0;
	const unsigned available=remaining-schedule.rootGroups;
	schedule.workerCount=available<119?available:119;
	if (!schedule.workerCount) return false;
	schedule.scheduledGroups=available>schedule.workerCount*16? schedule.workerCount*16:available;
	schedule.extraWorkers=schedule.scheduledGroups;
	while (schedule.extraWorkers>=schedule.workerCount) {
		schedule.extraWorkers-=schedule.workerCount;
		schedule.groupsPerWorker++;
	}
	return schedule.groupsPerWorker && schedule.groupsPerWorker<=16
		&& schedule.extraWorkers<schedule.workerCount;
}

struct PioneerQ35WorkFragment {
	unsigned firstGroup, groups;
};

struct PioneerQ35WorkTask {
	unsigned fragment, firstGroup, groups;
};

enum : unsigned { PioneerQ35WorkMaxFragments=28 };

struct PioneerQ35WorkBatch {
	unsigned rootFragment, rootFirstGroup, rootGroups, workerCount, processedGroups;
	unsigned nextGroup[PioneerQ35WorkMaxFragments];
	PioneerQ35WorkTask worker[119];
};

enum : unsigned { PioneerQ35ArgmaxPeerCount=119, PioneerQ35ArgmaxMaxFragments=7 };

struct PioneerQ35ArgmaxTask {
	unsigned fragment, firstGroup, groups;
};

struct PioneerQ35ArgmaxProgram {
	unsigned fragmentCount, workerCount, groups;
	PioneerQ35ArgmaxTask root;
	PioneerQ35ArgmaxTask worker[PioneerQ35ArgmaxPeerCount];
};

// Partition a large row-wise reduction into one persistent work item per
// peer. ROOT owns the first group; peers own balanced contiguous ranges that
// never cross a physical bank fragment. Unlike WorkBatch this has no
// per-peer 16-group output-buffer limit because peers reduce locally.
inline bool PioneerQ35BuildArgmaxProgram(const PioneerQ35WorkFragment* fragments,
	unsigned fragmentCount,PioneerQ35ArgmaxProgram& program)
{
	if (!fragments || !fragmentCount || fragmentCount>PioneerQ35ArgmaxMaxFragments) return false;
	program={};
	unsigned total=0;
	unsigned expectedFirst=0;
	for (unsigned f=0;f<fragmentCount;f++) {
		if (!fragments[f].groups || fragments[f].firstGroup!=expectedFirst
			|| fragments[f].groups>0xffffffffU-expectedFirst) return false;
		expectedFirst+=fragments[f].groups;
		total+=fragments[f].groups;
	}
	if (total<=PioneerQ35ArgmaxPeerCount || total>0x00ffffffU) return false;
	program.fragmentCount=fragmentCount;
	program.workerCount=PioneerQ35ArgmaxPeerCount;
	program.groups=unsigned(total);
	program.root={0,fragments[0].firstGroup,1};

	unsigned available[PioneerQ35ArgmaxMaxFragments]={};
	unsigned cores[PioneerQ35ArgmaxMaxFragments]={};
	unsigned remainder[PioneerQ35ArgmaxMaxFragments]={};
	bool awarded[PioneerQ35ArgmaxMaxFragments]={};
	const unsigned remaining=total-1;
	unsigned allocated=0;
	for (unsigned f=0;f<fragmentCount;f++) {
		available[f]=fragments[f].groups-(f==0?1U:0U);
		unsigned share=available[f]*PioneerQ35ArgmaxPeerCount;
		while (share>=remaining) { share-=remaining; cores[f]++; }
		remainder[f]=share;
		allocated+=cores[f];
	}
	while (allocated<PioneerQ35ArgmaxPeerCount) {
		unsigned selected=fragmentCount;
		for (unsigned f=0;f<fragmentCount;f++)
			if (!awarded[f] && available[f]>cores[f]
				&& (selected==fragmentCount || remainder[f]>remainder[selected])) selected=f;
		if (selected==fragmentCount) return false;
		cores[selected]++;
		awarded[selected]=true;
		allocated++;
	}
	unsigned worker=0;
	for (unsigned f=0;f<fragmentCount;f++) {
		if (!cores[f]) continue;
		if (cores[f]>available[f]) return false;
		unsigned base=0,extra=available[f];
		while (extra>=cores[f]) { extra-=cores[f]; base++; }
		unsigned first=fragments[f].firstGroup+(f==0?1U:0U);
		for (unsigned local=0;local<cores[f];local++) {
			const unsigned count=base+unsigned(local<extra);
			if (!count || worker>=PioneerQ35ArgmaxPeerCount) return false;
			program.worker[worker++]={f,first,count};
			first+=count;
		}
		if (first!=fragments[f].firstGroup+fragments[f].groups) return false;
	}
	return worker==PioneerQ35ArgmaxPeerCount;
}

// Translate one tensor's bank-local row groups into a contiguous logical
// output region so same-shaped projections can share a dispatch wave.
inline bool PioneerQ35AppendWorkFragments(const PioneerQ35WorkFragment* parts,
	unsigned partCount,unsigned baseGroup,unsigned tensor,
	PioneerQ35WorkFragment* fragments,unsigned* fragmentTensor,unsigned& fragmentCount)
{
	if (!parts || !partCount || !fragments || !fragmentTensor
		|| fragmentCount>PioneerQ35WorkMaxFragments
		|| partCount>PioneerQ35WorkMaxFragments-fragmentCount) return false;
	for (unsigned part=0;part<partCount;part++) {
		if (!parts[part].groups || parts[part].firstGroup>0xffffffffU-baseGroup
			|| parts[part].groups>0xffffffffU-(baseGroup+parts[part].firstGroup)) return false;
		fragments[fragmentCount]={baseGroup+parts[part].firstGroup,parts[part].groups};
		fragmentTensor[fragmentCount++]=tensor;
	}
	return true;
}

// Fill one dispatch wave from all tensor fragments. A task never crosses a
// bank fragment, but the wave balances independent row groups across every
// ready core instead of launching each bank as a separate small wave.
inline bool PioneerQ35BuildWorkBatch(const PioneerQ35WorkFragment* fragments,
	unsigned fragmentCount,const unsigned* cursor,PioneerQ35WorkBatch& batch)
{
	if (!fragments || !cursor || !fragmentCount || fragmentCount>PioneerQ35WorkMaxFragments) return false;
	batch={};
	unsigned remaining=0;
	for (unsigned fragment=0;fragment<fragmentCount;fragment++) {
		if (!fragments[fragment].groups || cursor[fragment]>fragments[fragment].groups
			|| fragments[fragment].firstGroup>0xffffffffU-fragments[fragment].groups) return false;
		const unsigned available=fragments[fragment].groups-cursor[fragment];
		if (available>0xffffffffU-remaining) return false;
		remaining+=available;
		batch.nextGroup[fragment]=cursor[fragment];
	}
	PioneerQ35WorkSchedule schedule;
	if (!PioneerQ35BuildWorkSchedule(remaining,schedule)) return false;
	unsigned nextFragment=0;
	if (schedule.rootGroups) {
		while (nextFragment<fragmentCount
			&& batch.nextGroup[nextFragment]==fragments[nextFragment].groups) nextFragment++;
		if (nextFragment==fragmentCount) return false;
		batch.rootFragment=nextFragment;
		batch.rootFirstGroup=fragments[nextFragment].firstGroup+batch.nextGroup[nextFragment];
		batch.rootGroups=1;
		batch.nextGroup[nextFragment]++;
		batch.processedGroups=1;
		if (++nextFragment==fragmentCount) nextFragment=0;
	}
	batch.workerCount=schedule.workerCount;
	for (unsigned worker=0;worker<schedule.workerCount;worker++) {
		unsigned fragment=0,attempt=0;
		for (;attempt<fragmentCount;attempt++) {
			fragment=nextFragment+attempt;
			if (fragment>=fragmentCount) fragment-=fragmentCount;
			if (batch.nextGroup[fragment]<fragments[fragment].groups) break;
		}
		if (attempt==fragmentCount) return false;
		const unsigned available=fragments[fragment].groups-batch.nextGroup[fragment];
		const unsigned wanted=schedule.Groups(worker);
		if (!wanted) return false;
		const unsigned count=available<wanted?available:wanted;
		batch.worker[worker]={fragment,
			fragments[fragment].firstGroup+batch.nextGroup[fragment],count};
		batch.nextGroup[fragment]+=count;
		batch.processedGroups+=count;
		nextFragment=fragment+1;
		if (nextFragment==fragmentCount) nextFragment=0;
	}
	return batch.workerCount && batch.processedGroups
		&& batch.processedGroups<=schedule.rootGroups+schedule.scheduledGroups;
}

#endif
