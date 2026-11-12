/* SPDX-License-Identifier: MIT */
// Explicit one-shot candidate test. Token IDs only; no CPU model or fallback.
#if ((!defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) || PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128 != 1) \
 && (!defined(PIONEER_Q38_QGWD_CLIENT_141) || PIONEER_Q38_QGWD_CLIENT_141 != 1)) \
 || !defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1
#error QGW8 full candidate diagnostic needs its exact build roles
#endif
#include <Drivers.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#ifdef PIONEER_Q38_QGWD_CLIENT_141
#include "PioneerQ38HybridDenseQ8VerifiedModel.h"
using DiagnosticModel=PioneerQ38HybridDenseQ8VerifiedModel;
#else
#include "PioneerQ38HybridHeadVerifiedModel.h"
using DiagnosticModel=PioneerQ38HybridHeadVerifiedModel;
#endif
#include "PioneerQ38HybridFullSession.h"
#ifdef PIONEER_Q38_QGWF_RATE_CLIENT_144
#include "PioneerQ38TokenRateSamples.h"
#endif
#ifdef PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC
#if PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_QGWD_TWO_CLIENT_142) || PIONEER_Q38_QGWD_TWO_CLIENT_142 != 1
#error Upload timing is an explicit QGWD two-token diagnostic only
#endif
#include "PioneerQ38UploadTiming.h"
#endif

