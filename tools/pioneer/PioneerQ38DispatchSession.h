/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_SESSION_H
#define PIONEER_Q38_DISPATCH_SESSION_H
#include "PioneerQ38CohortSession.h"
#include "PioneerQ38DispatchHost.h"
#ifdef PIONEER_Q38_CPU_VALIDATION
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
#endif
// Separate54 strategy:119 persistent arithmetic peers+one dispatcher. Same
// same-FD authenticated source,bulk proofs/cache,all-wave atomic output and
// exclusive owner/clean END. Default has NO CPU products/reference/fallback;
// explicit CPU_VALIDATION adds the separate diagnostic in CohortSession.
// This NEVER
// converts WAIT into the old per-job stopped=1 interface.
struct PioneerQ38DispatchSessionMode {
	static constexpr unsigned Workers=119,Operation=TT_PCI_PROBE_Q38_DISPATCH_RUN;
	PioneerQ38DispatchHistory history;
	template<class IO> bool Support(IO& io,int fd) {
		tt_q38_dispatch_support s={};
#ifdef PIONEER_Q38_CPU_VALIDATION
		// Older55 close implicitly performed END after a user-only numeric
		// failure. Require the new explicit-END contract BEFORE any BEGIN.
		if (!io.Call(fd,TT_PCI_PROBE_Q38_WORK_SUPPORT,&s,sizeof(s)) || s.driver!=55
			|| s.version!=TT_Q38_WORK_SUPPORT_VERSION || s.peers!=119 || s.rows!=256 || s.max_inputs!=17408
			|| s.banks!=7 || s.offset_alignment!=64 || s.persistent!=1 || s.model_inference) return false;
		s={};
#endif
		return io.Call(fd,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,&s,sizeof(s)) && s.driver==TT_Q38_DISPATCH_DRIVER && s.version==1
			&& s.peers==119 && s.rows==256 && s.max_inputs==17408 && s.banks==7 && s.offset_alignment==64 && s.persistent==1 && !s.model_inference;
	}
	static auto Request(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input) { return PioneerQ38DispatchRequest(wave,input); }
	bool Collect(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input,const tt_q38_dispatch_probe& reply,float* scratch,unsigned rows)
	{ return PioneerQ38DispatchCollect(wave,input,reply,history,scratch,rows); }
};
template<class IO,class Source=PioneerQ38PublicModelSource>
using PioneerQ38DispatchSession=PioneerQ38CohortSession<IO,Source,PioneerQ38DispatchSessionMode>;
#endif
