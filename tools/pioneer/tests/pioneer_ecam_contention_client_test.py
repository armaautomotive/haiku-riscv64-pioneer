#!/usr/bin/env python3
"""Run the actual Haiku contention client against a simulated OS/driver."""
import os
from pathlib import Path
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='q35-contention-client-') as temporary:
    tmp = Path(temporary)
    (tmp / 'Drivers.h').write_text('''#pragma once
#include <cstdint>
using uint32=uint32_t;using int32=int32_t;using status_t=int;
enum {B_DEVICE_OP_CODES_END=9999};
''')
    (tmp / 'OS.h').write_text('''#pragma once
#include <Drivers.h>
using sem_id=int;using thread_id=int;using bigtime_t=int64_t;
enum {B_RELATIVE_TIMEOUT=1,B_NORMAL_PRIORITY=10};
using thread_func=int32(*)(void*);
sem_id create_sem(int32,const char*);status_t delete_sem(sem_id);status_t release_sem(sem_id);
status_t acquire_sem_etc(sem_id,int32,uint32,bigtime_t);
thread_id spawn_thread(thread_func,const char*,int32,void*);status_t resume_thread(thread_id);
status_t wait_for_thread_etc(thread_id,uint32,bigtime_t,status_t*);
bigtime_t system_time();status_t snooze(bigtime_t);
''')
    (tmp / 'Errors.h').write_text('''#pragma once
enum {B_OK=0,B_ERROR=-1,B_BAD_VALUE=-2,B_NO_INIT=-3,B_CANCELED=-4,B_NOT_ALLOWED=-5,B_TIMED_OUT=-6};
''')
    harness = r'''
#include <cstdarg>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <cassert>
#define open fake_open
#define close fake_close
#define ioctl fake_ioctl
#define main contention_client_main
#include "SOURCE"
#undef main
#undef ioctl
#undef close
#undef open
using namespace std::chrono;
struct Sem {std::mutex lock;std::condition_variable cv;int count=0;};
struct Thread {thread_func fn;void* arg;std::thread thread;std::mutex lock;std::condition_variable cv;bool done=false;int32 result=0;};
static std::map<int,std::shared_ptr<Sem>> sems;
static std::map<int,std::shared_ptr<Thread>> threads;
static int nextSem=10,nextThread=100,mode=0;
static thread_local int current=-1;
static std::mutex leaseLock;static std::condition_variable leaseCv;static bool active=false;static int waiter=-1,waiters=0;
sem_id create_sem(int32 count,const char*) {auto p=std::make_shared<Sem>();p->count=count;int id=nextSem++;sems[id]=p;return id;}
status_t delete_sem(sem_id id) {return sems.erase(id)==1?B_OK:B_ERROR;}
status_t release_sem(sem_id id) {auto p=sems.at(id);std::lock_guard<std::mutex> lock(p->lock);++p->count;p->cv.notify_one();return B_OK;}
status_t acquire_sem_etc(sem_id id,int32,uint32,bigtime_t timeout) {auto p=sems.at(id);std::unique_lock<std::mutex> lock(p->lock);
 if(!p->cv.wait_for(lock,microseconds(timeout),[&]{return p->count>0;}))return B_TIMED_OUT;--p->count;return B_OK;}
thread_id spawn_thread(thread_func fn,const char*,int32,void* arg) {auto p=std::make_shared<Thread>();p->fn=fn;p->arg=arg;int id=nextThread++;threads[id]=p;return id;}
status_t resume_thread(thread_id id) {auto p=threads.at(id);p->thread=std::thread([p,id]{current=id;int32 r=p->fn(p->arg);
 {std::lock_guard<std::mutex> lock(p->lock);p->result=r;p->done=true;}p->cv.notify_one();});return B_OK;}
status_t wait_for_thread_etc(thread_id id,uint32,bigtime_t timeout,status_t* result) {auto p=threads.at(id);
 {std::unique_lock<std::mutex> lock(p->lock);if(!p->cv.wait_for(lock,microseconds(timeout),[&]{return p->done;}))return B_TIMED_OUT;*result=p->result;}
 p->thread.join();threads.erase(id);return B_OK;}
bigtime_t system_time(){return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();}
status_t snooze(bigtime_t us){std::this_thread::sleep_for(microseconds(us));return B_OK;}
extern "C" int fake_open(const char*,int,...) {static int fd=3;return fd++;}
extern "C" int fake_close(int) {return 0;}
extern "C" int fake_ioctl(int,unsigned long op,...) {
 va_list args;va_start(args,op);void* data=va_arg(args,void*);va_end(args);
 if(op==PIONEER_ECAM_LEASE_HOLD){auto& h=*static_cast<pioneer_ecam_lease_hold*>(data);
  {std::lock_guard<std::mutex> lock(leaseLock);active=true;waiter=-1;waiters=0;}release_sem(h.ready);
  h.callbackStatus=acquire_sem_etc(h.release,1,B_RELATIVE_TIMEOUT,2000000);
  {std::lock_guard<std::mutex> lock(leaseLock);active=false;}leaseCv.notify_all();
  h.holdStatus=B_OK;h.profileStatus=h.callbackStatus==B_OK?B_OK:B_ERROR;h.profileStage=6;h.profileReady=1;return 0;
 }
 if(op==PIONEER_ECAM_LEASE_STATE){auto& s=*static_cast<pioneer_ecam_lease_state*>(data);std::lock_guard<std::mutex> lock(leaseLock);
  s={1,B_OK,uint32(active),waiter,waiters};return 0;}
 assert(op==PIONEER_ECAM_LEASE_PROBE);
 {std::unique_lock<std::mutex> lock(leaseLock);if(active && mode==0){waiter=current;++waiters;leaseCv.wait(lock,[]{return !active;});--waiters;}}
 auto& r=*static_cast<pioneer_ecam_lease_probe_result*>(data);
 r={1,B_OK,B_OK,1,B_OK,6,1,B_OK,B_OK,B_CANCELED,1,B_OK,B_NOT_ALLOWED,0};return 0;
}
int main(){
 mode=0;assert(contention_client_main()==0 && threads.empty() && sems.empty() && !active);
 mode=1;assert(contention_client_main()==1 && threads.empty() && sems.empty() && !active);
 std::puts("PASS actual contention client success and no-block false-positive rejection; SIMULATED ONLY");
}
'''.replace('SOURCE', str(repo / 'tools/pioneer/pioneer_ecam_lease_contention.cpp'))
    (tmp / 'test.cpp').write_text(harness)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-fsanitize=address,undefined', '-pthread', '-I', str(tmp),
                    str(tmp / 'test.cpp'), '-o', str(tmp / 'test')], check=True)
    subprocess.run([str(tmp / 'test')], check=True)
