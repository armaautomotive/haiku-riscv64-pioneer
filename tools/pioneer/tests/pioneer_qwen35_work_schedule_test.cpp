/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <vector>
#include "../PioneerQwen35WorkSchedule.h"

int main()
{
	PioneerQ35WorkSchedule schedule{};
	assert(!PioneerQ35BuildWorkSchedule(0,schedule));
	for (unsigned remaining=1;remaining<=20000;remaining++) {
		assert(PioneerQ35BuildWorkSchedule(remaining,schedule));
		const unsigned root=remaining>1?1:0;
		const unsigned available=remaining-root;
		const unsigned workers=available<119?available:119;
		const unsigned scheduled=available>workers*16?workers*16:available;
		assert(schedule.rootGroups==root && schedule.workerCount==workers
			&& schedule.scheduledGroups==scheduled);
		assert(schedule.rootGroups+schedule.workerCount==(remaining<120?remaining:120));
		unsigned total=0;
		for (unsigned worker=0;worker<119;worker++) {
			const unsigned count=schedule.Groups(worker);
			if (worker<workers) assert(count && count<=16);
			else assert(!count);
			total+=count;
		}
		assert(total==scheduled && root+total<=remaining);
		if (remaining>=120) assert(schedule.rootGroups==1 && schedule.workerCount==119);
	}
	assert(PioneerQ35BuildWorkSchedule(160,schedule));
	assert(schedule.rootGroups==1 && schedule.workerCount==119
		&& schedule.groupsPerWorker==1 && schedule.extraWorkers==40);
	assert(PioneerQ35BuildWorkSchedule(1922,schedule));
	assert(schedule.rootGroups==1 && schedule.workerCount==119
		&& schedule.scheduledGroups==1904 && schedule.groupsPerWorker==16
		&& schedule.extraWorkers==0);

	const unsigned totals[]={1,2,31,32,119,120,160,576,15520};
	for (unsigned total:totals) for (unsigned fragmentsCount=1;fragmentsCount<=14;fragmentsCount++) {
		PioneerQ35WorkFragment fragments[14]={};
		unsigned cursor[14]={},first=0;
		for (unsigned fragment=0;fragment<fragmentsCount;fragment++) {
			const unsigned groups=total/fragmentsCount+unsigned(fragment<total%fragmentsCount);
			if (!groups) break;
			fragments[fragment]={first,groups};
			first+=groups;
		}
		unsigned actualFragments=fragmentsCount;
		while (actualFragments && !fragments[actualFragments-1].groups) actualFragments--;
		assert(first==total && actualFragments);
		std::vector<unsigned> visits(total,0);
		unsigned completed=0;
		while (completed<total) {
			PioneerQ35WorkBatch batch{};
			assert(PioneerQ35BuildWorkBatch(fragments,actualFragments,cursor,batch));
			const unsigned remaining=total-completed;
			unsigned fragmentMask=0;
			assert(batch.workerCount==(remaining-(remaining>1?1:0)<119
				?remaining-(remaining>1?1:0):119));
			if (remaining>=120) assert(batch.rootGroups==1 && batch.workerCount==119);
			if (batch.rootGroups) {
				assert(batch.rootFragment<actualFragments);
				fragmentMask|=1U<<batch.rootFragment;
				const unsigned group=batch.rootFirstGroup;
				assert(group>=fragments[batch.rootFragment].firstGroup
					&& group<fragments[batch.rootFragment].firstGroup+fragments[batch.rootFragment].groups);
				assert(visits[group]++==0);
			}
			for (unsigned worker=0;worker<batch.workerCount;worker++) {
				const auto& task=batch.worker[worker];
				assert(task.fragment<actualFragments && task.groups && task.groups<=16);
				fragmentMask|=1U<<task.fragment;
				const auto& fragment=fragments[task.fragment];
				assert(task.firstGroup>=fragment.firstGroup
					&& task.groups<=fragment.firstGroup+fragment.groups-task.firstGroup);
				for (unsigned group=0;group<task.groups;group++)
					assert(visits[task.firstGroup+group]++==0);
			}
			if (!completed && remaining>=120 && actualFragments==14)
				assert(fragmentMask==0x3fff);
			for (unsigned fragment=0;fragment<actualFragments;fragment++) {
				assert(batch.nextGroup[fragment]>=cursor[fragment]
					&& batch.nextGroup[fragment]<=fragments[fragment].groups);
				cursor[fragment]=batch.nextGroup[fragment];
			}
			completed+=batch.processedGroups;
			assert(completed<=total);
		}
		for (unsigned visitsForGroup:visits) assert(visitsForGroup==1);
	}

	PioneerQ35WorkFragment composite[14]={},local[7]={};
	unsigned compositeTensor[14]={},compositeCount=0,baseGroup=0;
	for (unsigned tensor=0;tensor<2;tensor++) {
		unsigned firstGroup=0;
		for (unsigned part=0;part<7;part++) {
			const unsigned groups=576/7+unsigned(part<576%7);
			local[part]={firstGroup,groups};
			firstGroup+=groups;
		}
		assert(firstGroup==576);
		assert(PioneerQ35AppendWorkFragments(local,7,baseGroup,tensor,
			composite,compositeTensor,compositeCount));
		baseGroup+=576;
	}
	assert(compositeCount==14 && baseGroup==1152);
	for (unsigned i=0;i<compositeCount;i++) {
		assert(compositeTensor[i]==i/7);
		assert(composite[i].firstGroup==(i%7==0?(i/7)*576:composite[i-1].firstGroup+composite[i-1].groups));
	}
	unsigned compositeCursor[14]={},compositeDone=0;
	std::vector<unsigned> compositeVisits(1152,0);
	while (compositeDone<1152) {
		PioneerQ35WorkBatch batch{};
		assert(PioneerQ35BuildWorkBatch(composite,compositeCount,compositeCursor,batch));
		for (unsigned worker=0;worker<batch.workerCount;worker++) {
			const auto& task=batch.worker[worker];
			for (unsigned group=0;group<task.groups;group++)
				assert(compositeVisits[task.firstGroup+group]++==0);
		}
		if (batch.rootGroups) assert(compositeVisits[batch.rootFirstGroup]++==0);
		for (unsigned i=0;i<compositeCount;i++) compositeCursor[i]=batch.nextGroup[i];
		compositeDone+=batch.processedGroups;
	}
	for (unsigned visitsForGroup:compositeVisits) assert(visitsForGroup==1);
}