struct Transport {
 unsigned lastOperation=0;int openError=0,lastError=0,closeError=0;
#ifdef PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC
 PioneerQ38UploadTiming* timing=nullptr;
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_CLIENT_131
 bool OutputSafe(const void* output,size_t bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
  return output && bytes && out<=UINTPTR_MAX-bytes && self<=UINTPTR_MAX-sizeof(*this)
   && !(out<self+sizeof(*this) && self<out+bytes);
 }
#endif
 int Open(){const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)openError=errno;return fd;}
 bool Call(int fd,unsigned op,void* data,size_t size){
#ifdef PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC
  const bool span=timing && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && data && size==sizeof(tt_q38_span_upload);
  const uint64_t requested=span?static_cast<tt_q38_span_upload*>(data)->bytes:0;
  const uint64_t before=span?timing->Begin():0;
#endif
  lastOperation=op;const int result=ioctl(fd,op,data,size);lastError=result==0?0:errno;
#ifdef PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC
  if(span)timing->End(before,timing->callNs,timing->callCalls,timing->callBytes,requested);
#endif
  return result==0;
 }
 int Close(int fd){const int result=close(fd);if(result)closeError=errno;return result;}
};
static uint64_t Now(){
 timespec t={};if(clock_gettime(CLOCK_MONOTONIC,&t) || t.tv_sec<0 || t.tv_nsec<0 || t.tv_nsec>=1000000000)return 0;
 return uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);
}
[[maybe_unused]] static bool Number(const char* text,unsigned max,unsigned& value){
 if(!text || *text<'0' || *text>'9')return false;
 char* end=nullptr;errno=0;const unsigned long result=strtoul(text,&end,10);
 if(errno || !end || *end || result>max)return false;
 value=unsigned(result);return true;
}
template<class Session> static void ReportFailure(const Session& session){
 typename Session::ControlObservation observed;
 if(!session.FailureObservation(observed))return;
 const auto& r=observed.reply;
 fprintf(stderr,"HYBRID_CONTROL_OBSERVATION action=%u call_passed=%u request_unchanged=%u "
  "version=%u operation=%u started=%u ready=%u unsafe=%u complete=%u stage=%u records=%u status=%ld detail=",
  observed.action,observed.callPassed,observed.requestUnchanged,r.version,r.operation,r.started,r.ready,r.unsafe,
  r.complete,r.stage,r.records,(long)r.status);
 for(unsigned i=0;i<16;i++)fprintf(stderr,"%s%u",i?",":"",r.detail[i]);
 fputs("; UNTRUSTED_DIAGNOSTIC_ONLY_NO_RETRY_OR_CLEANUP_AUTHORITY\n",stderr);
}
static void ReportTransportFailure(const Transport& io){
 if(io.openError)fprintf(stderr,"HYBRID_OPEN_FAILURE errno=%d; DIAGNOSTIC_ONLY\n",io.openError);
 if(io.lastError)fprintf(stderr,"HYBRID_IOCTL_FAILURE operation=%u errno=%d; DIAGNOSTIC_ONLY\n",io.lastOperation,io.lastError);
 if(io.closeError)fprintf(stderr,"HYBRID_CLOSE_FAILURE errno=%d; DIAGNOSTIC_ONLY\n",io.closeError);
}
int main(int argc,char** argv){
 namespace F=PioneerQ38FullOffload;
#ifdef PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT
 puts("QGWF_PIPELINED_UPLOAD_EXPERIMENT; NO PERFORMANCE OR QUALITY CLAIM");
#endif
#ifdef PIONEER_Q38_QGWD_RESIDENT_CLIENT_146
 if(argc!=6 || strcmp(argv[1],"--run-qgwd-resident-146") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgwd-resident-146 ABS_FFN_BFP4 ABS_FFN_BFP8 ABS_HEAD_BFP8 ABS_DENSE_BFP8; two fixed-input requests\n",stderr);return 2;
 }
 unsigned outputs=17,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGWF_UNROLL_CLIENT_145)
 if(argc!=6 || strcmp(argv[1],"--run-qgwf-unroll-145") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgwf-unroll-145 ABS_FFN_BFP4 ABS_FFN_BFP8 ABS_HEAD_BFP8 ABS_DENSE_BFP8; fixed input, up to 17 outputs\n",stderr);return 2;
 }
 unsigned outputs=17,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGWF_RATE_CLIENT_144)
 if(argc!=6 || strcmp(argv[1],"--run-qgwf-rate-144") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgwf-rate-144 ABS_FFN_BFP4 ABS_FFN_BFP8 ABS_HEAD_BFP8 ABS_DENSE_BFP8; one fixed input, up to 17 outputs\n",stderr);return 2;
 }
 unsigned outputs=17,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGWF_CLIENT_143)
 if(argc!=6 || strcmp(argv[1],"--run-qgwf-143") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgwf-143 ABS_FFN_BFP4 ABS_FFN_BFP8 ABS_HEAD_BFP8 ABS_DENSE_BFP8; one fixed input, two outputs only\n",stderr);return 2;
 }
 unsigned outputs=2,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGWD_TWO_CLIENT_142)
 if(argc!=6 || strcmp(argv[1],"--run-qgwd-142") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgwd-142 ABS_FFN_BFP4 ABS_FFN_BFP8 ABS_HEAD_BFP8 ABS_DENSE_BFP8; one fixed input, two outputs only\n",stderr);return 2;
 }
 unsigned outputs=2,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGWD_CLIENT_141)
 if(argc!=6 || strcmp(argv[1],"--run-qgwd-141") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgwd-141 ABS_FFN_BFP4 ABS_FFN_BFP8 ABS_HEAD_BFP8 ABS_DENSE_BFP8; one fixed token only\n",stderr);return 2;
 }
 unsigned outputs=1,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGWC_CLIENT_136)
 if(argc!=5 || strcmp(argv[1],"--run-qgwc-srca") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/'){
  fputs("Use --run-qgwc-srca ABS_FFN_BFP4_CACHE ABS_FFN_BFP8_CACHE ABS_HEAD_BFP8_CACHE; one fixed token only\n",stderr);return 2;
 }
 unsigned outputs=1,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
 if(argc!=6 || strcmp(argv[1],"--run-qgw8-trace") || argv[2][0]!='/' || argv[3][0]!='/'
  || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgw8-trace ABS_FFN_BFP4_CACHE ABS_FFN_BFP8_CACHE ABS_HEAD_BFP8_CACHE NEW_ABS_TRACE\n",stderr);return 2;
 }
 struct stat destination={};errno=0;
 if(lstat(argv[5],&destination)==0 || errno!=ENOENT){fputs("QGW8_TRACE_DESTINATION_REFUSED; no device open\n",stderr);return 2;}
 unsigned outputs=1,ids[F::MaxPrompt]={248319},prompts=1;
