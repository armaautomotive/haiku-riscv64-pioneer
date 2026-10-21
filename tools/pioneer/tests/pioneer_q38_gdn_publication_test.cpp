/* SPDX-License-Identifier: MIT */
// ACTUAL controller/owner lifecycle,EXPLICIT synthetic COUNTER fabric only.
// No RV32/data-zero/native execution claim;the separate actual-image suite
// covers those operations and their corruption/timeouts independently.
#define PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_state_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38InitializeOwnedState.h"
#include <type_traits>
template<class T,class=void> struct CanPublish:std::false_type {};
template<class T> struct CanPublish<T,std::void_t<decltype(std::declval<T&>().PublishInitialization(std::declval<PioneerQ38GdnIdentity>()))>>:std::true_type {};
static void Fill(ZeroControlFixture& f,bool leaveHole)
{
	f.io.publicationCounterFabricOnly=true; assert(f.PrepareZero());
	for (unsigned region=0;region<96;region++) {
		unsigned pages=0; assert(f.state->InitializationPages(f.identity,region,pages) && pages==768);
		for (unsigned first=0;first<pages;first+=256) {
			unsigned count=std::min(256U,pages-first);
			if (leaveHole && region==95 && first==512) count--;
			assert(f.Zero(region,first,count) && !f.state->InitializationPublished(f.identity));
		}
	}
	unsigned pages=0; assert(f.state->InitializationProgress(f.identity,pages) && pages==73728-unsigned(leaveHole));
	assert(!f.state->InitializationPublished(f.identity)); // count alone is NEVER publication
}
int main(int argc,char** argv)
{
	assert(argc==2); setvbuf(stdout,nullptr,_IOLBF,0);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	static_assert(!CanPublish<PioneerQ38GdnBanks>::value,"no public publication bypass");
	{
		ZeroControlFixture f(argv[1]); Fill(f,false); assert(f.CloseZero() && f.state->InitializationPublished(f.identity));
		unsigned touches=f.io.touches; assert(f.CloseZero() && f.io.touches==touches && f.state->InitializationPublished(f.identity));
		for (unsigned field=0;field<4;field++) { auto foreign=f.identity;
			if (!field) foreign.session++; else if (field==1) foreign.sequence++;
			else if (field==2) foreign.modelEpoch++; else foreign.driverGeneration++;
			assert(!f.state->InitializationPublished(foreign)); }
		f.state->Fault(); assert(!f.state->InitializationPublished(f.identity));
	}
	{
		ZeroControlFixture f(argv[1]); Fill(f,true);
		assert(f.CloseZero() && !f.state->InitializationPublished(f.identity)); // physically clean partial END is still legal
	}
	{
		ZeroControlFixture f(argv[1]); Fill(f,true);
		assert(f.Zero(95,767,1) && !f.state->InitializationPublished(f.identity));
		assert(f.CloseZero() && f.state->InitializationPublished(f.identity)); // LAST hole only,then actual CLOSE
	}
	{
		ZeroControlFixture f(argv[1]); Fill(f,false); unsigned touches=f.io.touches;
		assert(!f.CloseZero(false) && !f.state->InitializationPublished(f.identity) && f.io.touches==touches); f.Unsafe();
	}
	{
		ZeroControlFixture f(argv[1]); Fill(f,false); f.io.othersHeld=false;
		assert(!f.CloseZero() && !f.state->InitializationPublished(f.identity)); f.Unsafe();
	}
	for (bool striped:{false,true}) {
		ZeroControlFixture f(argv[1],striped); f.io.publicationCounterFabricOnly=true;
		PioneerQ38InitializeOwnedResult result;
		assert(PioneerQ38InitializeOwnedState(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.identity,
			f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.lease,result));
		assert(result.pages==73728 && result.regions==(striped?672U:96U) && result.jobs==(striped?672U:288U)
			&& result.closed && result.published && !result.recovery_required && f.state->InitializationPublished(f.identity));
		unsigned touches=f.io.touches;
		assert(!PioneerQ38InitializeOwnedState(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.identity,
			f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.lease,result) && f.io.touches==touches);
		for (auto page:f.initialized) assert(!page);
	}
	for (unsigned fault:{1U,144U,288U}) {
		ZeroControlFixture f(argv[1]); f.io.publicationCounterFabricOnly=true; f.io.publicationFailJob=fault;
		PioneerQ38InitializeOwnedResult result;
		assert(!PioneerQ38InitializeOwnedState(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.identity,
			f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.lease,result));
		assert(result.recovery_required && !result.published && !result.closed && result.jobs==fault-1
			&& f.io.rootStarts==fault && !f.state->InitializationPublished(f.identity)); f.Unsafe();
	}
	puts("PASS OFFLINE ACTUAL private initialization publication:full73728-page count alone NEVER publishes,ONLY known-clean physical CLOSE contract does,last-hole/partial CLOSE/foreign epochs/unsafe CLOSE/source fault/sticky fault/repeated CLOSE verified;SYNTHETIC_COUNTER_FABRIC_ONLY_NO_DATA_ZERO_OR_RV32_NATIVE_CLAIM;NOT compute/read/model authority");
	puts("PASS OFFLINE ACTUAL whole-owned-state initializer:96/672-region geometry,288/672 bounded jobs,ALL73728 pages before CLOSE/publication,repeated attempt inert,first/middle/last job timeouts stop WITHOUT CLOSE/publication/later IO,learned bitmap untouched;SYNTHETIC_COUNTER_FABRIC_ONLY_NOT_NATIVE_OR_COMPUTE_READY");
}
