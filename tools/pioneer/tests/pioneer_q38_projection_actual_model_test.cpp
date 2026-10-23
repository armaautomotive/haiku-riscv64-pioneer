/* SPDX-License-Identifier: MIT */
// DEBUG OFFLINE ONLY. Full SAME-FILE public GGUF SHA before constructing
// even the simulated native device. Original bytes feed actual installed
// zero/QPE1/QRN1/QPR1/QFP3 instruction machines;CPU references only OBSERVE.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_ONLY
#include "pioneer_q38_projection_execution_native_factory_test.cpp"
#include "pioneer_q38_projection_execution_machine.h"
#include "../PioneerQwen38PackedPlan.h"
#include <unistd.h>
static const char* actualModelPath=nullptr;
static FILE* ActualModelFopen(const char* path,const char* mode){
 assert(actualModelPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
 return fopen(actualModelPath,mode);
}
static int ActualModelExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
// Platform syscalls ONLY are adapted. Actual production full-SHA/same-FILE
// source authentication/parser/epoch/read methods execute unchanged.
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#define fopen ActualModelFopen
#define execv ActualModelExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38EmbeddingValidationReference.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
#include "../PioneerQ38CpuValidation.h"
class ActualModelWeights:public ProjectionExecutionLearnedBytes {
 struct Stripe {unsigned route;uint32_t address,bytes;std::vector<uint8_t> data;};
 PioneerQ38PublicModelSource& source;PioneerQwen38PackedPlan plan;
 std::vector<Stripe> stripes;bool prepared=false;unsigned normTensor=UINT32_MAX,embeddingTensor=UINT32_MAX;
public:
 std::array<uint8_t,46080> embedding;
 std::array<uint32_t,5120> norm,rawReference,normalizedReference;
 uint64_t loadedBytes=0,tileReads=0;
 explicit ActualModelWeights(PioneerQ38PublicModelSource& original):source(original){}
 bool Ready()const override{return prepared && source.Ready() && plan.MatchesSource(source.Model());}
 bool Prepare(){
  if(prepared || !source.Ready() || !plan.Build(source.Model(),8192,true,true))return false;
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!canonical->Build() || canonical->Count()!=plan.Parts().size())return false;
  const auto& parts=plan.Parts();unsigned projections=0;
  for(unsigned i=0;i<parts.size();i++){const auto& p=parts[i];const auto& c=*canonical->Part(i);
   if(c.kind!=p.kind || c.layer!=p.layer || c.role!=p.role || c.bank!=p.bank || c.firstRow!=p.firstRow || c.rows!=p.rows
    || c.rowBytes!=p.rowBytes || c.offset+canonical->Base!=p.proposedAddress || c.bytes!=p.bytes || c.allocated!=p.allocated)return false;
   if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   if(p.layer==64 && !p.role)embeddingTensor=p.tensor;
   if(p.layer || (p.role!=0 && p.role!=5 && p.role!=6 && p.role!=10 && p.role!=11))continue;
   Stripe s={PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,p.bytes,std::vector<uint8_t>(p.allocated)};
   for(unsigned page=0;page<p.allocated/4096;page++)if(!source.ReadWeightPage(plan,i,page,s.data.data()+page*4096,4096))return false;
   loadedBytes+=p.allocated;
   if(!p.role){
    if(p.bytes!=20480 || p.rows!=1 || p.allocated!=20480)return false;normTensor=p.tensor;
    for(unsigned j=0;j<5120;j++){const uint8_t* q=s.data.data()+j*4;norm[j]=uint32_t(q[0])|(uint32_t(q[1])<<8)|(uint32_t(q[2])<<16)|(uint32_t(q[3])<<24);}
   }else{if(c.type!=8 || p.rowBytes!=5760)return false;stripes.push_back(std::move(s));projections++;}
  }
  prepared=projections==16 && normTensor!=UINT32_MAX && embeddingTensor!=UINT32_MAX && source.Ready();return Ready();
 }
 bool Token(unsigned token){
  if(!Ready() || token>=248320)return false;
  unsigned found=0;
  const auto& parts=plan.Parts();
  for(unsigned i=0;i<parts.size();i++){const auto& p=parts[i];
   if(p.kind!=PIONEER_Q38_WEIGHT || p.layer!=64 || p.role || token<p.firstRow || token-p.firstRow>=p.rows)continue;
   unsigned offset=(token-p.firstRow)/16*46080,first=offset/4096,within=offset%4096;
   std::vector<uint8_t> pages(((within+46080+4095)/4096)*4096);
   for(unsigned page=0;page<pages.size()/4096;page++)if(!source.ReadWeightPage(plan,i,first+page,pages.data()+page*4096,4096))return false;
   memcpy(embedding.data(),pages.data()+within,46080);found++;
  }
  return found==1 && PioneerQ38EmbeddingReference(source,embeddingTensor,token,rawReference.data())
   && PioneerQ38LearnedRmsReference(source,normTensor,rawReference.data(),normalizedReference.data()) && Ready();
 }
 bool Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes)override{
  if(!Ready() || !out || bytes!=576 || (address&63))return false;
  for(const auto& s:stripes)if(s.route==route && address>=s.address && address-s.address<s.bytes){
   unsigned relative=address-s.address;
   if(relative%576 || bytes>s.bytes-relative)return false;
   memcpy(out,s.data.data()+relative,bytes);tileReads++;return Ready();
  }
  return false; // NEVER synthesize/clip/requantize or substitute products
 }
 bool Validate(unsigned role,const uint32_t* actual){
  if(!Ready() || (role!=5 && role!=6 && role!=10 && role!=11))return false;
  PioneerQ38Tensor expected;if(!PioneerQ38Expected(0,role,expected))return false;
  const auto* t=source.Model().Find(expected.name.c_str());if(!t)return false;
  std::array<float,5120> originalInput;for(unsigned i=0;i<5120;i++)originalInput[i]=PioneerAttentionFloat(normalizedReference[i]);
  std::vector<float> result(unsigned(t->outputs));for(unsigned i=0;i<result.size();i++)result[i]=PioneerAttentionFloat(actual[i]);
  return PioneerQ38CpuValidate(source,unsigned(t-source.Model().Tensors().data()),originalInput.data(),5120,result.data(),unsigned(result.size()));
 }
};
int main(int argc,char** argv){
 assert(argc==7 || (argc==8 && !strcmp(argv[7],"--all-tokens")));setvbuf(stdout,nullptr,_IOLBF,0);actualModelPath=argv[6];
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("DEBUG_OFFLINE_ACTUAL_PUBLIC_MODEL_FULL_SHA_BEFORE_ANY_NATIVE_CONTEXT;NO_DEVICE_ACCESS_OR_TOKEN_RATE");
 PioneerQ38PublicModelSource original;assert(original.Open() && original.Ready());
 puts("FULL_SAME_FILE_c600de03_SHA_AND_ALL851_TENSORS_PASS");
 ActualModelWeights weights(original);assert(weights.Prepare());
 printf("ORIGINAL_CANONICAL_WEIGHT_BYTES%llu all16projection_stripes_plus_layer0_norm;NO_WEIGHT_EXPANSION\n",(unsigned long long)weights.loadedBytes);
 unsigned tokens=0;
 for(unsigned token:argc==8?std::vector<unsigned>{0,15,248319}:std::vector<unsigned>{0}){
  assert(weights.Token(token)); // independent references BEFORE native context
  ProjectionNativeFixture f(argv[3],argv[4],argv[5],token,token?6:0);
  f.io.originalEmbeddingWeights=weights.embedding.data();f.io.originalRmsWeights=weights.norm.data();
  printf("ACTUAL_MODEL_TOKEN%u_GENUINE_NATIVE_PRODUCE\n",token);f.Produce();
  for(unsigned i=0;i<5120;i++)assert(f.io.wholeWorkspace[i]==weights.normalizedReference[i]
   && f.io.wholeWorkspace[TT_Q38_RMS_BACKUP/4+i]==weights.rawReference[i]);
  puts("ACTUAL_MODEL_QPE1_QRN1_ALL5120_BITS_AND_RAW_RESIDUAL_MATCH_INDEPENDENT_ORIGINAL_GGUF");
  f.Attach(argv[1],argv[2]);f.transport->LearnedSource(weights);
  assert(PrepareQ38ProjectionExecution(*f.c,f.projection));
  uint64_t before=weights.tileReads;
  for(unsigned role:{10U,11U,5U,6U}){
   printf("ACTUAL_MODEL_TOKEN%u_NATIVE_PROJECTION_ROLE%u\n",token,role);
   assert(RunQ38ProjectionExecution(*f.c,role,f.projection));f.transport->VerifyOutput();
   const auto* output=f.io.wholeWorkspace.data()+PioneerQ38ProjectionOutputOffset(0,role)/4;
   // Independently reads ORIGINAL packed GGUF rows,not tiled callback data.
   bool matches=weights.Validate(role,output);
   if(!matches)fprintf(stderr,"STOP_ACTUAL_MODEL_NUMERIC_MISMATCH token%u role%u;NO subsequent native I/O/END/restore\n",token,role);
   assert(matches && weights.Ready());
   for(unsigned i=0;i<5120;i++)assert(f.io.wholeWorkspace[i]==weights.normalizedReference[i]);
  }
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
  for(unsigned role:{10U,11U,5U,6U})assert(weights.Validate(role,f.io.wholeWorkspace.data()+PioneerQ38ProjectionResultOffset(role)/4));
  puts("PASS ALL16480 ORIGINAL reference words STILL BIT-EXACT simultaneously AFTER four actual QPS1 projections;NO host numeric device writes");
#endif
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
  auto readback=std::make_unique<std::array<uint32,18432>>();
  assert(CloseQ38ProjectionExecution(*f.c,f.projection,readback->data()) && f.projection.validationPages==18
   && f.projection.validationWords==18432 && f.transport->ValidationReads()==18);
  for(unsigned i=0;i<18432;i++)assert((*readback)[i]==f.io.wholeWorkspace[TT_Q38_PROJECTION_RESULT_BASE/4+i]);
  for(unsigned role:{10U,11U,5U,6U})assert(weights.Validate(role,readback->data()+(PioneerQ38ProjectionResultOffset(role)-TT_Q38_PROJECTION_RESULT_BASE)/4));
  puts("PASS ALL16480 ORIGINAL reference words read through ACTUAL fixed18page DEBUG NoC transfer AFTER ALL120holds BEFORE ROOT restore;NO host numeric device writes");
  bool closed=true;
#else
  bool closed=CloseQ38ProjectionExecution(*f.c,f.projection);
#endif
  assert(weights.tileReads-before==164800 && closed && EndFactory(*f.c)
   && f.c->q38Dispatch->closed && !f.c->Unsafe() && original.Ready()
   && f.projection.memory.restoredWords==8134072 && f.projection.memory.verifiedOriginalWords==16268144);
  printf("PASS ACTUAL public GGUF token%u:QPE1+QRN1+ALL119QFP3/4projections/244matrices/80producers/16480original-row-reference outputs BIT-EXACT/119boots once/complete native END;SIMULATED_HARDWARE_NOT_CARD_OR_MODEL_SPEED\n",token);
  tokens++;
 }
 printf("PASS DEBUG original-model native-context instruction validation tokens%u,full SAME-FILE SHA/scope/proofs/independent original numeric references,NO CPU output substitution/device install/inference-speed claim\n",tokens);
}
