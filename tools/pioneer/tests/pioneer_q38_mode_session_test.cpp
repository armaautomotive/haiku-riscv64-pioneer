/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_SESSION_MOCK_ONLY
#include "pioneer_q38_dispatch_session_test.cpp"
#include "../PioneerQ38WorkSession.h"
struct WorkSessionIO:DispatchSessionIO {
	unsigned workRuns=0,workSupports=0;
	bool oldOnly=false;
	WorkSessionIO(FakeSource& s,unsigned t):DispatchSessionIO(s,t) {}
	bool Call(int fd,uint32_t op,void* data,size_t bytes) {
		if (op==TT_PCI_PROBE_Q38_WORK_SUPPORT) {
			workSupports++; if (oldOnly) return false;
			bool okay=DispatchSessionIO::Call(fd,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,data,bytes);
			static_cast<tt_q38_dispatch_support*>(data)->version=TT_Q38_WORK_SUPPORT_VERSION; return okay;
		}
		if (op==TT_PCI_PROBE_Q38_WORK_RUN) {
			workRuns++; bool okay=DispatchSessionIO::Call(fd,TT_PCI_PROBE_Q38_DISPATCH_RUN,data,bytes);
			auto& r=*static_cast<tt_q38_dispatch_probe*>(data);
			// Work host must NOT require checked-only peer/input/stats data.
			memset(r.result.peer,0,sizeof(r.result.peer)); memset(r.result.stats,0,sizeof(r.result.stats));
			r.activation[0]^=1;
			return okay;
		}
		assert(op!=TT_PCI_PROBE_Q38_DISPATCH_RUN && op!=TT_PCI_PROBE_Q38_DISPATCH_SUPPORT);
		return DispatchSessionIO::Call(fd,op,data,bytes);
	}
};
#ifndef PIONEER_Q38_MODE_SESSION_MOCK_ONLY
int main()
{
	(void)&Check; (void)&CohortHostTestMain;
	Fixture fixture; FakeSource source(fixture.file);
	const auto* t=source.Model().Find("blk.3.attn_k.weight"); assert(t);
	unsigned index=unsigned(t-source.Model().Tensors().data());
	std::vector<float> input(5120,.125f),output(1024,-99),before=output;
#ifdef PIONEER_Q38_CPU_VALIDATION
	{
		DispatchSessionIO old(source,index); old.fault=19;
		PioneerQ38DispatchSession<DispatchSessionIO,FakeSource> rejected(old,source);
		assert(!rejected.Open() && !old.begins && !old.runs && old.closes==1);
	}
	DispatchSessionIO io(source,index); PioneerQ38DispatchSession<DispatchSessionIO,FakeSource> session(io,source);
	assert(session.Open() && !session.Multiply(index,input.data(),5120,output.data(),1024,1));
	assert(output==before && session.Failed() && io.closes==1 && !io.ends && !session.Finish());
	unsigned calls=io.uploads+io.runs;
	assert(!session.Multiply(index,input.data(),5120,output.data(),1024,1) && calls==io.uploads+io.runs);
	puts("PASS CPU-validated SESSION:independently rejects wrong numeric output BEFORE caller publication,CLOSE ONLY without END/retry/CPU substitution;NO DEVICE IO");
#else
	for (unsigned fault:{0U,4U,7U,10U}) {
		WorkSessionIO io(source,index); io.fault=fault; io.failWave=2; source.valid=true;
		PioneerQ38WorkSession<WorkSessionIO,FakeSource> session(io,source); output=before;
		assert(session.Open()); bool okay=session.Multiply(index,input.data(),5120,output.data(),1024,1);
		assert(okay==!fault && io.workRuns==io.runs && io.workSupports==1);
		if (fault) assert(output==before && session.Failed() && io.closes==1 && !io.ends && !session.Finish());
		else { for (unsigned r=0;r<1024;r++) assert(output[r]==float(r+1)); assert(session.Finish() && io.ends==1); }
	}
	source.valid=true; WorkSessionIO io(source,index); io.oldOnly=true;
	PioneerQ38WorkSession<WorkSessionIO,FakeSource> session(io,source);
	assert(!session.Open() && io.closes==1 && !io.begins && !io.workRuns);
	puts("PASS WORK SESSION:own explicit ioctl,no checked fallback,no peer/input/result validation,multiwave assembly,source/transaction failures CLOSE ONLY,clean END;NO DEVICE IO");
#endif
}
#endif
