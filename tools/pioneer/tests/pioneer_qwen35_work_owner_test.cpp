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
	bool faultSafe=true;
	bool failFirstFaultProof=false;
	unsigned faultProofCalls=0;

	bool Scope(unsigned expected) const { return reset==expected; }
	bool RootHeld() const { return reset==0x47800; }
	bool Quiet() const { return quiet; }
	unsigned Read(unsigned address) const { return l1.at(address/4); }
	void Write(unsigned address,unsigned word) { l1.at(address/4)=word; }
	unsigned Command(unsigned i) const { return command[i]; }
	void WriteCommand(unsigned i,unsigned value) { command[i]=value; }
	unsigned Pc() const { return pc; }
	void Reset(unsigned value) { reset=value; resets++; }
	void Pause() {}
	unsigned long long Now() { return ++now; }
	bool WorkFaultSafe(unsigned reportedPc,unsigned* reason,unsigned* detail)
	{
		const bool safe=faultSafe && (!failFirstFaultProof || ++faultProofCalls>1)
			&& quiet && (reportedPc==D::WorkFaultPc || reportedPc==D::WorkFaultPc+4)
			&& Read(D::RootCompletionAddress)==D::RootFault;
		if (reason) *reason=safe?0:7;
		if (detail) *detail=safe?0:0x77;
		return safe;
	}
	bool RecoverWorkFault(unsigned* reason,unsigned* detail)
	{
		if (!WorkFaultSafe(Pc(),reason,detail) || reset!=0x46800) return false;
		Reset(0x87ffff);
		Reset(0x47800);
		return true;
	}

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
		Write(D::RootProgressAddress+(10+D::ProgressFullCoreWavesSlot)*4,7);
		Write(D::RootProgressAddress+(10+D::ProgressProjectionWavesSlot)*4,28);
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
		for (unsigned i=0;i<D::RootScaleEvidenceWords;i++)
			assert(!io.Read(D::RootScaleEvidenceAddress+i*4));
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
		assert(reply->progress.reserved[D::ProgressFullCoreWavesSlot]==7
			&& reply->progress.reserved[D::ProgressProjectionWavesSlot]==28);
		reply->progress.reserved[D::ProgressFullCoreWavesSlot]=0;
		assert(!D::ValidWorkResult(*reply,5,2,32,2));
		reply->progress.reserved[D::ProgressFullCoreWavesSlot]=7;
		assert(D::ValidWorkResult(*reply,5,2,32,2));
		assert(!owner->Active() && !owner->Restored() && io.command[5]==5);
	}
	assert(owner->Stop(io) && owner->Restored() && !owner->Unsafe() && io.l1==saved);
	assert(io.resets==8);
	auto failed=std::make_unique<D::WorkOwner>();
	assert(failed->Prepare(io,*prepare));
	request->requestId=1;
	assert(failed->Start(io,*request));
	io.pc=D::WorkFaultPc;
	io.Write(D::RootProgressAddress+10*4,9);
	io.Write(D::RootCheckpointAddress,0x54000300);
	io.Write(D::RootCheckpointAddress+4,119);
	io.Write(D::RootCompletionAddress,D::RootFault);
	reply->requestId=1;
	assert(failed->Poll(io,*reply) && failed->Recoverable() && !failed->Unsafe() && io.resets==10);
	assert(reply->state==D::WorkRecoverableFault);
	assert(reply->pc==D::WorkFaultPc && reply->progress.reserved[0]==9
		&& reply->progress.reserved[1]==D::RootFault
		&& reply->progress.reserved[2]==0x54000300 && reply->progress.reserved[3]==119);
	assert(failed->Stop(io));
	assert(failed->Restored() && !failed->Unsafe() && io.l1==saved);
	assert(io.resets==12);

	Mock pendingIO;
	pendingIO.failFirstFaultProof=true;
	auto pending=std::make_unique<D::WorkOwner>();
	assert(pending->Prepare(pendingIO,*prepare));
	request->requestId=1;
	assert(pending->Start(pendingIO,*request));
	pendingIO.pc=D::WorkFaultPc+4;
	pendingIO.Write(D::RootCompletionAddress,D::RootFault);
	reply->requestId=1;
	assert(pending->Poll(pendingIO,*reply) && pending->Recoverable());
	assert(reply->state==D::WorkRecoveryPending && reply->recoveryProofReason==7
		&& reply->recoveryProofDetail==0x77);
	assert(pending->Stop(pendingIO) && pending->Restored() && !pending->Unsafe());

	Mock changedFaultIO;
	auto changedFault=std::make_unique<D::WorkOwner>();
	assert(changedFault->Prepare(changedFaultIO,*prepare));
	request->requestId=1;
	assert(changedFault->Start(changedFaultIO,*request));
	changedFaultIO.pc=D::WorkFaultPc;
	changedFaultIO.Write(D::RootCompletionAddress,D::RootFault);
	reply->requestId=1;
	assert(changedFault->Poll(changedFaultIO,*reply) && changedFault->Recoverable());
	changedFaultIO.faultSafe=false; // STOP must repeat the fleet-wide safety proof.
	assert(!changedFault->Stop(changedFaultIO) && changedFault->Unsafe()
		&& !changedFault->Restored() && changedFaultIO.reset==0x46800);

	Mock fallbackIO;
	auto failedFallback=std::make_unique<D::WorkOwner>();
	assert(failedFallback->Prepare(fallbackIO,*prepare));
	request->requestId=1;
	assert(failedFallback->Start(fallbackIO,*request));
	fallbackIO.pc=D::WorkFaultPc+4;
	fallbackIO.Write(D::RootProgressAddress+10*4+2*4,D::RootDispatchRejectTag|623);
	fallbackIO.Write(D::RootProgressAddress+10*4+3*4,17);
	fallbackIO.Write(D::RootProgressAddress+10*4+4*4,0x3f800000);
	fallbackIO.Write(D::RootProgressAddress+10*4+5*4,(13U<<24)|2560U);
	const unsigned scaleEvidence[D::RootScaleEvidenceWords]={
		0xaaf6efed,0,0,0,0,0,0,0,0,0xbf61fd40,0xbf61fd40,0xbf61fd40,
		0xbf61fd40,0xbf61fd40,0xbf61fd40};
	for (unsigned i=0;i<D::RootScaleEvidenceWords;i++)
		fallbackIO.Write(D::RootScaleEvidenceAddress+i*4,scaleEvidence[i]);
	fallbackIO.Write(D::RootCheckpointAddress,0x54000000); // stale entry receipt must not mask the rejection
	fallbackIO.Write(D::RootCompletionAddress,D::RootFault);
	fallbackIO.faultSafe=false;
	reply->requestId=1;
	fallbackIO.pc=D::WorkFaultPc+8; // an unknown terminal PC must stay quarantined
	assert(!failedFallback->Poll(fallbackIO,*reply) && failedFallback->Unsafe());
	assert(reply->progress.reserved[2]==(D::RootDispatchRejectTag|623)
		&& reply->progress.reserved[3]==17
		&& reply->progress.reserved[4]==0x3f800000
		&& reply->progress.reserved[5]==((13U<<24)|2560U));
	for (unsigned i=0;i<D::RootScaleEvidenceWords;i++)
		assert(reply->generatedIds[i]==scaleEvidence[i]);
}
