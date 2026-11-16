/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_L1_ORIGINAL_PAGE_JOURNAL_H
#define PIONEER_Q38_GDN_L1_ORIGINAL_PAGE_JOURNAL_H

#ifdef PIONEER_Q38_GDN_L1_NATIVE_PAGE_ACCESS_CANDIDATE
#include "PioneerQ38GdnL1NativePageAccessRole.h"
#else
#if !defined(PIONEER_Q38_GDN_L1_PAGE_JOURNAL_INSPECTION) \
	|| PIONEER_Q38_GDN_L1_PAGE_JOURNAL_INSPECTION != 1 \
	|| defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
	|| defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
	|| defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
	|| defined(PIONEER_DRIVER_BUILD_TAG) || defined(_KERNEL_MODE)
#error GDN L1 original-page journal is a standalone offline inspection component
#endif
#endif

#include <new>
#include <stddef.h>
#include <stdint.h>

// Transactional snapshot primitive for the scattered recurrent-state pages.
// The IO adapter must bind these calls to the same native session/FD and must
// independently prove that the requested worker is held and its L1 mapping is
// stable. This class grants no device access, launch, token-commit or Work
// authority. A poisoned journal deliberately retains its heap snapshot.
template<class Allocator,unsigned Workers,unsigned ResidentSlots,unsigned PagesPerSlot>
class PioneerQ38GdnL1OriginalPageJournal {
public:
	enum : unsigned {
		PageBytes=4096,WordsPerPage=PageBytes/sizeof(uint32_t),
		Pages=Workers*ResidentSlots*PagesPerSlot,
		SavedWords=Pages*WordsPerPage,
		SavedBytes=SavedWords*sizeof(uint32_t)
	};
	enum Phase { Empty, Capturing, Captured, Restoring, Restored, Poisoned, AbandonedForInspection };
	struct Progress {
		unsigned phase=Empty,worker=0,slot=0,page=0,wordsRead=0,wordsRestored=0,wordsVerified=0;
	};

private:
	uint32_t fAddress[Workers][ResidentSlots][PagesPerSlot] = {};
	uint32_t* fSaved=nullptr;
	Phase fPhase=Empty;
	bool fRestoreAttempted=false,fOriginalsVerified=false;
	Progress fProgress={};

	bool BuildMap(const Allocator& allocator)
	{
		if(!allocator.Ready())return false;
		for(unsigned worker=0;worker<Workers;worker++){
			for(unsigned slot=0;slot<ResidentSlots;slot++){
				typename Allocator::PageTable table={};
				if(!allocator.Resolve(worker+1,slot,table) || !table.resident
					|| table.physicalWorker!=worker+1 || table.stateIndex!=slot)return false;
				for(unsigned page=0;page<PagesPerSlot;page++){
					const uint32_t address=table.pages[page];
					if((address&(PageBytes-1)) || address<0x90000
						|| address>0x180000-PageBytes)return false;
					for(unsigned prior=0;prior<page;prior++)
						if(table.pages[prior]==address)return false;
					for(unsigned priorSlot=0;priorSlot<slot;priorSlot++)
						for(unsigned priorPage=0;priorPage<PagesPerSlot;priorPage++)
							if(fAddress[worker][priorSlot][priorPage]==address)return false;
					fAddress[worker][slot][page]=address;
				}
			}
		}
		return true;
	}

public:
	PioneerQ38GdnL1OriginalPageJournal()=default;
	PioneerQ38GdnL1OriginalPageJournal(const PioneerQ38GdnL1OriginalPageJournal&)=delete;
	PioneerQ38GdnL1OriginalPageJournal& operator=(const PioneerQ38GdnL1OriginalPageJournal&)=delete;
	~PioneerQ38GdnL1OriginalPageJournal()
	{
		// Never free the only original-data copy after an uncertain snapshot or
		// restore. Native owners must retain this object on every poisoned path.
		if(fPhase==Empty || fPhase==Restored || fPhase==AbandonedForInspection)delete[] fSaved;
	}

	Phase State()const{return fPhase;}
	bool HasSnapshot()const{return fOriginalsVerified;}
	bool RestoredSuccessfully()const{return fPhase==Restored;}
	bool Unsafe()const{return fPhase==Poisoned || fPhase==Capturing || fPhase==Restoring;}
	const Progress& LastProgress()const{return fProgress;}
#ifdef PIONEER_Q38_GDN_L1_PAGE_JOURNAL_INSPECTION
	bool ValidateMapForInspection(const Allocator& allocator)
	{
		return fPhase==Empty && BuildMap(allocator);
	}
#endif

