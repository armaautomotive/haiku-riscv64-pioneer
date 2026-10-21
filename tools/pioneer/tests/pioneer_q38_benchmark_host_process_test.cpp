/* SPDX-License-Identifier: MIT */
// ACTUAL production main and same-FILE fork barrier,with CPU-only fake
// authenticated source/session/graph. NO GGML,accelerator,real model/hash.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_Q38_BENCHMARK_HOST
#include "../PioneerQ38BenchmarkHost.h"
static volatile sig_atomic_t stopRequested=0;
static void RequestStop(int){stopRequested=1;}
static bool Scenario(const char* s){return getenv("TEST_SCENARIO") && !strcmp(s,getenv("TEST_SCENARIO"));}
struct Source {
 FILE* file=nullptr;bool borrowed=false,valid=false;struct stat identity={};
 ~Source(){if(file)fclose(file);}
 bool Open(){assert(!file);file=fopen(getenv("TEST_SOURCE"),"rb");valid=file && !fstat(fileno(file),&identity);return valid && !Scenario("authentication");}
 bool Ready(){struct stat now={};return valid && file && !fstat(fileno(file),&now) && now.st_ino==identity.st_ino && now.st_size==identity.st_size;}
#include "benchmark_fork_barrier.inc"
};
struct Session {
 bool failed=false;
 bool Failed(){return failed;}
 bool Finish(){return !Scenario("finish-failed");}
};
struct State {Source source;Session session;bool opened=false;};
static int GraphSmoke(State& state,const char* output) {
 static unsigned count=0;assert(++count==1 && !state.opened); // fresh COW globals/session EVERY child
 if(Scenario("killed"))raise(SIGKILL);
 if(Scenario("signal")){while(!stopRequested)pause();return 1;}
 char data[4096];assert(!fseeko(state.source.file,7000,SEEK_SET));
 assert(fread(data,1,sizeof(data),state.source.file)==sizeof(data));
 for(unsigned i=0;i<sizeof(data);i++)assert(uint8_t(data[i])==uint8_t((i+7000)*29+17));
 // Leave a different buffered/underlying offset for the next parent barrier.
 assert(!fseeko(state.source.file,123,SEEK_SET) && fread(data,1,200,state.source.file)==200);
 int fd=open(output,O_WRONLY|O_CREAT|O_EXCL,0600);assert(fd>=0);
 assert(write(fd,"same-source-fresh-session\n",26)==26);assert(!close(fd));
 if(Scenario("source-changed"))assert(!truncate(getenv("TEST_SOURCE"),100));
 state.opened=!Scenario("not-opened");
 state.session.failed=Scenario("session-failed");
 return Scenario("graph-failed")?1:0;
}
#include "benchmark_host_main.inc"
