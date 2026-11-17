/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_L1_PAGE_JOURNAL_INSPECTION 1
#define PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 1
#define PIONEER_Q38_FULL_OFFLOAD_BATCHED 1
#define PIONEER_Q38_FULL_OFFLOAD_METRICS 1
#define PIONEER_Q38_FULL_OFFLOAD_CLIENT 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnL1PageAllocator.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnL1OriginalPageJournal.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

struct MockAllocator {
	struct PageTable { unsigned physicalWorker,stateIndex;bool resident;uint32_t pages[2]; };
	bool ready=true,duplicate=false,unaligned=false;
	bool Ready()const{return ready;}
	bool Resolve(unsigned worker,unsigned slot,PageTable& output)const{
		if(worker<1 || worker>3 || slot>=2)return false;
		const uint32_t base=0x90000+slot*8192;
		PageTable candidate={worker,slot,true,{base,base+4096}};
		if(duplicate && slot==1)candidate.pages[0]=0x90000;
		if(unaligned && slot==1)candidate.pages[0]++;
		output=candidate;return true;
	}
};

using Journal=PioneerQ38GdnL1OriginalPageJournal<MockAllocator,3,2,2>;
using ProductionJournal=PioneerQ38GdnL1OriginalPageJournal<PioneerQ38GdnL1PageAllocator,119,25,8>;
static_assert(ProductionJournal::SavedBytes==97484800,"119-peer original L1-page journal is 97,484,800 bytes");

struct MockIO {
	static constexpr unsigned Workers=3,Slots=2,Pages=2,Words=1024;
	std::array<std::array<std::array<uint32_t,Words>,Pages>,Slots> memory[Workers];
	bool held=true,tamperSecondPass=false;
	unsigned reads=0,writes=0,failRead=0,failWrite=0;

	MockIO(){
		for(unsigned w=0;w<Workers;w++)for(unsigned s=0;s<Slots;s++)for(unsigned p=0;p<Pages;p++)
			for(unsigned i=0;i<Words;i++)memory[w][s][p][i]=0x91e10da5U^(w<<24)^(s<<16)^(p<<12)^i;
	}
	bool AllWorkersHeld()const{return held;}
	bool Scope(unsigned worker)const{return held && worker<Workers;}
	bool Decode(unsigned worker,uint32_t address,unsigned& slot,unsigned& page)const{
		if(worker>=Workers || address<0x90000 || address>=0x90000+Slots*Pages*4096 || (address&4095))return false;
		const unsigned index=(address-0x90000)/4096;slot=index/Pages;page=index%Pages;return slot<Slots;
	}
	bool ReadPage(unsigned worker,uint32_t address,uint32_t* output){
		reads++;if(reads==failRead)return false;
		unsigned slot=0,page=0;if(!output || !Decode(worker,address,slot,page))return false;
		if(tamperSecondPass && reads==Slots*Pages*Workers+1)memory[worker][slot][page][0]^=1;
		std::memcpy(output,memory[worker][slot][page].data(),Words*sizeof(uint32_t));return true;
	}
	bool WritePage(unsigned worker,uint32_t address,const uint32_t* input){
		writes++;if(writes==failWrite)return false;
		unsigned slot=0,page=0;if(!input || !Decode(worker,address,slot,page))return false;
		std::memcpy(memory[worker][slot][page].data(),input,Words*sizeof(uint32_t));return true;
	}
	void CorruptAll(){for(auto& w:memory)for(auto& s:w)for(auto& p:s)p.fill(0xdeadbeefU);}
	bool Original(unsigned w,unsigned s,unsigned p,unsigned i)const{
		return memory[w][s][p][i]==(0x91e10da5U^(w<<24)^(s<<16)^(p<<12)^i);
	}
};

static void SuccessAndHeldRetry(){
	MockAllocator allocator;MockIO io;Journal journal;
	io.held=false;assert(!journal.Capture(allocator,io));assert(journal.State()==Journal::Empty && io.reads==0);
	io.held=true;assert(journal.Capture(allocator,io));
	assert(journal.State()==Journal::Captured && io.reads==24 && io.writes==0);
	assert(journal.LastProgress().wordsRead==12*1024 && journal.LastProgress().wordsVerified==12*1024);
	io.CorruptAll();io.held=false;assert(!journal.Restore(io));
	assert(journal.State()==Journal::Captured && io.writes==0);
	io.held=true;assert(journal.Restore(io));
	assert(journal.RestoredSuccessfully() && io.writes==12 && io.reads==48);
	for(unsigned w=0;w<3;w++)for(unsigned s=0;s<2;s++)for(unsigned p=0;p<2;p++)
		for(unsigned i=0;i<1024;i++)assert(io.Original(w,s,p,i));
	const unsigned writes=io.writes;assert(!journal.Restore(io) && io.writes==writes);
}

static void RefuseBadMapBeforeReads(){
	for(unsigned mode=0;mode<3;mode++){
		MockAllocator allocator;if(mode==0)allocator.ready=false;else if(mode==1)allocator.duplicate=true;else allocator.unaligned=true;
		MockIO io;Journal journal;assert(!journal.Capture(allocator,io));
		assert(io.reads==0 && io.writes==0 && journal.State()==Journal::Empty);
	}
}

