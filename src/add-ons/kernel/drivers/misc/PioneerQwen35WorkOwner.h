/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_WORK_OWNER_H
#define PIONEER_QWEN35_WORK_OWNER_H

#include "PioneerQwen35WorkProtocol.h"
#include "PioneerQwen35WorkImage.h"

namespace PioneerQwen35 {

// Sole writer for the ROOT while its private seven-bank session is resident.
// All methods run under the same card lease and driver lock.
class WorkOwner {
	enum Phase { Empty, Held, Running, RecoverableFault, Closed, Fault };
	static constexpr unsigned HeldReset=0x47800, ArmReset=0x87ffff, RunReset=0x46800;
	static constexpr unsigned L1Words=(RootEnd-RootTextAddress)/4;
	static constexpr unsigned CheckpointWords=4;
	static constexpr unsigned SavedWords=1+L1Words+CheckpointWords+RootCompletionWords;
	static constexpr unsigned TextWords=sizeof(WorkTextWords)/sizeof(*WorkTextWords);
	static constexpr unsigned ManifestWords=sizeof(DeviceLayout)/4;

	Phase phase=Empty;
	Range rope{};
	unsigned context=0, requestId=0, promptCount=0, maxOutput=0;
	unsigned long long began=0, prefillObserved=0;
	unsigned original[SavedWords];
	unsigned originalCommand[10];
	unsigned manifest[ManifestWords];

	static unsigned Address(unsigned word)
	{
		if (!word) return RootBootAddress;
		if (word<=L1Words) return RootTextAddress+(word-1)*4;
		if (word<=L1Words+CheckpointWords)
			return RootCheckpointAddress+(word-1-L1Words)*4;
		return RootCompletionAddress+(word-1-L1Words-CheckpointWords)*4;
	}

	template<class IO>
	bool Installed(IO& io) const
	{
		if (io.Read(RootBootAddress)!=WorkBootWord) return false;
		for (unsigned i=0; i<TextWords; i++)
			if (io.Read(RootTextAddress+i*4)!=WorkTextWords[i]) return false;
		for (unsigned i=0; i<ManifestWords; i++)
			if (io.Read(RootLayoutAddress+i*4)!=manifest[i]) return false;
		return true;
	}

	bool Fail() { phase=Fault; return false; }

public:
	WorkOwner()=default;
	WorkOwner(const WorkOwner&)=delete;
	WorkOwner& operator=(const WorkOwner&)=delete;

	bool Restored() const { return phase==Closed; }
	bool Unsafe() const { return phase==Fault; }
	bool Recoverable() const { return phase==RecoverableFault; }
	bool Active() const { return phase==Running; }
	unsigned RequestId() const { return requestId; }
	bool CanStart(const WorkStart& start) const
	{
		return phase==Held && ValidWorkStart(start,context,requestId+1);
	}

	template<class IO>
	bool Prepare(IO& io, const WorkPrepare& prepare)
	{
		if (phase!=Empty || !io.Scope(HeldReset) || !io.RootHeld() || !io.Quiet()) return Fail();
		context=prepare.manifest.context;
		rope=prepare.rope;
		memcpy(manifest,&prepare.manifest,sizeof(manifest));
		for (unsigned i=0; i<SavedWords; i++) original[i]=io.Read(Address(i));
		for (unsigned i=0; i<10; i++) originalCommand[i]=io.Command(i);
		if (!io.Scope(HeldReset) || !io.Quiet()) return Fail();
		for (unsigned i=0; i<SavedWords; i++)
			if (io.Read(Address(i))!=original[i]) return Fail();
		for (unsigned i=0; i<10; i++)
			if (io.Command(i)!=originalCommand[i]) return Fail();
		for (unsigned i=0; i<TextWords; i++) io.Write(RootTextAddress+i*4,WorkTextWords[i]);
		for (unsigned i=0; i<ManifestWords; i++) io.Write(RootLayoutAddress+i*4,manifest[i]);
		io.Write(RootBootAddress,WorkBootWord);
		if (!io.Scope(HeldReset) || !io.Quiet() || !Installed(io)) return Fail();
		phase=Held;
		return true;
	}

	template<class IO>
	bool Start(IO& io, const WorkStart& start)
	{
		if (!CanStart(start)) return false;
		if (!io.Scope(HeldReset) || !io.RootHeld() || !io.Quiet() || !Installed(io)) return Fail();
		const RootControl control={RootBootMagic,start.promptCount,start.maxOutput,context,
			EndOfText,EndOfTurn,rope,start.requestId,{}};
		unsigned words[sizeof(control)/4];
		memcpy(words,&control,sizeof(control));
		for (unsigned i=0; i<sizeof(words)/4; i++) io.Write(RootControlAddress+i*4,words[i]);
		for (unsigned i=0; i<sizeof(RequestProgress)/4; i++) io.Write(RootProgressAddress+i*4,0);
		for (unsigned i=0; i<RootScaleEvidenceWords; i++) io.Write(RootScaleEvidenceAddress+i*4,0);
		for (unsigned i=0; i<start.promptCount; i++) io.Write(RootPromptAddress+i*4,start.prompt[i]);
		for (unsigned i=0; i<RootCompletionWords; i++) io.Write(RootCompletionAddress+i*4,0);
		for (unsigned i=0; i<sizeof(words)/4; i++)
			if (io.Read(RootControlAddress+i*4)!=words[i]) return Fail();
		for (unsigned i=0; i<sizeof(RequestProgress)/4; i++)
			if (io.Read(RootProgressAddress+i*4)) return Fail();
		for (unsigned i=0; i<RootScaleEvidenceWords; i++)
			if (io.Read(RootScaleEvidenceAddress+i*4)) return Fail();
		for (unsigned i=0; i<start.promptCount; i++)
			if (io.Read(RootPromptAddress+i*4)!=start.prompt[i]) return Fail();
		for (unsigned i=0; i<RootCompletionWords; i++)
			if (io.Read(RootCompletionAddress+i*4)) return Fail();
		io.WriteCommand(5,129);
		if (io.Command(5)!=129 || !io.Scope(HeldReset) || !io.Quiet()) return Fail();
		promptCount=start.promptCount;
		maxOutput=start.maxOutput;
		requestId=start.requestId;
		io.Reset(ArmReset);
		io.Pause();
		if (!io.Scope(ArmReset) || !io.Quiet()) return Fail();
		phase=Running;
		prefillObserved=0;
		began=io.Now();
		io.Reset(RunReset);
		if (!io.Scope(RunReset)) return Fail();
		return true;
	}

