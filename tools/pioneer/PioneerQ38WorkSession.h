/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORK_SESSION_H
#define PIONEER_Q38_WORK_SESSION_H
#include "PioneerQ38DispatchSession.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
#ifdef PIONEER_Q38_NATIVE_Q8_LAYOUT
#include "PioneerQ38NativeQ8DebugTag.h"
#endif
// WORK has its own operation namespace,not a fallback or a relabelled
// checked execution. Host checks transaction identity/ranges ONLY. No
// peer/input/output numeric validation and no CPU learned products.
class PioneerQ38WorkHistory {
	uint32_t nonce=0;
public:
	bool Accept(const tt_q38_dispatch_probe& job) {
		const auto& r=job.result;
		if (nonce>=0x7ffffffe || r.version!=1 || r.state!=TT_RUNTIME_PASSED
			|| r.recovery_required || r.nonce!=nonce+1 || r.peers!=job.peers
			|| r.root_started!=1 || r.root_completed!=1 || r.root_held!=1
			|| r.root_marker!=0x314d3851 || r.root_count!=job.peers) return false;
		nonce++; return true;
	}
};
struct PioneerQ38WorkSessionMode {
	static constexpr unsigned Workers=119,Operation=TT_PCI_PROBE_Q38_WORK_RUN;
	PioneerQ38WorkHistory history;
	// PIONEER_NATIVE_Q8_WORK_SUPPORT_BEGIN
	template<class IO> bool Support(IO& io,int fd) {
#ifdef PIONEER_Q38_NATIVE_Q8_LAYOUT
		tt_pioneer_driver_reload metadata={}; metadata.version=1;
		if (!io.Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || !PioneerQ38NativeQ8DebugTag(metadata)) return false;
#endif
		tt_q38_dispatch_support s={};
		return io.Call(fd,TT_PCI_PROBE_Q38_WORK_SUPPORT,&s,sizeof(s)) && s.driver==TT_Q38_DISPATCH_DRIVER && s.version==TT_Q38_WORK_SUPPORT_VERSION
			&& s.peers==119 && s.rows==256 && s.max_inputs==17408 && s.banks==7 && s.offset_alignment==64 && s.persistent==1 && !s.model_inference;
	}
	// PIONEER_NATIVE_Q8_WORK_SUPPORT_END
	static auto Request(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input) { return PioneerQ38DispatchRequest(wave,input); }
	bool Collect(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input,const tt_q38_dispatch_probe& reply,float* scratch,unsigned rows) {
		if (!scratch || !wave.Ready() || wave.workers>119 || rows!=wave.outputs || input.type!=wave.type || input.inputs!=wave.inputs
			|| reply.type!=wave.type || reply.inputs!=wave.inputs || reply.peers!=wave.workers
			|| !PioneerQ38Disjoint(reinterpret_cast<const float*>(&reply),sizeof(reply)/4,scratch,rows)) return false;
		for (unsigned w=0;w<wave.workers;w++) if (wave.firstRow[w]>rows || wave.part[w].groups*16>rows-wave.firstRow[w]) return false;
		if (!wave.Ready() || !history.Accept(reply)) return false;
		for (unsigned w=0;w<wave.workers;w++) for (unsigned r=0;r<wave.part[w].groups*16;r++)
			scratch[wave.firstRow[w]+r]=PioneerAttentionFloat(reply.output[w][r]);
		return true;
	}
};
template<class IO,class Source=PioneerQ38PublicModelSource>
using PioneerQ38WorkSession=PioneerQ38CohortSession<IO,Source,PioneerQ38WorkSessionMode>;
#endif
