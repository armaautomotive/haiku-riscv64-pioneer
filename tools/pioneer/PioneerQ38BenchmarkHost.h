/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BENCHMARK_HOST_H
#define PIONEER_Q38_BENCHMARK_HOST_H
#ifndef PIONEER_Q38_BENCHMARK_HOST
#error Isolated fixed-public-FFN benchmark host ONLY,not an Assistant/model feature
#endif
#include <string.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
static inline bool PioneerQ38BenchmarkHostTag(const tt_pioneer_driver_reload& r) {
 return r.version==1 && r.driver==55 && r.guard_version==1 && r.generation
  && !r.quiesced && !r.cold && !r.active_owner && r.open_count==1
  && (r.build_tag==UINT64_C(0x2026102100010012) || r.build_tag==UINT64_C(0x2026102100010016)
   || r.build_tag==UINT64_C(0x2026102100010017));
}
// Process-control metadata ONLY;not file/device access or completion proof.
// Caller independently authenticates the SAME open file before any fork,
// and a child must prove clean GPU END before its zero exit is accepted.
class PioneerQ38BenchmarkHostPolicy {
 enum Phase {Auth,Idle,Child,Poisoned,Closed}phase=Auth;
 unsigned round=0;
 bool Fault(){phase=Poisoned;return false;}
public:
 bool Authenticated(bool okay){if(phase!=Auth || !okay)return Fault();phase=Idle;return true;}
 bool Begin(){if(phase!=Idle || round==1024)return Fault();round++;phase=Child;return true;}
 bool ChildFinished(bool knownClean){if(phase!=Child || !knownClean)return Fault();phase=Idle;return true;}
 bool Close(){if(phase!=Idle)return Fault();phase=Closed;return true;}
 unsigned Round()const{return round;}
 bool Waiting()const{return phase==Idle;}
 static unsigned Command(const char* line){return !line?0:!strcmp(line,"measure\n")?1:!strcmp(line,"quit\n")?2:0;}
};
#endif