	template<class IO>
	bool Poll(IO& io, WorkPoll& result)
	{
		if (!Active() || result.requestId!=requestId) return false;
		if (!io.Scope(RunReset)) return Fail();
		const unsigned marker=io.Read(RootCompletionAddress);
		const auto observed=io.Now();
		result.pc=io.Pc();
		const unsigned statusWords=sizeof(RequestProgress)/4;
		unsigned progress[statusWords];
		for (unsigned i=0; i<statusWords; i++) progress[i]=io.Read(RootProgressAddress+i*4);
		memcpy(&result.progress,progress,sizeof(progress));
		// Prefer the explicit request-cleared terminal marker: some fault
		// paths report the parked ROOT PC one instruction past WorkFaultPc.
		if (marker==RootFault || (result.pc==WorkFaultPc && marker!=RootDone)) {
			result.progress.reserved[1]=marker;
			for (unsigned i=0; i<RootScaleEvidenceWords; i++)
				result.generatedIds[i]=io.Read(RootScaleEvidenceAddress+i*4);
			const unsigned failureStage=result.progress.reserved[2];
			const unsigned failureDetail=result.progress.reserved[3];
			if ((failureStage&RootDispatchRejectMask)!=RootDispatchRejectTag) {
				const unsigned checkpointStage=io.Read(RootCheckpointAddress);
				const unsigned checkpointDetail=io.Read(RootCheckpointAddress+4);
				if (checkpointStage || checkpointDetail) {
					result.progress.reserved[2]=checkpointStage;
					result.progress.reserved[3]=checkpointDetail;
				} else {
					result.progress.reserved[2]=failureStage;
					result.progress.reserved[3]=failureDetail;
				}
			}
		}
		if (!prefillObserved && result.progress.prefilled==promptCount) prefillObserved=observed;
		result.startMicros=began;
		result.prefillObservedMicros=prefillObserved;
		if (!marker) {
			if (result.progress.magic && (result.progress.magic!=RootProgressMagic
				|| result.progress.requestId!=requestId)) return Fail();
			result.state=WorkRunning;
			return true;
		}
		if (marker==RootFault && (result.pc==WorkFaultPc || result.pc==WorkFaultPc+4)
			&& io.WorkFaultSafe(result.pc)) {
			phase=RecoverableFault;
			result.state=WorkRecoverableFault;
			return true;
		}
		if (marker!=RootDone || !io.Quiet()) return Fail();
		result.completeObservedMicros=observed;
		const unsigned generated=result.progress.generated;
		if (!generated || generated>maxOutput) return Fail();
		for (unsigned i=0; i<generated; i++) result.generatedIds[i]=io.Read(RootOutputAddress+i*4);
		if (!ValidWorkResult(result,promptCount,maxOutput,context,io.Read(RootCompletionAddress+4))
			|| !Installed(io)) return Fail();
		for (unsigned i=0; i<2; i++) {
			io.Pause();
			result.pc=io.Pc();
			if (result.pc!=WorkParkPc && result.pc!=WorkParkPc+4) return Fail();
		}
		if (!io.Scope(RunReset) || !io.Quiet()) return Fail();
		io.Reset(ArmReset);
		io.Pause();
		if (!io.Scope(ArmReset) || !io.Quiet()) return Fail();
		io.Reset(HeldReset);
		io.Pause();
		if (!io.Scope(HeldReset) || !io.Quiet()) return Fail();
		for (unsigned i=0; i<10; i++) io.WriteCommand(i,originalCommand[i]);
		for (unsigned i=0; i<10; i++)
			if (io.Command(i)!=originalCommand[i]) return Fail();
		phase=Held;
		result.state=WorkComplete;
		return true;
	}

	template<class IO>
	bool Stop(IO& io)
	{
		bool recovered=false;
		if (phase==RecoverableFault) {
			if (!io.RecoverWorkFault()) return Fail();
			phase=Held;
			recovered=true;
		}
		if (phase!=Held) return false;
		if (!io.Scope(HeldReset) || !io.Quiet() || !io.RootHeld()) return Fail();
		for (unsigned i=0; i<SavedWords; i++) io.Write(Address(i),original[i]);
		for (unsigned i=0; i<SavedWords; i++)
			if (io.Read(Address(i))!=original[i]) return Fail();
		if (recovered) for (unsigned i=0; i<10; i++) io.WriteCommand(i,originalCommand[i]);
		for (unsigned i=0; i<10; i++)
			if (io.Command(i)!=originalCommand[i]) return Fail();
		if (!io.Scope(HeldReset) || !io.Quiet()) return Fail();
		phase=Closed;
		return true;
	}
};
}
#endif
