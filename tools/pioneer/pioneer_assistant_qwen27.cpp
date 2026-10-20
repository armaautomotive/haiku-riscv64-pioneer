/* SPDX-License-Identifier: MIT */
// Isolated persistent Assistant worker: pinned PUBLIC model only, WORK
// matrices, CPU non-matrix ops, NO CPU learned reference/fallback. Does NOT
// enable the unqualified mutable-state/zero/GDN images.
#include "TenstorrentHaikuQwen38.h"
#include "PioneerQ38GgmlBackend.h"
#include "../../src/apps/assistant/Qwen27Protocol.h"
#include "../../src/apps/assistant/Qwen27Pool.h"
#include "llama.h"
#include <poll.h>
#include <memory>
namespace A=Assistant27;
static double Now() { timespec t={}; return clock_gettime(CLOCK_MONOTONIC,&t)?-1:double(t.tv_sec)+double(t.tv_nsec)*1e-9; }
static void Log(ggml_log_level level,const char* text,void*) { if (level>=GGML_LOG_LEVEL_WARN) fputs(text,stderr); }
struct Runtime {
 PioneerQ38PublicModelSource source;
 PioneerQ38NativeTransport io;
 PioneerQ38NativeSession session;
 llama_model* model=nullptr; llama_context* context=nullptr;
 A::PoolOwner<ggml_threadpool_t,ggml_threadpool_free> pools;
 A::Receiver receiver; uint32_t request=0; bool opened=false,cancelled=false,lost=false,configured=false;
 double lastProgress=0;
 explicit Runtime():session(io,source) {}
 bool Emit(unsigned kind,unsigned value=0,const std::string& text="") {
  A::Frame f; f.kind=kind; f.id=request; f.value=value; f.text=text;
  if (!A::Send(1,f)) lost=true;
  return !lost;
 }
 bool PollCancel() {
  for (;;) {
   A::Frame f; auto r=receiver.Read(0,f);
   if (r==A::Receiver::Waiting) return !lost;
   if (r!=A::Receiver::Received || f.kind!=A::Cancel || f.id!=request || f.value || !f.text.empty()) { lost=true; return false; }
   cancelled=true;
  }
 }
 static bool Ready(void* p) { auto& r=*static_cast<Runtime*>(p); return !r.lost && r.source.Ready() && !r.session.Failed(); }
 static bool Finish(void* p) { return static_cast<Runtime*>(p)->session.Finish(); }
 static void Abort(void* p) { static_cast<Runtime*>(p)->session.Abort(); }
 static bool UploadProgress(void* p,uint64_t bytes,uint64_t total) {
  auto& r=*static_cast<Runtime*>(p); if (!r.PollCancel()) return false;
  double now=Now(); if (now-r.lastProgress<.5 && bytes!=total) return true;
  r.lastProgress=now; char status[192]; snprintf(status,sizeof(status),"%sQwen27B P100A upload:%.1f/%.1f MiB. Stop waits for this token boundary.",r.cancelled?"Stop pending; finishing token. ":"",double(bytes)/1048576,double(total)/1048576);
  return r.Emit(A::Progress,total?unsigned(bytes*1000/total):UINT32_MAX,status);
 }
 static bool Multiply(void* p,unsigned tensor,const float* x,unsigned k,float* y,unsigned n) {
  auto& r=*static_cast<Runtime*>(p);
  if (!r.PollCancel()) { r.session.Abort(); return false; }
  if (!r.opened) {
   if (!r.session.Open()) return false;
   r.opened=true; r.session.Observe(UploadProgress,&r);
   if (!r.session.PrimeSpan()) return false;
   if (!r.Emit(A::Progress,UINT32_MAX,"P100A weights resident; evaluating prompt. CPU attention/state; validation OFF.")) { r.session.Abort(); return false; }
  }
  return r.session.Multiply(tensor,x,k,y,n);
 }
 bool Load(unsigned threads) {
  if (!threads || threads>64 || !Emit(A::Progress,UINT32_MAX,"Authenticating pinned19GB Qwen27B file (same file descriptor; no accelerator session yet).") || !source.Open()) return false;
  // Capability query is CPU metadata only; BEGIN is deferred until reply.
  int fd=io.Open(); if (fd<0) return false;
  PioneerQ38WorkSessionMode mode; bool supported=mode.Support(io,fd); if (io.Close(fd)) supported=false;
  if (!supported) return false;
  FILE* file=source.BorrowForModelLoad(); if (!file) return false;
  llama_log_set(Log,nullptr); llama_backend_init();
  llama_model_params mp=llama_model_default_params(); ggml_backend_dev_t hostOnly[]={nullptr};
  mp.devices=hostOnly; mp.n_gpu_layers=0; mp.load_mode=LLAMA_LOAD_MODE_MMAP; mp.use_extra_bufts=false;
  if (!Emit(A::Progress,UINT32_MAX,"Loading Qwen27B host mmap/context; P100A upload occurs on first reply.")) return false;
  model=llama_model_load_from_file_ptr(file,mp); if (!model || !source.Ready()) return false;
  PioneerQ38GgmlDispatch d={this,&source.Model(),Ready,Multiply,Finish,Abort};
  if (!PioneerQ38GgmlConfigure(d)) return false;
  configured=true; ggml_backend_register(PioneerQ38GgmlReg());
  auto cp=llama_context_default_params(); cp.n_ctx=128; cp.n_batch=cp.n_ubatch=1;
  cp.n_threads=cp.n_threads_batch=threads; cp.offload_kqv=false; cp.op_offload=true; cp.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_DISABLED;
  context=llama_init_from_model(model,cp); if (!context) return false;
  auto tp=ggml_threadpool_params_default(threads); tp.poll=0; auto pool=ggml_threadpool_new(&tp);
  if (!pool || !pools.Replace(pool)) return false;
  llama_attach_threadpool(context,pool,pool);
  return source.Ready() && PioneerQ38GgmlSnapshot().recognizedWeights==497 && Emit(A::Ready,0,"QWEN27_WORK_HOST_READY_497");
 }
 bool Generate(const A::Frame& f) {
  request=f.id; cancelled=false;
  if (!A::Question(f.text) || f.value<1 || f.value>A::MaxTokens) return Emit(A::Done,A::Rejected,"Qwen27B question/token limit exceeded; no request executed.");
  std::string prompt=A::Prompt(f.text),answer; const auto vocab=llama_model_get_vocab(model);
  llama_token tokens[128]; int count=llama_tokenize(vocab,prompt.data(),int(prompt.size()),tokens,128,false,true);
  if (count<1 || count>A::MaxPromptTokens || count+int(f.value)>128) return Emit(A::Done,A::Rejected,"Qwen27B prompt exceeds96 tokens. Shorten the question; no fallback.");
  llama_memory_clear(llama_get_memory(context),true); // CPU recurrent/KV reset,NOT weights/re-upload
  unsigned generated=0,warmSamples=0; double warm=0,began=Now(); llama_token token=0;
  for (int position=0;generated<f.value;position++) {
   if (!PollCancel()) return false;
   if (cancelled) return Emit(A::Done,A::Cancelled,"Stopped at a complete token boundary; resident weights retained. No CPU fallback.");
   if (position<count) token=tokens[position];
   auto before=PioneerQ38GgmlSnapshot(); double start=Now();
   char phase[192];
   if (position<count) snprintf(phase,sizeof(phase),"Evaluating Qwen27B prompt token %d of %d on P100A (slow Work mode).",position+1,count);
   else snprintf(phase,sizeof(phase),"Generating Qwen27B token %u of at most %u; CPU attention/state; no CPU reference.",generated+1,f.value);
   if (!Emit(A::Progress,UINT32_MAX,phase)) return false;
   auto batch=llama_batch_get_one(&token,1);
   if (llama_decode(context,batch)) return false;
   auto after=PioneerQ38GgmlSnapshot();
   if (!source.Ready() || after.failed || after.projections-before.projections!=497 || !opened) return false;
   if (pools.NeedsRetirement()) llama_synchronize(context);
   pools.AfterSuccessfulDecode(); // CPU backend has safely rebound its pool
   if (position>=count) { warm+=Now()-start; warmSamples++; }
   if (!PollCancel()) return false;
   if (cancelled) return Emit(A::Done,A::Cancelled,"Stopped at a complete token boundary; resident weights retained. No CPU fallback.");
   const float* logits=llama_get_logits_ith(context,-1); int best=-1,size=llama_vocab_n_tokens(vocab);
   if (!logits) return false;
   for (int i=0;i<size;i++) { if (!isfinite(logits[i])) return false; if (best<0 || logits[i]>logits[best]) best=i; }
   if (position+1<count) continue;
   if (llama_vocab_is_eog(vocab,best)) break;
   char piece[1024]; int n=llama_token_to_piece(vocab,best,piece,sizeof(piece),0,true);
   if (n<0 || answer.size()+unsigned(n)>A::MaxAnswer) return false;
   answer.append(piece,n); token=best; generated++;
   if (!Emit(A::Text,0,answer)) return false;
  }
  char status[320]; snprintf(status,sizeof(status),"P100A Qwen27B Work:%u output tokens,%.1fs; warm generation%u samples/%.3fs (%.3f tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.",generated,Now()-began,warmSamples,warm,warm>0?warmSamples/warm:0);
  return Emit(A::Done,A::Complete,status);
 }
 bool Threads(unsigned threads) {
  if (!threads || threads>64) return Emit(A::Error,A::Rejected,"Invalid CPU thread count; previous pool retained.");
  auto tp=ggml_threadpool_params_default(threads); tp.poll=0;
  ggml_threadpool_t replacement=ggml_threadpool_new(&tp);
  if (!replacement) return Emit(A::Error,A::Rejected,"CPU pool allocation failed; previous pool retained.");
  // Main command loop ONLY: a complete graph/request has returned. No
  // accelerator call, memory clear, BEGIN/END, source reopen or weight upload.
  llama_synchronize(context); llama_detach_threadpool(context);
  llama_set_n_threads(context,threads,threads);
  llama_attach_threadpool(context,replacement,replacement);
  if (!pools.Replace(replacement)) return false; // retains last backend-bound pool
  return Emit(A::Ready,threads,"QWEN27_WORK_THREADS_READY");
 }
 bool Close() {
  bool clean=!lost && !session.Failed() && (configured?PioneerQ38GgmlFinish():session.Finish());
  if (!clean) session.Abort();
  if (context) { llama_synchronize(context); llama_detach_threadpool(context); llama_free(context); context=nullptr; }
  pools.AfterContextFree();
  if (model) { llama_model_free(model); model=nullptr; }
  return clean;
 }
};
int main(int argc,char** argv) {
 if (argc!=2 || strcmp(argv[1],"--assistant-channel")) return 2;
 Runtime r; A::Frame f; uint32_t previous=0;
 for (;;) {
  auto result=r.receiver.Read(0,f);
  if (result==A::Receiver::Waiting) { pollfd p={0,POLLIN,0}; if (poll(&p,1,-1)<0 && errno!=EINTR) break; continue; }
  if (result!=A::Receiver::Received) break;
  if (f.kind==A::Cancel && f.id==previous && !f.value && f.text.empty()) continue; // late Stop after known Done
  if (f.id<=previous) break;
  previous=r.request=f.id;
  if (f.kind==A::Load && !r.model && f.text.empty()) {
   if (r.Load(f.value)) continue;
   // Load never submits BEGIN. Authentication/capability/context failure
   // cannot become a silently enabled CPU model or a card execution.
   r.Close(); r.Emit(A::Error,A::Rejected,"Qwen27B authentication, Work driver capability or context load failed; no accelerator BEGIN or fallback."); return 1;
  }
  if (f.kind==A::Generate && r.context) { if (r.Generate(f)) continue; break; }
  if (f.kind==A::Threads && r.context && f.text.empty()) { if (r.Threads(f.value)) continue; break; }
  if (f.kind==A::Unload && !f.value && f.text.empty()) {
   bool clean=r.Close(); if (clean && r.Emit(A::Closed,0,"QWEN27_KNOWN_CLEAN_END")) return 0; break;
  }
  break;
 }
 r.lost=true; r.session.Abort(); r.Emit(A::Error,A::Fatal,"Qwen27B execution/channel failed; CLOSE ONLY, retained owners, no END/retry/reset/CPU fallback.");
 r.Close(); return 70;
}
