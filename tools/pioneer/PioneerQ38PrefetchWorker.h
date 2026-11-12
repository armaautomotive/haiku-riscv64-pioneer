/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREFETCH_WORKER_H
#define PIONEER_Q38_PREFETCH_WORKER_H
#if !defined(PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT) || PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT != 1 \
 || (!defined(PIONEER_Q38_QGWF_RATE_CLIENT_144) && !defined(PIONEER_Q38_PREFETCH_WORKER_UNIT_TEST)) \
 || (defined(PIONEER_Q38_PREFETCH_WORKER_UNIT_TEST) && PIONEER_Q38_PREFETCH_WORKER_UNIT_TEST != 1) \
 || (defined(PIONEER_Q38_QGWF_RATE_CLIENT_144) && defined(PIONEER_Q38_PREFETCH_WORKER_UNIT_TEST)) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC)
#error The prepared-page prefetch worker is an isolated userspace diagnostic experiment
#endif
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

// Bounded single-job worker for file/cache preparation only. The owner thread
// retains all accelerator calls and may not touch the source or destination
// while a job is outstanding. Destruction joins even after a failed device
// call; this object cannot release, retry, reset, or otherwise access a device.
class PioneerQ38PrefetchWorker {
 std::mutex lock;std::condition_variable changed;std::thread thread;
 std::function<bool(std::string&)> job;std::string result;
 bool shutdown=false,pending=false,finished=false,okay=false,started=false;
 void Loop(){
  std::unique_lock<std::mutex> guard(lock);
  for(;;){
   changed.wait(guard,[&]{return shutdown || bool(job);});
   if(shutdown && !job)return;
   auto task=std::move(job);job={};guard.unlock();std::string message;bool success=false;
   try{success=task(message);}catch(...){message="Prepared-page worker threw an exception.";}
   guard.lock();result=std::move(message);okay=success;finished=true;changed.notify_all();
  }
 }
public:
 PioneerQ38PrefetchWorker()=default;
 PioneerQ38PrefetchWorker(const PioneerQ38PrefetchWorker&)=delete;
 PioneerQ38PrefetchWorker& operator=(const PioneerQ38PrefetchWorker&)=delete;
 bool Start(){
  std::lock_guard<std::mutex> guard(lock);if(started || shutdown)return false;
  try{thread=std::thread([this]{Loop();});}catch(...){return false;}
  started=true;return true;
 }
 bool Submit(std::function<bool(std::string&)> task){
  std::lock_guard<std::mutex> guard(lock);
  if(!started || shutdown || pending || !task)return false;
  job=std::move(task);result.clear();okay=false;finished=false;pending=true;changed.notify_all();return true;
 }
 bool Wait(std::string& error){
  std::unique_lock<std::mutex> guard(lock);if(!started || !pending)return false;
  changed.wait(guard,[&]{return finished;});const bool success=okay;error=std::move(result);
  pending=false;finished=false;changed.notify_all();return success;
 }
 ~PioneerQ38PrefetchWorker(){
  std::unique_lock<std::mutex> guard(lock);
  if(pending){changed.wait(guard,[&]{return finished;});pending=false;}
  shutdown=true;changed.notify_all();guard.unlock();if(thread.joinable())thread.join();
 }
};
#endif
