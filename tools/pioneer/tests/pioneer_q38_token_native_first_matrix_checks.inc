/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE != 1
#error Explicit native first-matrix TEST required
#endif
static void CheckNativeFirstMatrix(Fixture& f){
 namespace R=PioneerQ38GlobalChainLayout::R;namespace W=PioneerQ38SelectedGlobalWorker;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && !state->Unsafe());cpu.ResumeFirstMatrix();unsigned observedBucket=cpu.steps/2000000;
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned round=0;round<3000000 && !cpu.layerHeld && cpu.pc!=0x60000 && cpu.pc!=0x60004;round++){
   io.Pause();if(cpu.steps/2000000!=observedBucket){observedBucket=cpu.steps/2000000;
    printf("OBSERVED native first-matrix ROOTsteps%u pc%x packets%u doors%u phase%x;NOT completed layer/token\n",
     cpu.steps,cpu.pc,cpu.layerPackets,cpu.firstMatrixDoorbells,io.Raw(0,R::Status+20));
   }
  }
  if(!cpu.layerHeld)fprintf(stderr,"FIRST_MATRIX_DIAGNOSTIC ROOTpc%x steps%u doors%u phase%x;NO result supplied\n",
   cpu.pc,cpu.steps,cpu.firstMatrixDoorbells,io.Raw(0,R::Status+20));
  assert(cpu.layerHeld && cpu.firstMatrixDoorbells==3 && cpu.dispatchWorker==1 && io.Core(0).pending
   && cpu.pc!=0x60000 && cpu.pc!=0x60004 && !state->Unsafe() && !state->Closed()
   && io.Raw(1,W::Job+7*4)==1002 && io.Raw(1,W::Doorbell)==1);
  PioneerQ38TokenExecutionResult observed;assert(ObserveQ38TokenNativeRoot(c,observed) && observed.acknowledged && !observed.unsafe);
 }
 unsigned instructions=0,matrices=0,minPeerSp=0x70000,minPeerAccessed=0x70000;
 for(unsigned w=1;w<120;w++){
  auto& p=*io.bootstrapPeer[w-1];const auto s=p.cpu->Stats();assert(p.Idle() && !s.pending && !s.active);
  const auto stack=p.cpu->Stack();assert(stack.instructions==s.totalSteps && stack.minimumSp>=0x6e000);
  minPeerSp=std::min(minPeerSp,stack.minimumSp);minPeerAccessed=std::min(minPeerAccessed,stack.minimumAccessed);
  assert(io.Raw(w,W::Status+4)==(w<=3?1U:0U) && !io.Raw(w,W::Status+8));
  if(w<=3){
   const bool output=p.cpu->ObserveMatrixOutput();
   printf("OBSERVED native first-matrix worker%u pc%x sequence%u matrixInstructions%u scalarFP%u requests%u responses%u independentOutput%u;NOT complete layer/token\n",
    w,s.pc,s.sequence,s.matrixInstructions,s.floatOps,s.requests,s.responses,unsigned(output));
   // The fast matrix may perform its floating arithmetic in Tensix/SFPU,
   // not scalar RISC-FP. Require actual matrix instructions and the full
   // independent output comparison,never an incidental scalar-FP counter.
   assert(s.sequence==1 && s.matrixInstructions && output);matrices+=s.matrixInstructions;
  }
  else assert(!s.sequence && !s.matrixInstructions && !s.floatOps && !s.requests && !s.responses);
  instructions+=s.steps;
 }
 assert(cpu.instructionStack.Valid() && cpu.instructionStack.Initialized()
  && cpu.instructionStack.Instructions()==cpu.steps);
 printf("OBSERVED actual first-matrix accelerator ROOT reserved stack%u/accessed%u;MAX119peer reserved%u/accessed%u;8192 bytes EACH/separate HOST kernel stack unqualified\n",
  0x70000-cpu.instructionStack.MinimumSp(),0x70000-cpu.minStack,0x70000-minPeerSp,0x70000-minPeerAccessed);
 printf("PASS SAME native full851 owner/ALL119 full tensor ISA engines VIRGIN from construction/shared actual L1+CSR+ROOT inputs+authenticated original learned bytes/ACTUAL first3 workers complete role10 fullK5120+48 rows/real RISC+Tensix matrices%u+native output bytes+FP32/FP64 OBSERVERS ONLY/no CPU product/history transplant/ROOT validates first projection before NEXT matrix doorbell held/workerSteps%u ROOTsteps%u;NOT whole layer/token/STOP/END/physical/Work/TPS\n",
  matrices,instructions,cpu.steps);
}