#elif defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129)
 if(argc!=6 || strcmp(argv[1],"--run-qgw8-hidden") || argv[2][0]!='/' || argv[3][0]!='/'
  || argv[4][0]!='/' || argv[5][0]!='/'){
  fputs("Use --run-qgw8-hidden ABS_FFN_BFP4_CACHE ABS_FFN_BFP8_CACHE ABS_HEAD_BFP8_CACHE NEW_ABS_SNAPSHOT\n",stderr);return 2;
 }
 unsigned outputs=1,ids[F::MaxPrompt]={PioneerQ38Qgw8Hidden::Input},prompts=1;
#else
 if(argc<7 || argc>6+F::MaxPrompt || strcmp(argv[1],"--run-qgw8-candidate") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/'){
  fputs("Use --run-qgw8-candidate ABS_FFN_BFP4_CACHE ABS_FFN_BFP8_CACHE ABS_HEAD_BFP8_CACHE OUTPUT_COUNT TOKEN_ID...\n",stderr);return 2;
 }
 unsigned outputs=0,ids[F::MaxPrompt]={},prompts=unsigned(argc-6);
 if(!Number(argv[5],F::MaxOutput,outputs) || !outputs || prompts+outputs-1>F::MaxPositions)return 2;
 for(unsigned i=0;i<prompts;i++)if(!Number(argv[6+i],F::Vocabulary-1,ids[i]))return 2;
#endif
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet");
 auto model=std::make_unique<DiagnosticModel>();std::string error;
 if(!model->Prepare(argv[2],argv[3],argv[4],
#ifdef PIONEER_Q38_QGWD_CLIENT_141
  argv[5],
#endif
  error,[](unsigned done,unsigned total){
  if(done%21==0 || done==total)printf("HYBRID_PREFLIGHT %u/%u\n",done,total);
 }) || !model->Ready()){
  fprintf(stderr,"HYBRID_PREFLIGHT_REFUSED %s; no device open\n",error.c_str());return 1;
 }
 auto* pages=model->PreparedPages();const auto* canonical=model->Canonical();if(!pages || !canonical)return 1;
 Transport io;
#ifdef PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC
 PioneerQ38UploadTiming uploadTiming(Now);PioneerQ38TimedPages<DiagnosticModel::Pages> timedPages(*pages,uploadTiming);
 io.timing=&uploadTiming;
 auto session=std::make_unique<PioneerQ38HybridFullSession<PioneerQ38TimedPages<DiagnosticModel::Pages>,Transport>>(timedPages,io);
#else
 auto session=std::make_unique<PioneerQ38HybridFullSession<DiagnosticModel::Pages,Transport>>(*pages,io);
