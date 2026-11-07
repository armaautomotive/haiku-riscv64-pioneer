/* SPDX-License-Identifier: MIT */
// Execute actual compiled RV32 translation against the independent canonical
// host plan. The test inputs are metadata only, not uploaded device weights.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_Q38_HYBRID_BINDING_ONLY
#include "pioneer_q38_hybrid_job_binding_test.cpp"
#include <string>
struct RootPairMachine:Machine {
 std::vector<uint8_t> code;unsigned park=0,stores=0,minSP=0x70000;
 RootPairMachine(const char* directory,const RootProbe& root,unsigned layer,unsigned k,unsigned rows){
  code=Read((std::string(directory)+"/text.bin").c_str());auto boot=Read((std::string(directory)+"/boot.bin").c_str());
  assert(!code.empty() && code.size()<=8192 && boot.size()==4);
  std::ifstream sym(std::string(directory)+"/symbols");std::string a,t,n;
  while(sym>>a>>t>>n)if(n=="native_dram_band_park")park=unsigned(std::stoul(a,nullptr,16));
  assert(park);l1[0]=Word(boot,0);for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
  for(unsigned w=0;w<119;w++){for(unsigned i=0;i<8;i++)Put(0x76000+w*32+i*4,root.parts[w][i]);Put(0x77000+w*4,root.rows[w]);}
  for(unsigned b=0;b<7;b++)for(unsigned i=0;i<4;i++)Put(0x77200+b*16+i*4,root.table[b][i]);
  Put(0x74000,layer);Put(0x74004,k);Put(0x74008,rows);
 }
 static bool Range(unsigned a,unsigned start,unsigned size){return a>=start && a-start<size;}
 unsigned Index(unsigned a)override{
  assert(!(a&3));if(a==0x6000)return 0;
  if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
  if(Range(a,0x6e000,8192))return 2049+(a-0x6e000)/4;
  if(Range(a,0x76000,119*32))return 4097+(a-0x76000)/4;
  if(Range(a,0x77000,119*4))return 5050+(a-0x77000)/4;
  if(Range(a,0x77200,7*16))return 5170+(a-0x77200)/4;
  if(Range(a,0x74000,12))return 5200+(a-0x74000)/4;
  if(Range(a,0x17ffc0,12))return 5203+(a-0x17ffc0)/4;
  assert(false);return 0;
 }
 bool CodeAddress(unsigned a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
 unsigned StepLimit()const override{return 2000000;}
 void Put(unsigned a,unsigned v){l1[Index(a)]=v;}
 unsigned Get(unsigned a){return l1[Index(a)];}
 unsigned Load(unsigned a)override{return Get(a);}
 void Store(unsigned a,unsigned v)override{
  assert(Range(a,0x6e000,8192) || Range(a,0x76000,119*32) || Range(a,0x17ffc0,12));
  if(Range(a,0x76000,119*32)){assert((a-0x76000)%32==0);stores++;}Put(a,v);
 }
 unsigned Execute(){
  while(pc!=park){Step();if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
  assert(Get(0x17ffc8)==0x3142444e);return Get(0x17ffc4);
 }
};
int main(int argc,char** argv){
 (void)&HW::JobShape;
 assert(argc==2);auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridJobBinding binding;assert(binding.Bind(*canonical,UINT64_C(0x1fff000000000000)));
 auto* proposal=binding.Plan();assert(proposal);unsigned count=0;
 for(unsigned layer:{0U,47U,48U,60U,61U})for(unsigned role:{2U,4U}){
  PioneerQ38PairedProjectionJobs paired;assert(paired.Build(*canonical,layer,role,0));
  auto original=std::make_unique<PioneerQ38ProjectionJobs>();assert(original->Build(*canonical,layer,role,0));
  auto root=std::make_unique<RootProbe>(RootProbe{*original});
  for(unsigned s=0;s<7;s++){
   const auto* stripe=canonical->ProjectionStripe(layer,role,s);assert(stripe);
   unsigned index=canonical->Count();for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)==stripe){index=i;break;}
   assert(index<proposal->parts.size());const auto& d=proposal->parts[index];
   root->table[stripe->bank][0]=canonical->Base+stripe->offset;root->table[stripe->bank][1]=canonical->Base+d.offset;
   root->table[stripe->bank][2]=stripe->firstRow;root->table[stripe->bank][3]=stripe->rows;
  }
  assert(PioneerQ38Bf16RootPairs::Build(*root,paired.Inputs(),paired.Outputs()));
  auto machine=std::make_unique<RootPairMachine>(argv[1],*root,layer,paired.Inputs(),paired.Outputs());
  assert(machine->Execute()==1 && machine->stores==119);
  for(unsigned w=0;w<119;w++){
   unsigned expected=0;if(layer>=48 && layer<=60){PioneerQ38Bfp8WorkerJob::Job j={};assert(binding.Build8(paired,layer,role,0,w,7,j));expected=j.words[3];}
   else {PioneerQ38Bfp4WorkerJob::Job j={};assert(binding.Build4(paired,layer,role,0,w,7,j));expected=j.words[3];}
   assert(machine->Get(0x76000+w*32)==expected);count++;
  }
  auto bad=std::make_unique<RootPairMachine>(argv[1],*root,layer,paired.Inputs(),paired.Outputs());
  bad->Put(0x77200,bad->Get(0x77200)^1);assert(!bad->Execute() && !bad->stores);
 }
 assert(count==1190);puts("PASS actual RV32 hybrid ROOT translation: 1190 assignments, both formats and widths, boundary layers, invalid mapping stops without writes; simulated only");
}