static void CaptureFailurePoisonsWithoutWrite(){
	MockAllocator allocator;MockIO io;Journal journal;io.failRead=4;
	assert(!journal.Capture(allocator,io));
	assert(journal.State()==Journal::Poisoned && io.reads==4 && io.writes==0);
	assert(!journal.Capture(allocator,io));assert(journal.DiscardPoisonedForInspection());
}

static void RejectChangingSnapshot(){
	MockAllocator allocator;MockIO io;Journal journal;io.tamperSecondPass=true;
	assert(!journal.Capture(allocator,io));
	assert(journal.State()==Journal::Poisoned && io.writes==0 && io.reads==13);
	assert(journal.DiscardPoisonedForInspection());
}

static void RestoreFailureIsSingleAttemptAndRetainsSnapshot(){
	MockAllocator allocator;MockIO io;Journal journal;
	assert(journal.Capture(allocator,io));io.CorruptAll();io.failWrite=3;
	assert(!journal.Restore(io));assert(journal.State()==Journal::Poisoned && io.writes==3);
	const unsigned reads=io.reads,writes=io.writes;
	assert(!journal.Restore(io) && io.reads==reads && io.writes==writes);
	assert(journal.DiscardPoisonedForInspection());
}

static void RestoreBudgetUsesMicrosecondsAndRejectsClockReversal(){
	constexpr uint64_t began=UINT64_C(900000);
	constexpr uint64_t limit=UINT64_C(300000000);
	static_assert(PioneerQ38GdnL1RestoreBudget::LimitUs==limit,"five minutes in system_time microseconds");
	PioneerQ38GdnL1RestoreBudget timeout(began);
	assert(timeout.Observe(began) && timeout.Observe(began+limit-1));
	assert(!timeout.Observe(began+limit) && !timeout.Observe(began));
	PioneerQ38GdnL1RestoreBudget reversed(began);
	assert(reversed.Observe(began+15) && reversed.Observe(began+15));
	assert(!reversed.Observe(began+14) && !reversed.Observe(began+16));
	PioneerQ38GdnL1RestoreBudget wrapped(UINT64_MAX-10);
	assert(wrapped.Observe(UINT64_MAX) && !wrapped.Observe(0));
}

struct TimedRestoreIO {
	MockIO& io;
	PioneerQ38GdnL1RestoreBudget budget{0};
	uint64_t nowUs=0;
	const unsigned expireAfterRead;
	TimedRestoreIO(MockIO& mock,unsigned expire):io(mock),expireAfterRead(expire){}
	bool AllWorkersHeld(){return budget.Observe(nowUs) && io.AllWorkersHeld();}
	bool Scope(unsigned worker){return budget.Observe(nowUs) && io.Scope(worker);}
	bool WritePage(unsigned worker,uint32_t address,const uint32_t* input){
		return budget.Observe(nowUs) && io.WritePage(worker,address,input) && budget.Observe(nowUs);
	}
	bool ReadPage(unsigned worker,uint32_t address,uint32_t* output){
		if(!budget.Observe(nowUs) || !io.ReadPage(worker,address,output))return false;
		if(io.reads==expireAfterRead)nowUs=PioneerQ38GdnL1RestoreBudget::LimitUs;
		return budget.Observe(nowUs);
	}
};

static void TimeoutDuringPageRetainsSnapshotEvenOnFinalRead(){
	for(unsigned expireAfterRead:{25U,48U}){
		MockAllocator allocator;MockIO io;Journal journal;
		assert(journal.Capture(allocator,io));io.CorruptAll();
		TimedRestoreIO access(io,expireAfterRead);
		assert(!journal.Restore(access) && journal.State()==Journal::Poisoned && journal.HasSnapshot());
		const unsigned writes=expireAfterRead==25?1:12;
		assert(io.reads==expireAfterRead && io.writes==writes);
		assert(!journal.Restore(io) && io.reads==expireAfterRead && io.writes==writes);
		assert(journal.DiscardPoisonedForInspection());
	}
}

static void ValidateProduction119PeerMap(){
	auto* allocator=new(std::nothrow) PioneerQ38GdnL1PageAllocator;assert(allocator);
	assert(allocator->Build());auto* journal=new(std::nothrow) ProductionJournal;assert(journal);
	assert(journal->ValidateMapForInspection(*allocator));
	delete journal;delete allocator;
}

int main(){
	SuccessAndHeldRetry();RefuseBadMapBeforeReads();CaptureFailurePoisonsWithoutWrite();
	RejectChangingSnapshot();RestoreFailureIsSingleAttemptAndRetainsSnapshot();ValidateProduction119PeerMap();
	RestoreBudgetUsesMicrosecondsAndRejectsClockReversal();TimeoutDuringPageRetainsSnapshotEvenOnFinalRead();
	std::puts("PASS GDN L1 original-page journal: production 119-peer map/no aliases/97,484,800-byte geometry; synthetic full preflight/capture/verify/held-only single restore/readback/fault retention/role-isolated; NOT native IO, token cache, Work, or speed evidence");
}
