/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_L1_PAGE_JOURNAL_INSPECTION 1
#define PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 1
#define PIONEER_Q38_FULL_OFFLOAD_BATCHED 1
#define PIONEER_Q38_FULL_OFFLOAD_METRICS 1
#define PIONEER_Q38_FULL_OFFLOAD_CLIENT 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnL1PageAllocator.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnL1OriginalPageJournal.h"

using Journal=PioneerQ38GdnL1OriginalPageJournal<PioneerQ38GdnL1PageAllocator,119,25,8>;
static_assert(Journal::SavedBytes==97484800,"full 119-peer L1 original snapshot is heap-only");

struct InspectionPageIO {
	bool AllWorkersHeld()const;
	bool Scope(unsigned worker)const;
	bool ReadPage(unsigned worker,uint32_t address,uint32_t* words);
	bool WritePage(unsigned worker,uint32_t address,const uint32_t* words);
};

extern "C" bool pioneer_q38_gdn_l1_journal_compile_only(
	Journal& journal,const PioneerQ38GdnL1PageAllocator& allocator,InspectionPageIO& io)
{
	return journal.Capture(allocator,io) && journal.Restore(io);
}
