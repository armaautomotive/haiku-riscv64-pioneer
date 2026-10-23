/* SPDX-License-Identifier: MIT */
// Actual native source/zero/QPE1/QRN1 + QPS1 -> QCB1 owner/instructions.
// Endpoints+learned weights simulated;NOT actual new native factory/card.
#define PIONEER_Q38_PROJECTION_EXECUTION_FIXTURE_ONLY
#include "pioneer_q38_projection_execution_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentDecoderOwner.h"
#include "pioneer_q38_decoder_conv_bridge.h"
#include <sys/wait.h>
#include <unistd.h>
struct DecoderIO:ExecutionIO {
 const char* convImage=nullptr;PioneerQ38ResidentDecoderOwner* expectedDecoder=nullptr;PioneerQ38ConvHistoryBanks* expectedHistory=nullptr;
 bool conv[120]={};unsigned pc[120]={},convStarts=0;
 PioneerQ38ResidentDecoderResult* decoderResult=nullptr;unsigned fault=0;bool injected=false;
 bool SelectRuntimeCore(unsigned w){
  bool okay=ExecutionIO::SelectRuntimeCore(w);
  if(okay && !injected && decoderResult && decoderResult->phase==2 && decoderResult->installed==120 && fault>=3 && fault<=10 && w==119){
   injected=true;
   if(fault==3)FullWrite(w,TT_Q38_CONV_BATCH_TABLE+40*4,FullRead(w,TT_Q38_CONV_BATCH_TABLE+40*4)^1);
   if(fault==4)FullWrite(w,0x64000+6084,FullRead(w,0x64000+6084)^1);
   if(fault==5)C().source[11]^=1;if(fault==6)C().counters[0]++;if(fault==7)C().command[9]^=1;
   if(fault==8)C().coreReset=0x46800;if(fault==9)C().mapping=false;if(fault==10)C().pciCommand[0]=0x406;
  }
  if(okay && !injected && decoderResult && decoderResult->phase==4 && fault>=11 && fault<=22 && w==119){
   injected=true;
   if(fault==11)FullWrite(w,TT_Q38_FFN_RESIDENT_SUMMARY,FullRead(w,TT_Q38_FFN_RESIDENT_SUMMARY)^1);
   if(fault==12)FullWrite(w,TT_Q38_CONV_BATCH_TABLE+383*4,FullRead(w,TT_Q38_CONV_BATCH_TABLE+383*4)^1);
   if(fault==13)C().command[9]^=1;if(fault==14)C().counters[0]++;if(fault==15)C().source[11]^=1;
   if(fault==16)C().coreReset=0x47800;if(fault==17)C().mapping=false;if(fault==18)pc[w]=0x64058;
   if(fault==19)FullWrite(w,0x17ffc8,0xdeadbeef);if(fault==20)FullWrite(w,0x17ffc4,0);
   if(fault==21)FullWrite(w,0x17ffc0,0);if(fault==22)FullWrite(w,0x64000,FullRead(w,0x64000)^1);
  }
  return okay;
 }
 bool ProjectionGraphMemoryWrite(unsigned w,uint32_t a,uint32_t v){
  if(!injected && decoderResult && ((fault==2 && decoderResult->phase==2 && w==119 && a==0x6000)
   || (fault==23 && decoderResult->phase==7 && w==119 && a==PioneerQ38ProjectionGraphSnapshotAddress(w,PioneerQ38ProjectionGraphSnapshotWords(w)-1)))){
   Touch();injected=true;return false;
  }
  return ExecutionIO::ProjectionGraphMemoryWrite(w,a,v);
 }
 bool ProjectionGraphMemoryReadChecked(unsigned w,uint32_t a,uint32_t& v){
  if(!injected && decoderResult && fault==24 && decoderResult->phase==8 && w==119
   && a==PioneerQ38ProjectionGraphSnapshotAddress(w,PioneerQ38ProjectionGraphSnapshotWords(w)-1)){Touch();injected=true;return false;}
  return ExecutionIO::ProjectionGraphMemoryReadChecked(w,a,v);
 }
 bool ResidentDecoderScope(const PioneerQ38ResidentDecoderOwner& owner,const PioneerQ38ProjectionGraphOwner& p,const PioneerQ38ConvHistoryBanks& h){
  if(fault==1){injected=true;return false;}
  return scope && producer && producer->Valid() && &owner==expectedDecoder && &p==expectedOwner && &h==expectedHistory
   && owner.MatchesNativeOwner(p,h);
 }
 void RuntimeResetWrite(uint32_t value){
  if(value==0x46800 && FullRead(current,0x6000)==PioneerQ38ConvBatchImageWord(0)){
   Touch();assert(!conv[current]);C().coreReset=value;conv[current]=true;convStarts++;
   pc[current]=ExecuteDecoderConv(convImage,producer->Model().plan,current,producer->Model().bank,*this,*producer);return;
  }
  ExecutionIO::RuntimeResetWrite(value);
 }
 uint32_t RuntimeStatusRead(unsigned i){if(!conv[current])return ExecutionIO::RuntimeStatusRead(i);Touch();return FullRead(current,0x17ffc0+i*4);}
 uint32_t RuntimePcRead(){if(!conv[current])return ExecutionIO::RuntimePcRead();Touch();return pc[current];}
};
int main(int argc,char** argv){
 assert(argc==7 || (argc==8 && !strcmp(argv[7],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionExecutionProducer producer(argv[3],argv[4],argv[5]);auto& model=producer.Model();assert(producer.Valid() && !model.position);
 producer.InitializeHistory();puts("DECODER_GENUINE_PRODUCER_HISTORY_ZERO_ALL1680");
 DecoderIO io;io.producer=&producer;io.convImage=argv[6];
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38ProjectionGraphMemory>();
 auto projection=std::make_unique<PioneerQ38ProjectionGraphOwner>();auto decoder=std::make_unique<PioneerQ38ResidentDecoderOwner>();
 PioneerQ38ProjectionGraphResult result;PioneerQ38ResidentDecoderResult execution;
 std::vector<std::unique_ptr<PanelOwners>> participants;
 for(unsigned w=0;w<120;w++){participants.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&participants[w]->store;
  n.noc=participants[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,producer.Proof(),producer.Banks(),*cohort,w));}
 uint32_t source[16],command[10],counts[8];producer.RootControls(source,command,counts);io.SetControls(0,command,counts);
 auto original=io.data;io.expectedOwner=projection.get();io.expectedMemory=memory.get();io.result=&result;io.memoryState=&result.memory;io.track=true;
 io.expectedDecoder=decoder.get();io.expectedHistory=model.graph.ConvHistory(model.identity);
 io.decoderResult=&execution;
 io.machine.reset(new ProjectionExecutionMachine(argv[1],argv[2],io));
 assert(projection->Prepare(io,producer.Proof(),producer.Banks(),*cohort,*memory,model.graph,model.plan,model.rms,model.identity,
  producer.Ledger(),PioneerResidentBanks::Pages,model.token,0,model.bank,10,result));
 unsigned before=io.touches;assert(!decoder->RunOnce(io,*projection,*io.expectedHistory,execution) && !decoder->Claimed() && io.touches==before);
 for(unsigned role:{10U,11U,5U,6U})assert(projection->Run(io,role,result));
 auto workspace=producer.DeviceWords();puts("DECODER_ACTUAL_QPS1_PRODUCER_ALL4ROLES");
 if(argc==8){
  // Fork ONLY simulated endpoint state after its genuine producer ran.
  // Every isolated child performs the actual STOP/replacement/arithmetic
  // controller,not a reconstructed completion/private-ready fixture.
  for(unsigned fault=1;fault<=24;fault++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    io.fault=fault;bool ran=decoder->RunOnce(io,*projection,*io.expectedHistory,execution);
    if(fault>=23){assert(ran);assert(!decoder->Close(io,execution));}else assert(!ran);
    assert(io.injected && decoder->Unsafe() && projection->Unsafe() && memory->Unsafe() && !decoder->Closed() && !projection->Closed()
     && !memory->Closed() && model.graph.Unsafe() && producer.Banks().Quarantined() && cohort->unsafe && io.retainedExecution && !producer.Valid());
    if(fault<=10)assert(!io.convStarts);if(fault>=11 && fault<=22)assert(io.convStarts==120 && !execution.held);
    unsigned touched=io.touches,reads=io.reads,writes=io.writes;
    assert(!decoder->RunOnce(io,*projection,*io.expectedHistory,execution) && !decoder->Close(io,execution)
     && !projection->Run(io,10,result) && !projection->Close(io,result) && !memory->CloseHeldNoLaunch(io,result.memory)
     && io.touches==touched && io.reads==reads && io.writes==writes);
    printf("PASS decoder fault%u private consumption/ALL ownership retained/ZERO later IO\n",fault);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS24 actual retained arithmetic handoff/code/table/source/counter/command/reset/map/PCI/status/PC/restore faults;first failure retainsALL/no later IO;SIMULATED HARDWARE ONLY");return 0;
 }
 bool okay=decoder->RunOnce(io,*projection,*io.expectedHistory,execution);
 if(!okay)fprintf(stderr,"DECODER_FAIL phase%u worker%u word%u stop%u/%u convStarts%u\n",execution.phase,execution.worker,execution.word,execution.stop.phase,execution.stop.word,io.convStarts);
 assert(okay && !decoder->Unsafe() && !decoder->Closed() && decoder->Claimed() && !projection->Ready() && !projection->Closed()
  && execution.installed==120 && execution.armed==120 && execution.released==120 && execution.completed==120 && execution.held==120
  && execution.stop.stopped==119 && !result.memory.restoredWords && !memory->Closed() && io.convStarts==120 && io.machine->Stops()==119);
 // Independent numerical observer ONLY AFTER actual device chain. No CPU
 // vector/result was supplied to either production execution controller.
 for(unsigned channel=0;channel<10240;channel++){
  uint32_t input=workspace[PioneerQ38ProjectionResultOffset(6)/4+channel];float x;memcpy(&x,&input,4);
  float weight=float(int((channel*7+3*11)%29)-14)*.03125f;float sum=x*weight;double expected=double(sum)/(1+exp(-double(sum)));
  uint32_t actualBits=producer.DeviceWords()[TT_Q38_CONV_OUTPUT/4+channel];float actual;memcpy(&actual,&actualBits,4);
  assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected)));
  assert(!producer.DeviceWords()[TT_Q38_CONV_NEXT/4+channel*3] && !producer.DeviceWords()[TT_Q38_CONV_NEXT/4+channel*3+1]
   && producer.DeviceWords()[TT_Q38_CONV_NEXT/4+channel*3+2]==input);
 }
 for(unsigned i=0;i<workspace.size();i++)if(!(i>=TT_Q38_CONV_OUTPUT/4 && i<(TT_Q38_CONV_OUTPUT+40960)/4)
  && !(i>=TT_Q38_CONV_NEXT/4 && i<(TT_Q38_CONV_NEXT+122880)/4))assert(producer.DeviceWords()[i]==workspace[i]);
 before=io.touches;assert(!projection->Run(io,10,result) && !projection->Close(io,result) && !memory->CloseHeldNoLaunch(io,result.memory)
  && !decoder->RunOnce(io,*projection,*io.expectedHistory,execution) && io.touches==before);
 // A fresh caller result cannot bypass actual current held-code restore.
 assert(decoder->Close(io,execution) && decoder->Closed() && projection->Closed() && memory->Closed() && io.data==original
  && execution.memory.restoredWords==8134072 && execution.memory.verifiedOriginalWords==8134072);
 before=io.touches;assert(!decoder->Close(io,execution) && !projection->Run(io,10,result) && !projection->Close(io,result) && io.touches==before);
 puts("PASS actual retained QPS1->QCB1 ALL120/640card-local jobs/10240convolution outputs+30720private next words/private proof consumed/old entry points inert/original8134072word snapshot retained THROUGH arithmetic and restored at final CLOSE;NO_NATIVE_FACTORY_CARD_FULL_MODEL_OR_SPEED_CLAIM");
}
