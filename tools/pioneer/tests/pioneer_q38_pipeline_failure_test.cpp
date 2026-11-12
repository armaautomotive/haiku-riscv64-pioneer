/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT 1
#define PIONEER_Q38_PREFETCH_WORKER_UNIT_TEST 1
#define PIONEER_Q38_DENSE_Q8_PREPARED_UPLOAD_FIXTURE_ONLY 1
#include "pioneer_q38_dense_q8_prepared_upload_test.cpp"
#include <atomic>
#include <chrono>
#include <thread>

struct PipelinePages {
 Plan::Proposal plan;std::thread::id owner=std::this_thread::get_id();
 std::atomic<bool> reading{false};std::atomic<unsigned> reads{0};unsigned failAt=0;
 bool AllVerified()const{assert(!reading.load());return true;}
 const auto* Part(unsigned i)const{return i<plan.parts.size()?&plan.parts[i]:nullptr;}
 bool ReadPages(unsigned,unsigned,unsigned count,uint8_t* output,unsigned capacity,std::string& error){
  assert(std::this_thread::get_id()!=owner && !reading.exchange(true) && capacity>=count*4096);
  const unsigned n=++reads;if(n==failAt){reading=false;error="injected page failure";return false;}
  std::memset(output,0x5a,count*4096);reading=false;return true;
 }
};
struct PipelineIO {
 std::thread::id owner=std::this_thread::get_id();unsigned calls=0;bool fail=false,badReply=false;
 PipelinePages* pages=nullptr;unsigned waitForRead=0;bool overlap=false;
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(std::this_thread::get_id()==owner && fd==42 && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  ++calls;auto& q=*static_cast<tt_q38_span_upload*>(data);
  if(waitForRead){
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   while(pages->reads.load()<waitForRead && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
   overlap=pages->reads.load()>=waitForRead;assert(overlap);
  }
  if(fail)return false;
  q.result={1,TT_RUNTIME_PASSED,q.bytes,q.bytes/4096,calls,1,1,1,0,0,0,0};
  if(badReply)q.result.nonce++;return true;
 }
};
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 for(unsigned mode=0;mode<4;mode++){
  PipelinePages pages;assert(Plan::Build(*canonical,pages.plan));
  unsigned first=0;while(canonical->Part(first)->kind!=canonical->WeightKind)first++;
  const unsigned firstPages=std::min(256U,pages.plan.parts[first].allocated/4096);
  assert(firstPages && first+1<canonical->Count());
  pages.failAt=mode==0?1:mode==1?(firstPages+15)/16+1:0;
  PipelineIO io;io.fail=mode==2;io.badReply=mode==3;io.pages=&pages;
  if(mode==1)io.waitForRead=pages.failAt;std::string error;
  PioneerQ38HybridSpanUpload<PipelinePages,PipelineIO,PioneerQ38DenseQ8UploadPlan> uploader;
  assert(!uploader.Run(pages,*canonical,io,42,error) && uploader.Failed() && !uploader.CompleteHeld());
  assert(io.calls==(mode?1U:0U) && uploader.DeviceTouched()==bool(mode) && !pages.reading.load());
  if(mode==1)assert(io.overlap);
 }
 puts("PASS pipeline page/IO/reply failures stop, join producer and retain uncertain owner; SYNTHETIC ONLY");
}
