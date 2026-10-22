/* SPDX-License-Identifier: MIT */
// Genuine full-SHA-authenticated SAME-FILE FFN weight staging,not a kernel
// source attestation,workspace/input/compute grant or LLM speed benchmark.
#if !defined(PIONEER_Q38_FFN_PRIME) || PIONEER_Q38_FFN_PRIME != 1
#error Exact isolated FFN priming role required
#endif
#include "TenstorrentHaikuQwen38.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
struct FfnPrimeNativeMode:PioneerQ38WorkSessionMode {
#ifdef PIONEER_Q38_FFN_MODEL_INIT_CLIENT
 uint64_t generation=0;
 uint64_t FfnModelGeneration()const{return generation;}
#endif
 template<class IO> bool Support(IO& io,int fd) {
  tt_pioneer_driver_reload r={};r.version=1;
  if(!io.Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r)) || r.version!=1 || r.driver!=55 || r.guard_version!=1
   || r.build_tag!=
#ifdef PIONEER_Q38_FFN_MODEL_INIT_CLIENT
    UINT64_C(0x2026102200010035)
#else
    UINT64_C(0x2026102100010012)
#endif
   || !r.generation || r.quiesced || r.cold || r.active_owner || r.open_count!=1)return false;
#ifdef PIONEER_Q38_FFN_MODEL_INIT_CLIENT
  tt_q38_ffn_model_support m={};
  if(!io.Call(fd,TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT,&m,sizeof(m)) || m.version!=1 || m.driver!=55 || m.model_namespace!=8
   || m.layers!=64 || m.ffn_pages!=74 || m.requires_closed_span!=1 || m.model_inference || m.reserved)return false;
  generation=r.generation;
#endif
  return PioneerQ38WorkSessionMode::Support(io,fd);
 }
};
struct FfnPrimeProgress {
 uint64_t last=0,printed=0;
 static bool Observe(void* opaque,uint64_t bytes,uint64_t total) {
  auto& p=*static_cast<FfnPrimeProgress*>(opaque);if(!bytes || bytes>total || bytes<=p.last)return false;
  if(!p.last || bytes==total || bytes-p.printed>=UINT64_C(16777216)) {
   printf("AUTHENTIC_FFN_UPLOAD bytes%llu/%llu;SOURCE_REPACK_AND_NATIVE_TRANSFER_NOT_INFERENCE\n",
    (unsigned long long)bytes,(unsigned long long)total);p.printed=bytes;
  }
  p.last=bytes;
  return true;
 }
};
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 unsigned layer;
#ifdef PIONEER_Q38_FFN_MODEL_INIT_CLIENT
 if(!strcmp(argv[1],"--initialize-layer0"))layer=0;
 else return 2;
#else
 if(!strcmp(argv[1],"--prime-layer0"))layer=0;
 else if(!strcmp(argv[1],"--prime-all-ffn"))layer=UINT32_MAX;
 else return 2;
#endif
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("FFN_SOURCE_PRIME_ONLY;NO_COHORT_OR_MODEL_COMPUTATION_CPU_PRODUCTS_OR_SPEED_CLAIM");
 PioneerQ38PublicModelSource source;
 if(!source.Open() || !source.Ready()){puts("STOP_FULL_PUBLIC_GGUF_SHA_REQUIRED_BEFORE_ACCELERATOR_OPEN");return 1;}
 puts("FULL_PUBLIC_GGUF_SHA_SAME_FILE_PASS_BEFORE_ACCELERATOR_OPEN");
 PioneerQ38NativeTransport io;
 PioneerQ38CohortSession<PioneerQ38NativeTransport,PioneerQ38PublicModelSource,FfnPrimeNativeMode> session(io,source);
 FfnPrimeProgress progress;session.Observe(FfnPrimeProgress::Observe,&progress);
 bool okay=session.SelectFfnPrime(layer) && session.Open() && session.PrimeSpan() && session.FfnPrimeComplete(layer);
#ifdef PIONEER_Q38_FFN_MODEL_INIT_CLIENT
 okay=okay && session.InitializeFfnWorkspace(layer) && session.FfnWorkspaceComplete(layer);
#endif
 if(!okay)session.Abort(); // CLOSE ONLY,no END/retry/fallback on source/transfer loss
 bool clean=session.Finish();okay=okay && clean;
#ifdef PIONEER_Q38_FFN_MODEL_INIT_CLIENT
 printf("%s_CANONICAL_FFN_NATIVE_WORKSPACE_INITIALIZATION layer%u pages74 clean_end%u;NOT_FULL_FFN_MODEL_COMPUTATION_OR_SPEED_CLAIM\n",
  okay?"PASS":"FAIL",layer,unsigned(clean));
#endif
 printf("%s_AUTHENTIC_FFN_SOURCE_PRIME selection%u pages%llu bytes%llu matrix_calls%llu clean_end%u;HOST_SHA_BOUND_UPLOAD_NOT_KERNEL_SOURCE_ATTESTATION_OR_FULL_FFN_COMPUTE\n",
  okay?"PASS":"FAIL",layer,(unsigned long long)session.pagesUploaded,(unsigned long long)session.bytesRetained,
  (unsigned long long)session.calls,unsigned(clean));
 return okay && !session.calls?0:1;
}
