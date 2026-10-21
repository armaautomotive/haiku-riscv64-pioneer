/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BENCHMARK_HOST
#error Separate fixed-public-FFN benchmark host ONLY
#endif
#if !defined(PIONEER_Q38_FFN_REPEAT_BENCH) || !defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_Q8_REPEAT_BENCH) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_ROOT_STATS_BENCH)
#error Benchmark host requires the exact original-TC FFN Work/no-CPU-reference role
#endif
#define main PioneerQ38UnusedStandaloneMain
#include "pioneer_q38_inference.cpp"
#undef main
#include "PioneerQ38BenchmarkHost.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
// Parent NEVER opens an accelerator FD or initializes threaded GGML. Each
// child inherits the SAME authenticated read-only source,then independently
// opens and explicitly ends ONE fresh accelerator session. Failed/lost
// child means STOP,never another fork,END/reset/retry by the parent.
int main(int argc,char** argv) {
 if(argc!=2 || strcmp(argv[1],"--public-ffn-command-host"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);setvbuf(stderr,nullptr,_IOLBF,0);umask(077);
 struct sigaction action={};action.sa_handler=RequestStop;sigemptyset(&action.sa_mask);
 if(sigaction(SIGINT,&action,nullptr) || sigaction(SIGTERM,&action,nullptr))return 1;
 char directory[]="/boot/home/pioneer-q38-benchmark.XXXXXX";
 if(!mkdtemp(directory))return 1;
 printf("BENCHMARK_HOST_SOURCE_AUTH_BEGIN directory%s;FULL_PUBLIC_SHA_REQUIRED_NO_ACCELERATOR_FD\n",directory);
 State state;PioneerQ38BenchmarkHostPolicy policy;
 if(!policy.Authenticated(state.source.Open() && state.source.Ready()))return 1;
 if(stopRequested)return 130;
 printf("BENCHMARK_HOST_IDLE directory%s round0 parent_accelerator_fd0;SAME_AUTHENTICATED_FD_FORK_BEFORE_GGML_THREADS_NO_CPU_REFERENCE\n",directory);
 char line[32];
 while(!stopRequested && fgets(line,sizeof(line),stdin)) {
  unsigned command=policy.Command(line);
  if(command==2)return policy.Close()?0:1;
  if(command!=1 || !policy.Waiting() || state.opened || state.session.Failed()
   || !state.source.PrepareBenchmarkFork() || !policy.Begin()) {
   puts("STOP_BENCHMARK_CONTROL_OR_SOURCE;NO_FORK_OR_CARD_IO");return 1;
  }
  char words[256],log[256];
  int a=snprintf(words,sizeof(words),"%s/round%04u.words",directory,policy.Round());
  int b=snprintf(log,sizeof(log),"%s/round%04u.log",directory,policy.Round());
  if(a<0 || b<0 || unsigned(a)>=sizeof(words) || unsigned(b)>=sizeof(log))return 1;
  int logFd=open(log,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
  if(logFd<0)return 1;
  fflush(stdout);fflush(stderr);
  pid_t child=fork();
  if(!child) {
   if(dup2(logFd,STDOUT_FILENO)<0 || dup2(logFd,STDERR_FILENO)<0)_exit(126);
   if(logFd>2)close(logFd);
   close(STDIN_FILENO); // child cannot consume another control command
   printf("BENCHMARK_CHILD round%u;SAME_AUTHENTICATED_FD_FRESH_SESSION_ORIGINAL_TC_WORK_NO_CPU_REFERENCE\n",policy.Round());
   int result=state.source.Ready()?GraphSmoke(state,words):1;
   // GraphSmoke success already required clean END. Finish is then an
   // idempotent HOST-state check;NEVER call it after a failed graph.
   if(!result && (!state.opened || state.session.Failed() || !state.session.Finish()))result=1;
   printf("BENCHMARK_CHILD_END round%u result%d known_clean%u\n",policy.Round(),result,unsigned(!result));
   if(fflush(stdout) || fflush(stderr))result=1;
   _exit(result?1:0);
  }
  close(logFd);
  if(child<0){policy.ChildFinished(false);puts("STOP_BENCHMARK_FORK_FAILED_NO_RETRY");return 1;}
  printf("BENCHMARK_CHILD_STARTED round%u pid%ld log%s output%s;NO_PARENT_ACCELERATOR_FD\n",policy.Round(),long(child),log,words);
  int status=0;pid_t done;
  do {
   done=waitpid(child,&status,0);
   if(done<0 && errno==EINTR && stopRequested)kill(child,SIGTERM);
  }while(done<0 && errno==EINTR);
  bool clean=done==child && WIFEXITED(status) && !WEXITSTATUS(status) && state.source.Ready();
  if(!policy.ChildFinished(clean)) {
   puts("STOP_BENCHMARK_CHILD_FAILED_OR_LOST;NO_FURTHER_FORK_END_RESET_RETRY_OR_CARD_IO");return 1;
  }
  if(stopRequested)return 130;
  printf("BENCHMARK_HOST_IDLE directory%s round%u parent_accelerator_fd0 child_known_clean1;AWAIT_EXPLICIT_MEASURE\n",directory,policy.Round());
 }
 return stopRequested?130:policy.Close()?0:1;
}
