/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BENCHMARK_HOST_H
#define PIONEER_Q38_BENCHMARK_HOST_H
#ifndef PIONEER_Q38_BENCHMARK_HOST
#error Isolated fixed-public-FFN benchmark host ONLY,not an Assistant/model feature
#endif
#include <string.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#if defined(PIONEER_Q38_BENCHMARK_HOST_LOCAL_Q4_QUALIFICATION) && (defined(PIONEER_Q38_BENCHMARK_HOST_Q4_DIRECT_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_Q4_NIBBLE_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_COMMAND_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PREFETCH_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_RAW_CACHE_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION))
#error Local Q4 benchmark has its distinct fixed17/28 original-TC role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_Q4_DIRECT_QUALIFICATION) && !defined(PIONEER_Q38_BENCHMARK_HOST_Q4_NIBBLE_QUALIFICATION)
#error Direct Q4 benchmark requires its complete separate fixed17/27 role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_Q4_NIBBLE_QUALIFICATION) && (defined(PIONEER_Q38_BENCHMARK_HOST_COMMAND_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PREFETCH_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_RAW_CACHE_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION))
#error Q4 nibble benchmark has its distinct fixed17/26 driver-tag role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_COMMAND_QUALIFICATION) && (defined(PIONEER_Q38_BENCHMARK_HOST_PREFETCH_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_RAW_CACHE_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION))
#error Command-reuse benchmark has its distinct fixed driver-tag role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_PREFETCH_QUALIFICATION) && (defined(PIONEER_Q38_BENCHMARK_HOST_RAW_CACHE_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION))
#error Prefetch benchmark has its distinct fixed driver-tag role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_RAW_CACHE_QUALIFICATION) && (defined(PIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION))
#error Raw-Dst cache benchmark has its distinct fixed driver-tag role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION) && (defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION))
#error Frontend-barrier benchmark has a distinct fixed driver-tag role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION) && (defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) || defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION))
#error Direct batch input has a distinct fixed benchmark-host role
#endif
#if defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION) && defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION)
#error Distinct original-TC benchmark-host diagnostic roles MUST NOT mix
#endif
static inline bool PioneerQ38BenchmarkHostTag(const tt_pioneer_driver_reload& r) {
 return r.version==1 && r.driver==55 && r.guard_version==1 && r.generation
  && !r.quiesced && !r.cold && !r.active_owner && r.open_count==1
#ifdef PIONEER_Q38_BENCHMARK_HOST_LOCAL_Q4_QUALIFICATION
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010028));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_Q4_DIRECT_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010027));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_Q4_NIBBLE_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010026));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_COMMAND_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010024));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_PREFETCH_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010023));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_RAW_CACHE_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010022));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010021));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010019)
   || r.build_tag==UINT64_C(0x2026102100010020));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION)
  && (r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010019));
#elif defined(PIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION)
  && r.build_tag==UINT64_C(0x2026102100010018);
#else
  && (r.build_tag==UINT64_C(0x2026102100010012) || r.build_tag==UINT64_C(0x2026102100010016)
   || r.build_tag==UINT64_C(0x2026102100010017));
#endif
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
