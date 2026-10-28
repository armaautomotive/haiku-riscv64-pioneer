/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_original_fabric.h"
#include <csignal>
int main(int argc,char** argv){
 assert(argc==5 || (argc==6 && (!strcmp(argv[5],"--prepare-only") || !strcmp(argv[5],"--bootstrap-only"))));
 setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START authenticate entire SAME-open original27B GGUF BEFORE ROOT construction;no CPU product/native owner/physical IO");
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<OriginalTokenFlowMachine>(argv[1],argv[2],argv[3],original);
 m->VerifyRawEndpoints();
 const auto first=*m->learned.begin();unsigned route=unsigned(first.first>>32),address=unsigned(first.first);
 // A synthetic raw word is deliberately present only in each disposable
 // child. Correct reads must IGNORE it, and source refusal must ABORT rather
 // than fall back to it. The unchanged parent never gains such a byte.
 for(unsigned bad=0;bad<4;bad++){
  assert(original.PrepareFork());fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   uint32_t wanted=m->Get(route,address);m->dram[first.first]=wanted^1;assert(m->Get(route,address)==wanted);
   if(bad==0)m->Put(route,address,wanted);
   if(bad==1){original.Revoke();(void)m->Get(route,address);}
   std::array<uint8_t,64> output;output.fill(0x5a);
   if(bad==2){original.Revoke();m->ReadRaw(route,address,output.data(),64);}
   if(bad==3)m->ReadRaw(route,address+first.second->allocated-4,output.data(),8);
   _exit(73);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT && original.Ready() && original.PrepareFork());
 }
 puts("PASS 4 genuine original-fabric refusals/learned write+revoked word+revoked batch+cross-part batch/no synthetic fallback/unchanged virgin parent;TEST transport safety only");
 if(argc==6 && !strcmp(argv[5],"--prepare-only"))return 0;
 if(argc==6){
  unsigned entry=0;std::ifstream symbols(std::string(argv[1])+"/program.symbols");std::string addressText,kind,name;
  while(symbols>>addressText>>kind>>name)if(name=="q38_return_recurrent_layer")entry=unsigned(std::stoul(addressText,nullptr,16));
  assert(entry);
  while(m->pc!=entry && m->pc!=0x60000){assert(m->steps<200000000);m->StepToken();}
  assert(m->pc==entry && m->releases==1 && m->embedded==1 && m->embeddingReads==20 && m->embeddingWrites==2 && m->embeddingBack==2
   && !m->allLayers && !m->matrixJobs && !m->peerWrites && !m->committed && original.Ready());
  for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));
  printf("PASS actual original27B embedding5120/all metadata/first returning entry/ROOT from RESET+SAME119 still virgin/stack reserved%u written%u/ROOTinstructions%u;NOT first layer/full-model numerics/native/physical/Work/TPS\n",0x70000-m->minSp,0x70000-m->minStack,m->steps);return 0;
 }
 puts("START actual original27B FULL3-token flow/from RESET/SAME119/ALL learned bytes from SAME authenticated GGUF/NO supplied hidden or CPU model products");
 bool okay=m->RunTokens();if(m->kvFault){assert(!okay);m->VerifyKvFault();assert(original.Ready());_exit(0);}
 assert(okay && original.Ready());m->VerifyTokens();assert(original.Ready());
}