#endif
 const uint64_t begin=Now();if(!begin)return 1;uint64_t next=UINT64_C(256)*1024*1024,inferenceBegin=0;
 if(!session->Begin(*canonical,ids,prompts,outputs,[&](uint64_t done,uint64_t total){
  if(done>=next || done==total){printf("HYBRID_UPLOAD %llu/%llu\n",(unsigned long long)done,(unsigned long long)total);
   while(next<=done)next+=UINT64_C(256)*1024*1024;}
 },[&]{puts("HYBRID_START_REQUEST diagnostic only; no completion implied");inferenceBegin=Now();return inferenceBegin>=begin;})){
  fprintf(stderr,"HYBRID_START_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);ReportTransportFailure(io);return 1;
 }
 const uint64_t started=Now();if(!started || !inferenceBegin || started<inferenceBegin){session->Abort();return 1;}
#ifdef PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC
 printf("HYBRID_UPLOAD_TIMING valid=%u read_calls=%llu read_bytes=%llu read_ns=%llu span_calls=%llu span_bytes=%llu span_ioctl_ns=%llu;HOST_WALL_TIME_NOT_RAW_PCIE\n",
  unsigned(uploadTiming.valid),(unsigned long long)uploadTiming.readCalls,(unsigned long long)uploadTiming.readBytes,
  (unsigned long long)uploadTiming.readNs,(unsigned long long)uploadTiming.callCalls,
  (unsigned long long)uploadTiming.callBytes,(unsigned long long)uploadTiming.callNs);
#endif
 printf("HYBRID_STARTED upload_and_start_seconds=%.3f\n",double(started-begin)*1e-9);
 uint64_t nextPending=started;
#ifdef PIONEER_Q38_QGWF_RATE_CLIENT_144
 PioneerQ38TokenRateSamples rateSamples(prompts,prompts+outputs-1);bool rateValid=true;
#endif
 while(!session->TerminalReady()){
#ifdef PIONEER_Q38_QGWF_RATE_CLIENT_144
  const uint64_t pollBegin=Now();
#endif
  if(!session->Poll([&](uint32_t firstPC,uint32_t secondPC){
   const uint64_t now=Now();if(now>=nextPending){
    printf("HYBRID_PENDING seconds=%.3f pc_first=%u pc_second=%u; diagnostic only\n",
     double(now-started)*1e-9,firstPC,secondPC);
    nextPending=now+UINT64_C(30)*1000000000;
   }
  })){fprintf(stderr,"HYBRID_OBSERVE_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);ReportTransportFailure(io);return 1;}
#ifdef PIONEER_Q38_QGWF_RATE_CLIENT_144
  const uint64_t pollEnd=Now();unsigned frontier=0;
  if(!pollBegin || !pollEnd || pollEnd<pollBegin || pollEnd/1000>uint64_t(INT64_MAX)
   || !session->LatestFrontier(frontier)
   || !rateSamples.ObserveCount(frontier,int64_t(pollBegin/1000),int64_t(pollEnd/1000)))rateValid=false;
#endif
  if(!session->TerminalReady()){
   const uint64_t now=Now();if(!now || now<started || now-started>UINT64_C(7200)*1000000000){
    session->Abort();fputs("HYBRID_TIMEOUT close only; recovery may be required\n",stderr);return 1;
   }
#ifdef PIONEER_Q38_QGWF_RATE_CLIENT_144
   // A 100 ms polling interval can miss the entire short decode on a fast
   // card. This deliberately measures a 10 ms-polling diagnostic workload;
   // it is not a claim about an uninstrumented production workload.
   usleep(10000);
#else
   usleep(100000);
#endif
  }
 }
 const uint64_t terminal=Now();
 if(!terminal || terminal<started){session->Abort();return 1;}
 if(!session->Finish()){fprintf(stderr,"HYBRID_FINISH_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);ReportTransportFailure(io);return 1;}
 unsigned tokens[F::MaxOutput]={},count=0;
 if(!session->CopyGeneratedTokens(tokens,F::MaxOutput,count))return 1;
#ifdef PIONEER_Q38_QGWD_RESIDENT_CLIENT_146
 if(!count){session->Abort();fputs("QGWD_RESIDENT_EMPTY_FIRST_OUTPUT close only; NO COMPARISON\n",stderr);return 1;}
#endif
 printf("HYBRID_RESULT prompt_tokens=%u generated_tokens=%u inference_seconds=%.3f (includes START and prompt processing)\n",
  prompts,count,double(terminal-inferenceBegin)*1e-9);
 if(count && terminal>inferenceBegin)printf(
#ifdef PIONEER_Q38_QGWD_CLIENT_141
  "QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT %.6f\n",
#else
  "QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT %.6f\n",
#endif
  double(count)*1e9/double(terminal-inferenceBegin));
 for(unsigned i=0;i<count;i++)printf("HYBRID_TOKEN %u\n",tokens[i]);
#ifdef PIONEER_Q38_QGWD_RESIDENT_CLIENT_146
 puts("QGWD_RESIDENT_FIRST_REQUEST_CLOSED weights retained; output quality not validated");
#else
 puts("HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated");
#endif
#ifdef PIONEER_Q38_QGWF_RATE_CLIENT_144
 PioneerQ38TokenRateSamples::Bounds rateBounds;
 if(rateValid && rateSamples.ObservedBounds(rateBounds) && rateBounds.sampledTokens>=2){
  printf("QGWF_OBSERVED_DECODE_BOUNDS tokens=%u first=%u last=%u min_us=%lld max_us=%lld lower_tps=%.9f upper_tps=%.9f;COUNT_WINDOW_NOT_QUALITY\n",
   rateBounds.sampledTokens,rateBounds.firstCommitted,rateBounds.lastCommitted,
   (long long)rateBounds.minElapsedUs,(long long)rateBounds.maxElapsedUs,
   rateBounds.lowerTokensPerSecond,rateBounds.upperTokensPerSecond);
 }else puts("QGWF_OBSERVED_DECODE_UNAVAILABLE insufficient_or_invalid_count_window;NOT_A_SPEED_RESULT");
#endif
#ifdef PIONEER_Q38_QGWD_RESIDENT_CLIENT_146
 const uint64_t secondBegin=Now();
 if(!secondBegin || !session->NextResidentRequest()){
  fprintf(stderr,"QGWD_RESIDENT_NEXT_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);ReportTransportFailure(io);return 1;
 }
 const uint64_t secondStarted=Now();
 if(!secondStarted || secondStarted<secondBegin){session->Abort();return 1;}
 puts("QGWD_RESIDENT_SECOND_STARTED no second upload; diagnostic only");
 uint64_t secondPending=secondStarted;
 while(!session->TerminalReady()){
  if(!session->Poll([&](uint32_t firstPC,uint32_t secondPC){
   const uint64_t now=Now();if(now>=secondPending){
    printf("QGWD_RESIDENT_SECOND_PENDING seconds=%.3f pc_first=%u pc_second=%u\n",double(now-secondStarted)*1e-9,firstPC,secondPC);
    secondPending=now+UINT64_C(30)*1000000000;
   }
  })){fprintf(stderr,"QGWD_RESIDENT_SECOND_OBSERVE_FAILED %s; close only\n",session->failure.c_str());ReportFailure(*session);ReportTransportFailure(io);return 1;}
  if(!session->TerminalReady()){
   const uint64_t now=Now();if(!now || now<secondStarted || now-secondStarted>UINT64_C(7200)*1000000000){
    session->Abort();fputs("QGWD_RESIDENT_SECOND_TIMEOUT close only\n",stderr);return 1;
   }
   usleep(10000);
  }
 }
 const uint64_t secondTerminal=Now();
 if(!secondTerminal || secondTerminal<secondStarted || !session->Finish()){
  session->Abort();fputs("QGWD_RESIDENT_SECOND_FINISH_FAILED close only\n",stderr);return 1;
 }
 unsigned repeated[F::MaxOutput]={},repeatedCount=0;
 if(!session->CopyGeneratedTokens(repeated,F::MaxOutput,repeatedCount) || repeatedCount!=count
  || !std::equal(tokens,tokens+count,repeated)){
  session->Abort();fputs("QGWD_RESIDENT_TOKEN_MISMATCH close only; quality not established\n",stderr);return 1;
 }
 printf("QGWD_RESIDENT_SECOND_MATCH count=%u seconds_including_start=%.3f; QUALITY_NOT_VALIDATED\n",
  count,double(secondTerminal-secondBegin)*1e-9);
 if(!session->EndResident()){fprintf(stderr,"QGWD_RESIDENT_END_FAILED %s; close only\n",session->failure.c_str());return 1;}
 puts("QGWD_RESIDENT_TWO_REQUESTS_UNLOADED; QUALITY_NOT_VALIDATED");
#endif
#ifdef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
 auto snapshot=std::make_unique<PioneerQ38TokenHiddenSnapshot>();
 if(!session->CopyHiddenSnapshot(snapshot.get(),sizeof(*snapshot))){fputs("QGW8_HIDDEN_SNAPSHOT_COPY_FAILED; AFTER_CLOSE\n",stderr);return 1;}
 const int file=open(argv[5],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(file<0){fprintf(stderr,"QGW8_HIDDEN_CREATE_FAILED errno=%d; AFTER_CLOSE\n",errno);return 1;}
 const auto* data=reinterpret_cast<const uint8_t*>(snapshot.get());size_t left=sizeof(*snapshot);bool saved=true;
 struct stat info={};if(fstat(file,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1
  || info.st_uid!=geteuid() || (info.st_mode&077)!=0)saved=false;
 while(saved && left){const ssize_t n=write(file,data,left);if(n<0 && errno==EINTR)continue;
  if(n<=0){saved=false;break;}data+=n;left-=size_t(n);}
 if(saved && fsync(file))saved=false;
 if(close(file))saved=false;
 if(!saved || left){fputs("QGW8_HIDDEN_WRITE_FAILED; PARTIAL_FILE_UNTRUSTED AFTER_CLOSE\n",stderr);return 1;}
 const auto digest=PioneerModelWeightCache::Hash(snapshot.get(),sizeof(*snapshot));
 printf("QGW8_HIDDEN_SNAPSHOT sha256=%s bytes=%zu; DATA_ONLY_AFTER_CLOSE\n",
  PioneerModelWeightCache::Hex(digest).c_str(),sizeof(*snapshot));
#elif defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"trace DATA is little endian");
 auto trace=std::make_unique<std::array<uint32_t,PioneerQ38Qgw8LayerTraceGeometry::Layers*PioneerQ38Qgw8LayerTraceGeometry::Words>>();
 const uint64_t traceGeneration=session->TraceGeneration();
 if(!traceGeneration || !session->CopyTrace(trace->data(),trace->size())){fputs("QGW8_TRACE_COPY_FAILED; AFTER_CLOSE\n",stderr);return 1;}
 const int file=open(argv[5],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(file<0){fprintf(stderr,"QGW8_TRACE_CREATE_FAILED errno=%d; AFTER_CLOSE\n",errno);return 1;}
 const auto* data=reinterpret_cast<const uint8_t*>(trace->data());size_t left=trace->size()*sizeof(uint32_t);bool saved=true;
 struct stat info={};if(fstat(file,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1
  || info.st_uid!=geteuid() || (info.st_mode&077)!=0)saved=false;
 while(saved && left){const ssize_t n=write(file,data,left);if(n<0 && errno==EINTR)continue;
  if(n<=0){saved=false;break;}data+=n;left-=size_t(n);}
 if(saved && fsync(file))saved=false;
 if(close(file))saved=false;
 if(!saved || left){fputs("QGW8_TRACE_WRITE_FAILED; PARTIAL_FILE_UNTRUSTED AFTER_CLOSE\n",stderr);return 1;}
 const auto digest=PioneerModelWeightCache::Hash(trace->data(),trace->size()*sizeof(uint32_t));
#ifdef PIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134
 printf("QGW8_DENSE_VALUE_SNAPSHOT sha256=%s bytes=%zu tag=0x2026110700010134 generation=%llu; DATA_ONLY_AFTER_CLOSE\n",
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133)
 printf("QGW8_DENSE_INPUT_SNAPSHOT sha256=%s bytes=%zu tag=0x2026110700010133 generation=%llu; DATA_ONLY_AFTER_CLOSE\n",
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132)
 printf("QGW8_DENSE_STAGE_SNAPSHOT sha256=%s bytes=%zu tag=0x2026110700010132 generation=%llu; DATA_ONLY_AFTER_CLOSE\n",
#else
 printf("QGW8_TRACE_SNAPSHOT sha256=%s bytes=%zu tag=0x2026110700010131 generation=%llu; DATA_ONLY_AFTER_CLOSE\n",
#endif
  PioneerModelWeightCache::Hex(digest).c_str(),trace->size()*sizeof(uint32_t),(unsigned long long)traceGeneration);
#endif
 return fflush(stdout)?1:0;
}
