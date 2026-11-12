/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT 1
#define PIONEER_Q38_PREFETCH_WORKER_UNIT_TEST 1
#include "../PioneerQ38PrefetchWorker.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <future>
#include <stdexcept>
int main(){
 std::atomic<unsigned> count{0};std::string error;
 {
  PioneerQ38PrefetchWorker worker;assert(!worker.Wait(error) && worker.Start() && !worker.Start());
  std::promise<void> entered,release;auto proceed=release.get_future();
  assert(worker.Submit([&](std::string&){entered.set_value();proceed.wait();count++;return true;}));
  entered.get_future().wait();assert(!worker.Submit([](std::string&){return true;}));
  // Owner can perform independent work while preparation is blocked.
  assert(count==0);release.set_value();assert(worker.Wait(error) && count==1 && !worker.Wait(error));
  for(unsigned i=0;i<10000;i++){
   assert(worker.Submit([&](std::string&){count++;return true;}));assert(worker.Wait(error));
  }
  assert(worker.Submit([](std::string& e){e="synthetic read failure";return false;}));
  assert(!worker.Wait(error) && error=="synthetic read failure");
  assert(worker.Submit([](std::string&)->bool{throw std::runtime_error("test");}));
  assert(!worker.Wait(error) && error=="Prepared-page worker threw an exception.");
  assert(worker.Submit([&](std::string&){count++;return true;}));
 } // destructor joins the outstanding job
 assert(count==10002);
 puts("PASS bounded prepared-page worker overlap, failure and join; NO DEVICE OR MODEL");
}
