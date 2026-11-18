/* SPDX-License-Identifier: MIT */
// Exercise the actual kernel owner against a deterministic L1 and NIU mock.
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35WorkOwner.h"

namespace D=PioneerQwen35;

struct Mock {
	std::vector<unsigned> l1=std::vector<unsigned>(0x180000/4,0x11223344);
	unsigned command[10]={0,1,2,3,4,5,6,7,8,9};
	unsigned reset=0x47800, resets=0, pc=0;
	unsigned long long now=0;
	bool quiet=true;

	bool Scope(unsigned expected) const { return reset==expected; }
	bool AllHeld() const { return reset==0x47800; }
	bool Quiet() const { return quiet; }
	unsigned Read(unsigned address) const { return l1.at(address/4); }
	void Write(unsigned address,unsigned word) { l1.at(address/4)=word; }
	unsigned Command(unsigned i) const { return command[i]; }
	void WriteCommand(unsigned i,unsigned value) { command[i]=value; }
	unsigned Pc() const { return pc; }
	void Reset(unsigned value) { reset=value; resets++; }
	void Pause() {}
	unsigned long long Now() { return ++now; }

	void Prefill(unsigned id)
	{
		Write(D::RootProgressAddress,D::RootProgressMagic);
		Write(D::RootProgressAddress+3*4,5);
		Write(D::RootProgressAddress+4*4,1000000);
		Write(D::RootProgressAddress+9*4,id);
	}

	void Finish()
	{
		Write(D::RootProgressAddress+1*4,6);
		Write(D::RootProgressAddress+2*4,2);
		Write(D::RootProgressAddress+6*4,2000000);
		Write(D::RootProgressAddress+8*4,1);
		Write(D::RootOutputAddress,42);
		Write(D::RootOutputAddress+4,13);
		Write(D::RootCompletionAddress+4,2);
		Write(D::RootCompletionAddress,D::RootDone);
		pc=D::WorkParkPc;
	}
};

int main()
{
	auto prepare=std::make_unique<D::WorkPrepare>();
	prepare->version=D::WorkVersion;
	prepare->manifest.context=32;
	prepare->rope={0,0x08000000,32*D::Rotary*4};
	auto request=std::make_unique<D::WorkStart>();
	request->version=D::WorkVersion;
	request->promptCount=5;
	request->maxOutput=2;
	auto reply=std::make_unique<D::WorkPoll>();
	auto owner=std::make_unique<D::WorkOwner>();
	Mock io;
	const auto saved=io.l1;
	assert(owner->Prepare(io,*prepare));
	assert(!owner->Restored() && !owner->Active());
	request->requestId=1;
	for (unsigned run=1; run<=2; run++) {
		request->requestId=run+1;
		assert(!owner->Start(io,*request));
		request->requestId=run;
		assert(owner->Start(io,*request) && owner->Active());
		assert(!owner->Stop(io) && io.command[5]==129 && !io.Read(D::RootCompletionAddress));
		assert(!io.Read(D::RootProgressAddress));
		reply->version=D::WorkVersion;
		reply->requestId=run;
		assert(owner->Poll(io,*reply) && reply->state==D::WorkRunning);
		io.Prefill(run);
		assert(owner->Poll(io,*reply) && reply->state==D::WorkRunning);
		io.Finish();
		assert(owner->Poll(io,*reply) && reply->state==D::WorkComplete);
		assert(reply->startMicros<reply->prefillObservedMicros
			&& reply->prefillObservedMicros<reply->completeObservedMicros);
		assert(reply->generatedIds[0]==42 && reply->generatedIds[1]==13);
		assert(!owner->Active() && !owner->Restored() && io.command[5]==5);
	}
	assert(owner->Stop(io) && owner->Restored() && !owner->Unsafe() && io.l1==saved);
	assert(io.resets==8);
	auto failed=std::make_unique<D::WorkOwner>();
	assert(failed->Prepare(io,*prepare));
	request->requestId=1;
	assert(failed->Start(io,*request));
	io.Write(D::RootCompletionAddress,D::RootFault);
	reply->requestId=1;
	assert(!failed->Poll(io,*reply) && failed->Unsafe() && io.resets==10);
	assert(!failed->Stop(io) && !failed->Restored());
}
