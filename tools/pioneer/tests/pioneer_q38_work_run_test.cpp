/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_DISPATCH_CLOSE_MOCK_ONLY
#include "pioneer_q38_dispatch_close_test.cpp"
#include "../PioneerQ38WorkSession.h"
static bool Work(RunFixture& f) { return PioneerQ38DispatchRun<RunIO,true>(f.io,f.dram,f.banks,*f.lease,f.initialized.data(),unsigned(f.initialized.size()),*f.job
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
	,&f.io.dmaInput,&f.io.dmaBulk,&f.io.dmaWide
#endif
); }
int main(int argc,char** argv)
{
	assert(argc==2);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	for (unsigned workers:{1U,7U,119U}) for (unsigned type:{8U,12U,14U}) {
		RunFixture f(argv[1],workers,type,5120);
		assert(Work(f) && f.io.inputMode==1 && f.lease->inputCacheWork);
		PioneerQ38WorkHistory history; assert(history.Accept(*f.job) && !history.Accept(*f.job));
		for (const auto& peer:f.job->result.peer) for (auto word:peer) assert(!word);
		assert(Work(f) && f.io.inputMode==2 && history.Accept(*f.job));
		assert(f.Run() && f.io.inputMode==1 && !f.lease->inputCacheWork);
		assert(f.Run() && f.io.inputMode==2);
		assert(Work(f) && f.io.inputMode==1 && f.lease->inputCacheWork);
		assert(Work(f) && f.io.inputMode==2);
	}
	// Optional exhaustive readbacks are deliberately absent in WORK. CPU
	// validation is a separate mode;WORK must not silently perform it.
	for (unsigned fault:{3U,5U,6U}) {
		RunFixture f(argv[1],7); f.io.rootFault=fault; f.io.workOptionalChecks=true;
		assert(Work(f) && !f.lease->unsafe && !f.banks.Quarantined());
		if (fault==6) {
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
			assert(f.job->output[6][0]!=0x7f800000); // post-completion PIO mutation cannot change DMA output
#else
			assert(f.job->output[6][0]==0x7f800000);
#endif
		}
	}
	// Root/source/counter/terminal/hold failures remain mandatory safety.
	for (unsigned fault:{1U,2U,4U,7U,8U,9U,10U,11U}) {
		RunFixture f(argv[1],7); f.io.rootFault=fault;
		assert(!Work(f) && f.lease->unsafe && f.banks.Quarantined());
		for (const auto& rows:f.job->output) for (auto word:rows) assert(!word);
		unsigned writes=f.io.Writes(),selects=f.io.selects;
		assert(!Work(f) && !f.Run() && !PioneerQ38DispatchClose(f.io,f.dram,f.banks,*f.lease,false));
		assert(writes==f.io.Writes() && selects==f.io.selects);
	}
	{
		CloseFixture f(argv[1]);
		assert((PioneerQ38DispatchRun<CloseIO,true>(f.io,f.dram,f.banks,*f.lease,f.initialized.data(),unsigned(f.initialized.size()),*f.job
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
			,&f.io.dmaInput,&f.io.dmaBulk,&f.io.dmaWide
#endif
		)));
		f.io.closing=true; assert(f.Close() && f.io.endHolds==120 && f.io.restored==120);
	}
	puts("PASS WORK:separate operation,NO peer/input/numeric readback validation,mandatory safety failure quarantine+CLOSE ONLY,Work/Checked cache provenance isolated;NO DEVICE IO");
}
