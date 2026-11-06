/* SPDX-License-Identifier: MIT */
// Full-offload candidate,not a switch inside the hybrid ggml worker.
// CPU: full-file authentication,weight preparation/upload,vocabulary,prompt
// tokenization,channel/lifecycle and detokenization. NO inference context.
#include <Drivers.h>
#include <OS.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <cstdio>
#include <memory>
#include "PioneerQ38FullOffloadSession.h"
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
#include "PioneerQ38TokenRateSamples.h" // observation arithmetic,NO model reference
#endif
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
#include "PioneerQ38StartupTiming.h"
#endif
#include "../../src/apps/assistant/Qwen27Protocol.h"
#include "llama.h"
namespace A=Assistant27;
namespace F=PioneerQ38FullOffload;
static double Now(){timespec t={};return clock_gettime(CLOCK_MONOTONIC,&t)?-1:double(t.tv_sec)+double(t.tv_nsec)*1e-9;}
static void Log(ggml_log_level level,const char* text,void*){
 if(level==GGML_LOG_LEVEL_WARN || level==GGML_LOG_LEVEL_ERROR)fputs(text,stderr);
}
struct Runtime {
 struct Transport {
  Runtime& runtime;
  int Open(){return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
  bool Call(int fd,unsigned operation,void* data,size_t bytes){
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
   if(operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && bytes==sizeof(tt_q38_span_upload)){
    const auto count=static_cast<const tt_q38_span_upload*>(data)->bytes;
    const auto before=system_time();const bool okay=!runtime.lost && ioctl(fd,operation,data,bytes)==0;
    runtime.startup.Upload(before,system_time(),count);return okay;
   }
#endif
   return !runtime.lost && ioctl(fd,operation,data,bytes)==0;
  }
  int Close(int fd){return close(fd);}
  void Progress(uint64_t bytes,uint64_t total){
   runtime.PollCancel();const double now=Now();
   if(now<0 || now<runtime.lastProgress){runtime.lost=true;return;}
   if(now-runtime.lastProgress<.5 && bytes!=total)return;
   runtime.lastProgress=now;char message[224];
   snprintf(message,sizeof(message),"Full-card Qwen27B upload %.1f/%.1f MiB.%s No CPU inference.",
    double(bytes)/1048576,double(total)/1048576,runtime.cancelled?" Stop pending at safe card boundary.":"");
   runtime.Emit(A::Progress,total?unsigned(bytes*1000/total):UINT32_MAX,message);
  }
 } io{*this};
 using Session=PioneerQ38FullOffloadSession<Transport>;
 PioneerQ38PublicModelSource source;
 std::unique_ptr<Session> session;
 llama_model* vocabularyModel=nullptr;
 A::Receiver receiver;uint32_t request=0;
 unsigned eos0=0,eos1=0;bool loaded=false,lost=false,cancelled=false,failed=false;
 const char* vocabularyFailure="not_started";
 double lastProgress=0;
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
 PioneerQ38StartupTiming startup;
#endif
 bool Emit(unsigned kind,unsigned value=0,const std::string& text=""){
  A::Frame frame;frame.kind=kind;frame.id=request;frame.value=value;frame.text=text;
  if(!A::Send(1,frame))lost=true;
  return !lost;
 }
 bool PollCancel(){
  for(;;){
   A::Frame frame;auto result=receiver.Read(0,frame);
   if(result==A::Receiver::Waiting)return !lost;
   if(result!=A::Receiver::Received || frame.kind!=A::Cancel || frame.id!=request || frame.value || !frame.text.empty()){
    lost=true;return false;
   }
   cancelled=true;
  }
 }
 bool LoadVocabulary(){
  vocabularyFailure="source_authentication";
  if(vocabularyModel || !source.Open())return false;
  vocabularyFailure="source_borrow";
  FILE* file=source.BorrowForModelLoad();if(!file)return false;
  llama_log_set(Log,nullptr);
  auto params=llama_model_default_params();ggml_backend_dev_t noDevices[]={nullptr};
  params.vocab_only=true;params.devices=noDevices;params.n_gpu_layers=0;params.use_extra_bufts=false;
  vocabularyFailure="vocabulary_load";
  vocabularyModel=llama_model_load_from_file_ptr(file,params);
  if(!vocabularyModel || !source.Ready())return false;
  const auto* vocab=llama_model_get_vocab(vocabularyModel);
  const int first=llama_vocab_eos(vocab),second=llama_vocab_eot(vocab);
  vocabularyFailure="vocabulary_or_eos_metadata";
  if(llama_vocab_n_tokens(vocab)!=F::Vocabulary || first<0 || first>=F::Vocabulary)return false;
  if(second>=F::Vocabulary)return false;
  eos0=PioneerQ38FullOffloadEog::Eos0;eos1=PioneerQ38FullOffloadEog::Eos1;
  vocabularyFailure="model_eog_set_mismatch";
  for(unsigned id=0;id<F::Vocabulary;id++)if(llama_vocab_is_eog(vocab,int(id))!=PioneerQ38FullOffloadEog::Contains(id))return false;
  vocabularyFailure="source_changed";
  if(!source.Ready())return false;
  vocabularyFailure=nullptr;
  return true;
 } // source/vocabulary ONLY;no accelerator FD or model context
 bool Load(unsigned threads){
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
  startup.LoadBegin(system_time());
#endif
  if(loaded || !threads || threads>64 || !Emit(A::Progress,UINT32_MAX,
   "Authenticating Qwen27B; CPU will load vocabulary only,not model weights or an inference context.") || !LoadVocabulary())return false;
  session=std::make_unique<Session>(io,source);
  if(!session->Probe())return false; // metadata only,never BEGIN during Load
  loaded=true;
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
  startup.LoadReady(system_time());
#endif
  return Emit(A::Ready,0,"QWEN27_FULL_OFFLOAD_VOCAB_READY");
 }
 bool Generate(const A::Frame& frame){
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
  const auto requestBegin=system_time();
#endif
  if(!loaded || failed || lost)return false;
  request=frame.id;cancelled=false;
  if(!A::Question(frame.text) || !frame.value || frame.value>F::MaxOutput)
   return Emit(A::Done,A::Rejected,"Full-card request requires a short question and1-32 output tokens; no fallback.");
  const std::string prompt=A::Prompt(frame.text);const auto* vocab=llama_model_get_vocab(vocabularyModel);
  llama_token tokens[F::MaxPrompt];
  const int count=llama_tokenize(vocab,prompt.data(),int(prompt.size()),tokens,F::MaxPrompt,false,true);
  if(count<1 || count>F::MaxPrompt || unsigned(count)+frame.value-1>F::MaxPositions)
   return Emit(A::Done,A::Rejected,"Prompt exceeds96 tokens; shorten it. No CPU inference or fallback.");
  uint32_t ids[F::MaxPrompt]={};for(int i=0;i<count;i++){if(tokens[i]<0)return false;ids[i]=unsigned(tokens[i]);}
  // First session has authenticated/probed only;following clean requests
  // need a fresh owner/upload. Persistent weight reuse is NOT claimed yet.
  if(session->Completed())session=std::make_unique<Session>(io,source);
  const double began=Now();if(began<0 || !PollCancel())return false;
  if(cancelled)return Emit(A::Done,A::Cancelled,"Stopped before accelerator BEGIN.");
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
  startup.Begin(requestBegin,unsigned(count),unsigned(count)+frame.value-1);
#endif
  if(!session->Begin(ids,unsigned(count),frame.value,eos0,eos1))return false;
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
  startup.Ready(system_time());
#endif
  if(!PollCancel())return false;
  const double computeStarted=Now();double previous=computeStarted;
  if(computeStarted<0 || !Emit(A::Progress,UINT32_MAX,
   "Qwen27B running entirely on P100A: embeddings,matrices,attention,KV,recurrent state,norms and greedy selection. CPU inference OFF."))return false;
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
  PioneerQ38TokenRateSamples samples(unsigned(count),unsigned(count)+frame.value-1);
  bool timingValid=true;unsigned measured=0,previousCount=UINT32_MAX;
  auto observeCount=[&](){
   const auto before=system_time();F::CounterPacket metric;
   if(!session->ObserveCount(metric))return false;
   const auto after=system_time();
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
   startup.Observe(metric.reply.detail[7],metric.reply.detail[9],metric.reply.detail[8],before,after);
#endif
   if(timingValid && !samples.Observe(metric,before,after))timingValid=false;
   measured++;
   if(metric.reply.detail[7]!=previousCount){
    previousCount=metric.reply.detail[7];char status[192];
    snprintf(status,sizeof(status),"Full-card count committed%u prompts%u limit%u;read-only DATA,not a terminal or cleanup receipt.",
     metric.reply.detail[7],metric.reply.detail[9],metric.reply.detail[8]);
    if(!Emit(A::Progress,UINT32_MAX,status))return false;
   }
   return true;
  };
#endif
  for(unsigned observations=0;!session->TerminalReady();observations++){
   const double now=Now();
   if(now<previous || now-computeStarted>=1800 || observations>=18000 || !PollCancel())return false;
   previous=now;
   if(cancelled && !session->CancelRequested() && !session->Cancel())return false;
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
   if(!observeCount())return false;
#endif
   if(!session->Poll())return false;
   if(!session->TerminalReady() && snooze(100000)!=B_OK)return false;
  }
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
  if(!observeCount())return false; // final SAME-FD count AFTER genuine terminal,BEFORE HOLD
#endif
  const double computeFinished=Now();
  if(computeFinished<previous || !session->Finish())return false;
  uint32_t output[F::MaxOutput]={};unsigned generated=0;
  if(!session->CopyGeneratedTokens(output,F::MaxOutput,generated))return false;
  std::string answer;
  for(unsigned i=0;i<generated;i++){
   if(llama_vocab_is_eog(vocab,int(output[i])))return false;
   char piece[1024];const int bytes=llama_token_to_piece(vocab,int(output[i]),piece,sizeof(piece),0,true);
   if(bytes<0 || answer.size()+unsigned(bytes)>A::MaxAnswer)return false;
   answer.append(piece,bytes);
  }
  if(!PollCancel() || !source.Ready())return false;
  if(cancelled || session->WasCancelled())return Emit(A::Done,A::Cancelled,"Card stopped safely; all resources restored. No CPU fallback.");
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
  // Only after genuine full END+Close,record/token checks and final source
  // stability. Counter DATA alone never grants successful publication.
  PioneerQ38TokenRateSamples::Bounds bounds;PioneerQ38TokenRateSamples::Window window;char metric[768];
  if(timingValid && samples.ObservedBounds(bounds) && samples.ObservedWindow(window) && bounds.sampledTokens>=16)
   snprintf(metric,sizeof(metric),"Full-card observed count rate sampled%u first%u last%u elapsed_min_us%lld elapsed_max_us%lld lower_tps%.9f upper_tps%.9f observations%u first_begin_us%lld first_end_us%lld last_begin_us%lld last_end_us%lld;AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality.",
    bounds.sampledTokens,bounds.firstCommitted,bounds.lastCommitted,(long long)bounds.minElapsedUs,(long long)bounds.maxElapsedUs,
    bounds.lowerTokensPerSecond,bounds.upperTokensPerSecond,measured,(long long)window.firstBegin,(long long)window.firstEnd,
    (long long)window.lastBegin,(long long)window.lastEnd);
  else snprintf(metric,sizeof(metric),"Full-card rate unavailable: insufficient post-prompt window or invalid timing;AFTER_CLEAN_END,no extrapolation.");
  if(!Emit(A::Progress,UINT32_MAX,metric))return false;
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE
  {namespace Q=PioneerQ38SelectedPhaseProfileData;
   PioneerQ38TokenPhaseProfileControl::Packet p;
   if(!session->CopyPhaseProfile(p))return false;
   char description[320];
   snprintf(description,sizeof(description),"Full-card phase profile valid%u committed%u begin_cycles%llu end_cycles%llu state%u error%u;AFTER_CLEAN_END,clock DATA,no frequency assumption.",
    p.reply.detail[7],p.reply.detail[8],(unsigned long long)Q::Get(p.words,Q::BeginCycle),
    (unsigned long long)Q::Get(p.words,Q::EndCycle),p.words[Q::State],p.words[Q::Error]);
   if(!Emit(A::Progress,UINT32_MAX,description))return false;
   if(p.reply.detail[7])for(unsigned phase=0;phase<Q::Phases;phase++){
    const unsigned bucket=Q::Buckets+phase*3;
    snprintf(description,sizeof(description),"Full-card phase profile bucket%u segments%u cycles%llu;AFTER_CLEAN_END,clock DATA,no frequency assumption.",
     phase,p.words[bucket],(unsigned long long)Q::Get(p.words,bucket+1));
    if(!Emit(A::Progress,UINT32_MAX,description))return false;
   }
  }
#endif
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
  // All genuine END/Close,token/source/cancel checks above have succeeded.
  // Answer-ready is not first visible text: IPC/UI delivery happens later.
  char startupMetric[768];
  if(!startup.Format(system_time(),startupMetric,sizeof(startupMetric))
   || !Emit(A::Progress,UINT32_MAX,startupMetric))return false;
#endif
  if(!Emit(A::Text,0,answer))return false;
  char status[320];snprintf(status,sizeof(status),
   "P100A full-offload candidate: %u output tokens; prompt+generation %.3fs,total %.3fs. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement.",
   generated,computeFinished-computeStarted,Now()-began);
  return Emit(A::Done,A::Complete,status);
 }
 bool Close(){
  const bool clean=!failed && !lost && (!session || session->KnownClean());
  if(!clean && session)session->Abort();
  session.reset();
  if(vocabularyModel){llama_model_free(vocabularyModel);vocabularyModel=nullptr;}
  loaded=false;return clean;
 }
 ~Runtime(){Close();}
};
int main(int argc,char** argv){
 if(argc==2 && !strcmp(argv[1],"--vocabulary-only")){
  Runtime runtime;
  if(!runtime.LoadVocabulary()){
   fprintf(stderr,"FAIL_FULL_OFFLOAD_VOCABULARY_ONLY reason%s;NO_ACCELERATOR_OPEN_OR_INFERENCE\n",runtime.vocabularyFailure?runtime.vocabularyFailure:"unknown");
   if(runtime.vocabularyModel && runtime.source.Ready()){
    const auto* vocab=llama_model_get_vocab(runtime.vocabularyModel);
    const int size=llama_vocab_n_tokens(vocab);
    fprintf(stderr,"VOCABULARY_METADATA count%d eos%d eot%d eog_ids",size,llama_vocab_eos(vocab),llama_vocab_eot(vocab));
    if(size>0 && size<=F::Vocabulary)for(int id=0;id<size;id++)if(llama_vocab_is_eog(vocab,id))fprintf(stderr," %d",id);
    fputc('\n',stderr);
   }
   return 1;
  }
  printf("PASS_FULL_OFFLOAD_VOCABULARY_ONLY vocabulary%u eos%u eot%u;whole_original_SHA/all851_layout;NO_ACCELERATOR_OPEN_OR_INFERENCE\n",
   F::Vocabulary,runtime.eos0,runtime.eos1);
  return runtime.Close()?0:1;
 }
 if(argc!=2 || strcmp(argv[1],"--assistant-channel"))return 2;
 Runtime runtime;A::Frame frame;uint32_t previous=0;
 for(;;){
  const auto result=runtime.receiver.Read(0,frame);
  if(result==A::Receiver::Waiting){pollfd p={0,POLLIN,0};if(poll(&p,1,-1)<0 && errno!=EINTR)break;continue;}
  if(result!=A::Receiver::Received)break;
  if(frame.kind==A::Cancel && frame.id==previous && !frame.value && frame.text.empty())continue;
  if(frame.id<=previous)break;
  previous=runtime.request=frame.id;
  if(frame.kind==A::Load && !runtime.loaded && frame.text.empty()){
   if(runtime.Load(frame.value))continue;
   runtime.Close();runtime.Emit(A::Error,A::Rejected,
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
    "Full-card vocabulary/model authentication or metrics tag90 capability unavailable; no inference or CPU fallback."
#elif defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED)
    "Full-card vocabulary/model authentication or batched tag89 capability unavailable; no inference or CPU fallback."
#else
    "Full-card vocabulary/model authentication or tag88 capability unavailable; no inference or CPU fallback."
#endif
    );return 1;
  }
  if(frame.kind==A::Generate && runtime.loaded){if(runtime.Generate(frame))continue;break;}
  if(frame.kind==A::Threads && runtime.loaded && frame.text.empty() && frame.value && frame.value<=64){
   if(runtime.Emit(A::Ready,frame.value,"QWEN27_FULL_OFFLOAD_CPU_INFERENCE_UNUSED"))continue;
   break;
  }
  if(frame.kind==A::Unload && !frame.value && frame.text.empty()){
   if(runtime.Close() && runtime.Emit(A::Closed,0,"QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END"))return 0;
   break;
  }
  break;
 }
 runtime.failed=true;runtime.lost=true;if(runtime.session)runtime.session->Abort();
 runtime.Emit(A::Error,A::Fatal,"Full-card execution/channel failed: close only,retained owners,no END/retry/reset or CPU fallback.");
 runtime.Close();return 70;
}