	// IO contract: Scope(worker) proves the caller's exact held-session and
	// stable-L1 mapping invariants; ReadPage reads exactly 4096 bytes from that
	// worker-local L1 address. All pages are mapped and allocated before the
	// first read. A complete second pass verifies the original snapshot before
	// this object reports Captured (and therefore before it may be used).
	template<class IO> bool Capture(const Allocator& allocator,IO& io)
	{
		if(fPhase!=Empty || !BuildMap(allocator) || !io.AllWorkersHeld())return false;
		uint32_t* saved=new(std::nothrow) uint32_t[SavedWords];
		uint32_t* verify=new(std::nothrow) uint32_t[WordsPerPage];
		if(!saved || !verify){delete[] verify;delete[] saved;return false;}
		fSaved=saved;fPhase=Capturing;fProgress.phase=fPhase;
		if(!io.AllWorkersHeld()){
			delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
		}
		for(unsigned worker=0;worker<Workers;worker++)for(unsigned slot=0;slot<ResidentSlots;slot++)
			for(unsigned page=0;page<PagesPerSlot;page++){
				fProgress.worker=worker;fProgress.slot=slot;fProgress.page=page;
				const uint32_t address=fAddress[worker][slot][page];
				uint32_t* destination=fSaved+(((worker*ResidentSlots+slot)*PagesPerSlot+page)*WordsPerPage);
				if(!io.Scope(worker) || !io.ReadPage(worker,address,destination)){
					delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
				}
				fProgress.wordsRead+=WordsPerPage;
			}
		// Verify all saved originals before any subsequent owner is allowed to
		// write an inference image or state into these pages.
		for(unsigned worker=0;worker<Workers;worker++)for(unsigned slot=0;slot<ResidentSlots;slot++)
			for(unsigned page=0;page<PagesPerSlot;page++){
				fProgress.worker=worker;fProgress.slot=slot;fProgress.page=page;
				const uint32_t address=fAddress[worker][slot][page];
				const uint32_t* original=fSaved+(((worker*ResidentSlots+slot)*PagesPerSlot+page)*WordsPerPage);
				if(!io.Scope(worker) || !io.ReadPage(worker,address,verify)){
					delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
				}
				for(unsigned word=0;word<WordsPerPage;word++){
					if(verify[word]!=original[word]){
						delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
					}
					fProgress.wordsVerified++;
				}
			}
		delete[] verify;fOriginalsVerified=true;fPhase=Captured;fProgress.phase=fPhase;return true;
	}

	// Restore is single-attempt after the caller confirms the complete worker
	// cohort is held. It writes each original page once, verifies immediately,
	// then performs a full second verification. Any failure poisons the journal;
	// callers must retain this object and must not retry or drop the snapshot.
	template<class IO> bool Restore(IO& io)
	{
		if(fPhase!=Captured || fRestoreAttempted || !io.AllWorkersHeld())return false;
		fRestoreAttempted=true;fPhase=Restoring;fProgress.phase=fPhase;
		uint32_t* verify=new(std::nothrow) uint32_t[WordsPerPage];
		if(!verify){fPhase=Poisoned;fProgress.phase=fPhase;return false;}
		for(unsigned worker=0;worker<Workers;worker++)for(unsigned slot=0;slot<ResidentSlots;slot++)
			for(unsigned page=0;page<PagesPerSlot;page++){
				fProgress.worker=worker;fProgress.slot=slot;fProgress.page=page;
				const uint32_t address=fAddress[worker][slot][page];
				const uint32_t* original=fSaved+(((worker*ResidentSlots+slot)*PagesPerSlot+page)*WordsPerPage);
				if(!io.Scope(worker) || !io.WritePage(worker,address,original)){
					delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
				}
				fProgress.wordsRestored+=WordsPerPage;
				if(!io.Scope(worker) || !io.ReadPage(worker,address,verify)){
					delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
				}
				for(unsigned word=0;word<WordsPerPage;word++)if(verify[word]!=original[word]){
					delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
				}
			}
		for(unsigned worker=0;worker<Workers;worker++)for(unsigned slot=0;slot<ResidentSlots;slot++)
			for(unsigned page=0;page<PagesPerSlot;page++){
				fProgress.worker=worker;fProgress.slot=slot;fProgress.page=page;
				const uint32_t address=fAddress[worker][slot][page];
				const uint32_t* original=fSaved+(((worker*ResidentSlots+slot)*PagesPerSlot+page)*WordsPerPage);
				if(!io.Scope(worker) || !io.ReadPage(worker,address,verify)){
					delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
				}
				for(unsigned word=0;word<WordsPerPage;word++)if(verify[word]!=original[word]){
					delete[] verify;fPhase=Poisoned;fProgress.phase=fPhase;return false;
				}
			}
		delete[] verify;fPhase=Restored;fProgress.phase=fPhase;return true;
	}

#ifdef PIONEER_Q38_GDN_L1_PAGE_JOURNAL_INSPECTION
	// Test-only escape hatch for releasing a deliberately retained poisoned
	// snapshot after assertions. This symbol cannot exist in native/Work roles.
	bool DiscardPoisonedForInspection()
	{
		if(fPhase!=Poisoned)return false;
		delete[] fSaved;fSaved=nullptr;fPhase=AbandonedForInspection;fProgress.phase=fPhase;return true;
	}
#endif
};

static_assert(1*1*1*(4096/sizeof(uint32_t))==1024,
	"one exact 4-KiB page journal geometry");

#endif
