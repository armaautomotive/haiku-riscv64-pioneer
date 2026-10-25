/* SPDX-License-Identifier: MIT */
// Genuine extracted source/zero/embedding/RMS and ACTUAL installed QPS1+
// ALL119 peers. Byte endpoints/clock/learned weights simulated,NOT native.
#define PIONEER_Q38_PROJECTION_EXECUTION_FIXTURE_ONLY
#include "pioneer_q38_projection_execution_test.cpp"
#include <type_traits>
static_assert(!std::is_default_constructible<PioneerQ38ResidentProjectionInput>::value
 && !std::is_copy_constructible<PioneerQ38ResidentProjectionInput>::value
 && !std::is_move_constructible<PioneerQ38ResidentProjectionInput>::value
 && !std::is_destructible<PioneerQ38ResidentProjectionInput>::value,"borrowed private capability cannot be constructed/copied/moved/destroyed by a caller");
struct ClosedInputIO:ExecutionIO {
 uint32_t RuntimePcRead(){assert(C().coreReset==0x46800 && "NEVER read a held program counter");return ExecutionIO::RuntimePcRead();}
};
int main(int argc,char** argv){
 assert(argc==7);setvbuf(stdout,nullptr,_IOLBF,0);const unsigned mode=unsigned(atoi(argv[6]));assert(mode<=3);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionExecutionProducer producer(argv[3],argv[4],argv[5]);auto& model=producer.Model();assert(producer.Valid() && !model.position);
 ClosedInputIO io;io.producer=&producer;
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38ProjectionGraphMemory>();
 auto owner=std::make_unique<PioneerQ38ProjectionGraphOwner>();PioneerQ38ProjectionGraphResult result;
 std::vector<std::unique_ptr<PanelOwners>> participants;
 for(unsigned w=0;w<120;w++){
  participants.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&participants[w]->store;
  n.noc=participants[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);
  assert(PioneerQ38DispatchPrepareRole(io,producer.Proof(),producer.Banks(),*cohort,w));
 }
 uint32_t source[16],command[10],counts[8];producer.RootControls(source,command,counts);
 for(unsigned i=0;i<16;i++)assert(source[i]==cohort->authority.source[0][i]);
 io.SetControls(0,command,counts);auto originals=io.data;
 io.expectedOwner=owner.get();io.expectedMemory=memory.get();io.result=&result;io.memoryState=&result.memory;io.track=true;
 io.machine.reset(new ProjectionExecutionMachine(argv[1],argv[2],io));
 unsigned touches=io.touches;assert(!owner->ResidentInput() && io.touches==touches);
 assert(owner->Prepare(io,producer.Proof(),producer.Banks(),*cohort,*memory,model.graph,model.plan,model.rms,model.identity,
  producer.Ledger(),PioneerResidentBanks::Pages,model.token,model.position,model.bank,10,result));
 const unsigned roles[]={10,11,5,6};
 for(unsigned i=0;i<(mode==1?2U:4U);i++){
  assert(owner->Run(io,roles[i],result));io.machine->VerifyOutput();
  touches=io.touches;assert(!owner->ResidentInput() && io.touches==touches); // NEVER publish before clean STOP/restore
 }
 if(mode==1)result.runs=4; // forged PUBLIC count cannot replace the missing PRIVATE two role receipts
 if(mode==2)io.restoreFault=1;
 if(mode==3)io.receiptFault=56; // corrupt the ACTUAL final STOP receipt,not a manufactured completion
 const auto actualDeviceBytes=producer.DeviceWords();
 const bool closed=owner->Close(io,result);
 if(mode>=2){
  assert(!closed && owner->Unsafe() && memory->Unsafe() && cohort->unsafe && producer.Banks().Quarantined() && io.retainedExecution);
  touches=io.touches;const auto reads=io.reads,writes=io.writes;
  assert(!owner->ResidentInput() && !owner->Close(io,result) && !owner->Run(io,10,result)
   && io.touches==touches && io.reads==reads && io.writes==writes);
  printf("PASS resident projection input fault%u:failed actual STOP/restore publishes NOTHING,ALL source/hosts/snapshots retained,ZERO following IO/reset/PC/restore/retry;NOT native\n",mode);return 0;
 }
 assert(closed && owner->Closed() && memory->Closed() && !owner->Ready() && io.data==originals
  && result.stopped==119 && result.held==119 && result.memory.restoredWords==8134072 && result.memory.verifiedOriginalWords==16268144
  && producer.DeviceWords()==actualDeviceBytes); // GPU stashes survive complete L1+command restoration
 touches=io.touches;const auto* input=owner->ResidentInput();assert(io.touches==touches);
 if(mode==1){assert(!input);puts("PASS partial TWO real projections + forged public r.runs4 cannot mint closed resident input;actual full STOP/holds/restore still completes;NO native grant");return 0;}
 assert(input && input->Current() && io.touches==touches && io.boots==119 && io.starts==5
  && io.machine->Matrices()==244 && io.machine->Producers()==80 && io.machine->Stops()==119);
 unsigned refused=0;
 for(unsigned w:{0U,119U})for(unsigned i=0;i<16;i++){
  cohort->authority.source[w][i]^=1;assert(!input->Current() && !owner->ResidentInput() && io.touches==touches);
  cohort->authority.source[w][i]^=1;assert(input->Current());refused++;
 }
 for(unsigned w:{0U,119U}){
  auto& n=cohort->worker[w];auto deny=[&](auto& field,bool bad){const auto old=field;field=bad;
   assert(!input->Current() && !owner->ResidentInput() && io.touches==touches);field=old;assert(input->Current());refused++;};
  deny(n.image.saved,false);deny(n.image.ready,false);deny(n.image.unsafe,true);deny(n.image.running,true);
  deny(n.Store().ready,false);deny(n.Store().pending,true);deny(n.Store().recovery_required,true);deny(n.noc.recovery_required,true);
 }
 auto* ledger=const_cast<uint8_t*>(producer.Ledger());
 for(unsigned role:roles){const auto* part=model.plan.ProjectionStripe(0,role,0);assert(part);
  const unsigned page=part->bank*unsigned(model.plan.Capacity/4096)+part->offset/4096;const auto old=ledger[page];assert(old==1);
  ledger[page]=0;assert(!input->Current() && !owner->ResidentInput() && io.touches==touches);ledger[page]=old;assert(input->Current());refused++;
 }
 owner->Fault();assert(!input->Current() && !owner->ResidentInput() && io.touches==touches);
 printf("PASS actual zero+QPE1+QRN1+ALL4 QPS1 projections/ALL119 peers->STOP+unheld terminals+ALL120 holds+8134072-word restore->PRIVATE closed GPU input;metadataRefusals%u/NO held PC reads/no CPU products or ready setters. Historical lineage ONLY,NOT native/init/current-slot/IO/token-rate authority\n",refused);
}
