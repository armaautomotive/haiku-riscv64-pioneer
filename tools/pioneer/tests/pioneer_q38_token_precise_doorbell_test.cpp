/* SPDX-License-Identifier: MIT */
// Actual HOST token transport observer grammar ONLY,not a model/worker run.
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY 1
#include "pioneer_q38_token_flow_test.cpp"
#include <csignal>
static std::array<uint32_t,64> Job(unsigned w,unsigned layer,unsigned position,unsigned token){
 std::array<uint32_t,64> job={};job[0]=G::Magic;job[2]=G::ActivationOperation;
 job[3]=w+1;job[4]=G::Peer(w+1);job[5]=layer;job[6]=position;job[27]=token;return job;
}
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 unsigned count=0;
 for(unsigned p=0;p<3;p++)for(unsigned layer=0;layer<64;layer++)for(unsigned w=0;w<68;w++){
  const bool dense=CL::Dense(layer);const unsigned token=p?17:248319;const auto job=Job(w,layer,p,token);
  TokenFlowMachine::ObservePreciseDoorbell(w,dense?6:7,dense,layer,p,token,layer,job.data(),count);
 }
 assert(count==3*64*68);
 for(unsigned bad=0;bad<18;bad++){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   unsigned worker=0,matrices=7,layer=0,position=0,token=248319,completed=0,observed=0;
   bool dense=false;auto job=Job(worker,layer,position,token);
   if(bad==0)worker=68;if(bad==1)matrices=6;if(bad==2)dense=true;if(bad==3)layer=64;
   if(bad==4)position=3;if(bad==5)token=248320;if(bad==6)completed=1;
   if(bad==7)job[0]^=1;if(bad==8)job[2]=2;if(bad==9)job[3]++;
   if(bad==10)job[4]^=1;if(bad==11)job[5]++;if(bad==12)job[6]++;if(bad==13)job[27]++;
   if(bad==14)observed=3*64*68;if(bad==16)matrices=8;if(bad==17)job[2]=4;
   TokenFlowMachine::ObservePreciseDoorbell(worker,matrices,dense,layer,position,token,completed,bad==15?nullptr:job.data(),observed);
   _exit(73);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
 }
 printf("PASS ACTUAL token transport QGW2 activation observer/%u bounded job-metadata cases+18 invalid-worker/phase/owner/layer/token/op/count refusals;DATA grammar ONLY,no ROOT/worker/model execution,completion or physical/TPS grant\n",count);
}
