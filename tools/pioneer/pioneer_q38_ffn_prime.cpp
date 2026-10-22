/* SPDX-License-Identifier: MIT */
// Genuine full-SHA-authenticated SAME-FILE FFN weight staging,not a kernel
// source attestation,workspace/input/compute grant or LLM speed benchmark.
#if !defined(PIONEER_Q38_FFN_PRIME) || PIONEER_Q38_FFN_PRIME != 1
#error Exact isolated FFN priming role required
#endif
#include "TenstorrentHaikuQwen38.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
struct FfnPrimeNativeMode:PioneerQ38WorkSessionMode {
 template<class IO> bool Support(IO& io,int fd) {
  tt_pioneer_driver_reload r={};r.version=1;
  if(!io.Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r)) || r.version!=1 || r.driver!=55 || r.guard_version!=1
   || r.build_tag!=UINT64_C(0x2026102100010012) || !r.generation || r.quiesced || r.cold || r.active_owner || r.open_count!=1)return false;
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
 if(!strcmp(argv[1],"--prime-layer0"))layer=0;
 else if(!strcmp(argv[1],"--prime-all-ffn"))layer=UINT32_MAX;
 else return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("FFN_SOURCE_PRIME_ONLY;NO_COHORT_OR_MODEL_COMPUTATION_CPU_PRODUCTS_OR_SPEED_CLAIM");
 PioneerQ38PublicModelSource source;
 if(!source.Open() || !source.Ready()){puts("STOP_FULL_PUBLIC_GGUF_SHA_REQUIRED_BEFORE_ACCELERATOR_OPEN");return 1;}
 puts("FULL_PUBLIC_GGUF_SHA_SAME_FILE_PASS_BEFORE_ACCELERATOR_OPEN");
 PioneerQ38NativeTransport io;
 PioneerQ38CohortSession<PioneerQ38NativeTransport,PioneerQ38PublicModelSource,FfnPrimeNativeMode> session(io,source);
 FfnPrimeProgress progress;session.Observe(FfnPrimeProgress::Observe,&progress);
 bool okay=session.SelectFfnPrime(layer) && session.Open() && session.PrimeSpan() && session.FfnPrimeComplete(layer);
 if(!okay)session.Abort(); // CLOSE ONLY,no END/retry/fallback on source/transfer loss
 bool clean=session.Finish();okay=okay && clean;
 printf("%s_AUTHENTIC_FFN_SOURCE_PRIME selection%u pages%llu bytes%llu matrix_calls%llu clean_end%u;HOST_SHA_BOUND_UPLOAD_NOT_KERNEL_SOURCE_ATTESTATION_OR_FULL_FFN_COMPUTE\n",
  okay?"PASS":"FAIL",layer,(unsigned long long)session.pagesUploaded,(unsigned long long)session.bytesRetained,
  (unsigned long long)session.calls,unsigned(clean));
 return okay && !session.calls?0:1;
}
