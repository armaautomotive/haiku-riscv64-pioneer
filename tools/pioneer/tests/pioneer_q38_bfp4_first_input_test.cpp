/* SPDX-License-Identifier: MIT */
// CPU-only synthetic ownership/refusal tests for the isolated observer.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#endif
#define main PioneerCpuReferenceExistingMain
#include "pioneer_q38_cpu_model_reference_test.cpp"
#undef main
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
struct FfnCalls {unsigned count=0,failAt=0;};
static bool Diagnostic(void* context,unsigned layer,unsigned role,const std::vector<float>& input,std::vector<float>& output){
 auto& calls=*static_cast<FfnCalls*>(context);calls.count++;
 assert(layer==0 && role==calls.count+1);
 assert(input.size()==(role==4?17408U:5120U));
 if(calls.count==calls.failAt)return false;
 output.assign(role==4?5120U:17408U,0.f);return true;
}
static bool WrongShape(void*,unsigned,unsigned,const std::vector<float>&,std::vector<float>& output){
 output.assign(1,0.f);return true;
}
static bool NonFinite(void*,unsigned,unsigned,const std::vector<float>&,std::vector<float>& output){
 output.assign(17408,std::numeric_limits<float>::infinity());return true;
}
static bool SelectedSecondLayer(void* context,unsigned layer,unsigned role,const std::vector<float>&,std::vector<float>&){
 auto& calls=*static_cast<FfnCalls*>(context);calls.count++;assert(layer==1 && role==2);return false;
}
static bool SequenceZeros(void* context,unsigned layer,unsigned role,const std::vector<float>& input,std::vector<float>& output){
 auto& calls=*static_cast<FfnCalls*>(context);assert(layer==calls.count/3 && role==2+calls.count%3);
 assert(input.size()==(role==4?17408U:5120U));calls.count++;
 output.assign(role==4?5120U:17408U,0.f);return true;
}
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--sequence-zero")));
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  std::vector<uint32_t> input={123},final={456};assert(!observer.ObserveFirstFfnInput(17,input,final));
  assert(input==std::vector<uint32_t>{123} && final==std::vector<uint32_t>{456} && !observer.Ready() && !source.reads);}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  std::vector<uint32_t> output={123};assert(!observer.ObserveFirstFfnInput(248319,output,output));
  assert(output==std::vector<uint32_t>{123} && !observer.Ready() && !source.reads);}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  std::vector<uint32_t> input,final;assert(observer.ObserveFirstFfnInput(248319,input,final));
  assert(input.size()==5120 && final.size()==5120 && !observer.Ready());
  for(auto word:input)assert(!(word&0x7fffffff));for(auto word:final)assert(!(word&0x7fffffff));
  Q38CpuTokenTrace trace;assert(!observer.Step(248319,trace) && !observer.ObserveFirstFfnInput(248319,input,final));}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  Q38CpuFirstFfnTrace trace;assert(observer.ObserveFirstFfnTrace(248319,trace) && !observer.Ready());
  assert(trace.input.size()==5120 && trace.residual.size()==5120 && trace.gate.size()==17408 && trace.up.size()==17408
   && trace.activation.size()==17408 && trace.down.size()==5120 && trace.final.size()==5120);
  for(const auto* values:{&trace.input,&trace.residual,&trace.gate,&trace.up,&trace.activation,&trace.down,&trace.final})
   for(auto word:*values)assert(!(word&0x7fffffff));}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;
  Q38CpuFirstFfnTrace trace;assert(observer.ObserveFirstFfnWithDiagnostic(248319,Diagnostic,&calls,trace));
  assert(calls.count==3 && !observer.Ready() && trace.final.size()==5120);
  for(auto word:trace.final)assert(!(word&0x7fffffff));}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;calls.failAt=2;
  Q38CpuFirstFfnTrace trace;trace.final={0xabcdef};
  assert(!observer.ObserveFirstFfnWithDiagnostic(248319,Diagnostic,&calls,trace));
  assert(calls.count==2 && !observer.Ready() && trace.final==std::vector<uint32_t>{0xabcdef});}
 for(auto diagnostic:{WrongShape,NonFinite}){
  ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;
  Q38CpuFirstFfnTrace trace;trace.final={0xabcdef};
  assert(!observer.ObserveFirstFfnWithDiagnostic(248319,diagnostic,&calls,trace));
  assert(!observer.Ready() && trace.final==std::vector<uint32_t>{0xabcdef});
 }
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;calls.failAt=1;
  Q38CpuTokenTrace trace;trace.hidden={0xabcdef};
  assert(!observer.StepWithFfnDiagnostic(248319,Diagnostic,&calls,trace));
  assert(calls.count==1 && !observer.Ready() && trace.hidden==std::vector<uint32_t>{0xabcdef});}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;
  Q38CpuTokenTrace trace;trace.hidden={0xabcdef};
  assert(!observer.StepWithSelectedFfnDiagnostic(248319,uint64_t(1)<<1,SelectedSecondLayer,&calls,trace));
  assert(calls.count==1 && !observer.Ready() && trace.hidden==std::vector<uint32_t>{0xabcdef});}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;
  Q38CpuTokenTrace trace;assert(!observer.StepWithSelectedFfnDiagnostic(248319,0,Diagnostic,&calls,trace));
  assert(!calls.count && !observer.Ready() && !source.reads);}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  Q38CpuTokenTrace trace;assert(!observer.StepWithFfnDiagnostic(248319,nullptr,nullptr,trace));
  assert(!observer.Ready() && !source.reads);}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;
  Q38CpuTokenTrace trace;trace.hidden={0xabcdef};
  assert(!observer.StepWithSelectedFfnSequence(248319,0,Diagnostic,&calls,trace));
  assert(!observer.Ready() && !source.reads && trace.hidden==std::vector<uint32_t>{0xabcdef});}
 {ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));FfnCalls calls;calls.failAt=1;
  Q38CpuTokenTrace trace;trace.hidden={0xabcdef};
  assert(!observer.StepWithSelectedFfnSequence(248319,~uint64_t(0),Diagnostic,&calls,trace));
  assert(calls.count==1 && !observer.Ready() && trace.hidden==std::vector<uint32_t>{0xabcdef});
  assert(!observer.StepWithSelectedFfnSequence(17,~uint64_t(0),Diagnostic,&calls,trace));}
 if(argc==2){
  ReferenceRowsProbe source;Q38CpuModelReference observer;assert(observer.Bind(source));
  for(unsigned position=0;position<3;position++){
   FfnCalls calls;Q38CpuTokenTrace trace;
   const unsigned token=position==0?248319:position==1?17:0;
   assert(observer.StepWithSelectedFfnSequence(token,~uint64_t(0),SequenceZeros,&calls,trace));
   assert(calls.count==192 && trace.position==position && trace.input==token);
   assert(trace.hidden.size()==65*5120 && trace.logits.size()==248320 && trace.selected==0 && observer.Ready());
   for(auto b:trace.hidden)assert(!(b&0x7fffffff));for(auto b:trace.logits)assert(!(b&0x7fffffff));
   if(!position)for(unsigned fault=0;fault<3;fault++){
    const pid_t child=fork();assert(child>=0);
    if(!child){
     FfnCalls fresh;Q38CpuTokenTrace untouched;untouched.hidden={0xabcdef};
     bool okay=fault==0?observer.Step(17,untouched):fault==1?
      observer.StepWithSelectedFfnSequence(17,1,SequenceZeros,&fresh,untouched):
      observer.StepWithSelectedFfnSequence(17,~uint64_t(0),Diagnostic,&fresh,untouched);
     assert(!okay && !observer.Ready() && !fresh.count && untouched.hidden==std::vector<uint32_t>{0xabcdef});_exit(0);
    }
    int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0 && observer.Ready());
   }
  }
  {const pid_t child=fork();assert(child>=0);
   if(!child){FfnCalls calls;Q38CpuTokenTrace untouched;untouched.hidden={0xabcdef};
    assert(!observer.StepWithSelectedFfnSequence(0,~uint64_t(0),SequenceZeros,&calls,untouched));
    assert(!calls.count && !observer.Ready() && untouched.hidden==std::vector<uint32_t>{0xabcdef});_exit(0);}
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0 && observer.Ready());}
  Q38CpuTokenTrace trace;trace.hidden={0xabcdef};
  assert(!observer.Step(0,trace) && !observer.Ready() && trace.hidden==std::vector<uint32_t>{0xabcdef});
  puts("PASS three-position CPU diagnostic sequence with independent retained zero state and renewed contexts; NO MODEL OR DEVICE");
 }
 puts("PASS first-FFN-input CPU-only observer: synthetic zero, exact prompt gate, alias/refusal, one-shot isolation; NO MODEL QUALITY OR DEVICE CLAIM");
}
