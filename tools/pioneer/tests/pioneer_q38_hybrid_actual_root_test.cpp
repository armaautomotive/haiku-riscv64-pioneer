/* SPDX-License-Identifier: MIT */
// Execute real linked ROOT functions with canonical metadata in the RV32
// simulator. Direct function harness: not full ROOT lifecycle or device IO.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_Q38_HYBRID_BINDING_ONLY
#include "pioneer_q38_hybrid_job_binding_test.cpp"
#include <map>
#include <string>
struct ActualRoot:Machine {
 std::vector<std::pair<unsigned,std::vector<uint8_t>>> rom;
 std::map<unsigned,unsigned> memory;std::vector<unsigned> build,job;
 unsigned minSP=0x70000;
 ActualRoot(const char* image){
  for(auto p:{std::make_pair("bf16",0x20000U),{"text",0x40000U},{"terminal",0x60000U},{"conv",0x64000U}})
   rom.push_back({p.second,Read((std::string(image)+"/"+p.first+".bin").c_str())});
  std::ifstream f(std::string(image)+"/program.demangled");std::string line;
  while(std::getline(f,line)){
   if(line.find("Projection::BfBuildPairs(")!=std::string::npos)build.push_back(unsigned(std::stoul(line.substr(0,8),nullptr,16)));
   if(line.find("Projection::BfMatrixJob(")!=std::string::npos)job.push_back(unsigned(std::stoul(line.substr(0,8),nullptr,16)));
  }
  assert(build.size()==2 && job.size()==2);
 }
 static bool Range(unsigned a,unsigned start,unsigned bytes){return a>=start && a-start<bytes;}
 const std::vector<uint8_t>* Rom(unsigned a,unsigned& base)const{
  for(const auto& p:rom)if(Range(a,p.first,unsigned(p.second.size()))){base=p.first;return &p.second;}return nullptr;
 }
 bool CodeAddress(unsigned a)const override{unsigned base=0;return Rom(a,base)!=nullptr;}
 unsigned Index(unsigned a)override{unsigned base=0;auto* p=Rom(a,base);assert(p && !(a&3));l1[8190]=Word(*p,a-base);return 8190;}
 unsigned Load(unsigned a)override{
  unsigned base=0;if(auto* p=Rom(a,base))return Word(*p,a-base);
  auto found=memory.find(a);if(found==memory.end())fprintf(stderr,"missing ROOT data %x at pc %x\n",a,pc);
  assert(found!=memory.end());return found->second;
 }
 void Store(unsigned a,unsigned value)override{
  unsigned base=0;assert(!Rom(a,base));assert(!(a&3));memory[a]=value;
 }
 unsigned StepLimit()const override{return 4000000;}
 bool Run(unsigned address,unsigned d,unsigned worker=0,unsigned sequence=0){
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;regs[10]=d;regs[11]=worker;regs[12]=sequence;
  pc=address;steps=0;
  while(pc!=0x60004 && pc!=0x60000){Step();assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minSP=std::min(minSP,regs[2]);}
  return pc==0x60004;
 }
};
int main(int argc,char** argv){
 (void)&HW::JobShape;
 assert(argc==2);auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridJobBinding binding;assert(binding.Bind(*canonical,UINT64_C(0x1fff000000000000)));
 uint32_t manifest[PioneerQ38HybridJobBinding::RootManifestWords];assert(binding.RootManifest(manifest,PioneerQ38HybridJobBinding::RootManifestWords));
 unsigned compared=0;
 for(unsigned component=0;component<2;component++)for(unsigned layer:{0U,3U,47U,48U,60U,61U})for(unsigned role:{2U,4U}){
  PioneerQ38PairedProjectionJobs paired;assert(paired.Build(*canonical,layer,role,0));
  auto original=std::make_unique<PioneerQ38ProjectionJobs>();assert(original->Build(*canonical,layer,role,0));
  RootProbe source{*original};auto m=std::make_unique<ActualRoot>(argv[1]);const unsigned d=0x74e00;
  for(unsigned i=0;i<64;i++)m->memory[d+i*4]=0;
  m->memory[d+6*4]=2025;m->memory[d+11*4]=paired.Outputs();m->memory[d+14*4]=paired.Inputs();m->memory[d+15*4]=12;
  m->memory[d+42*4]=layer;m->memory[d+43*4]=role;for(unsigned i=0;i<4;i++)m->memory[d+(32+i*2)*4]=i+1;
  for(unsigned i=0;i<PioneerQ38HybridJobBinding::RootManifestWords;i++)m->memory[0x16c400+i*4]=manifest[i];
  for(unsigned b=0;b<8;b++)m->memory[0x76d40+b*4]=PioneerQ38ProjectionRoute(b);
  for(unsigned w=0;w<119;w++){
   for(unsigned i=0;i<8;i++)m->memory[0x77000+w*32+i*4]=i==5?0x14000:source.Part(w,i);
   m->memory[0x76e00+w*4]=source.Row(w);
   uint32_t boot[64]={};boot[0]=HW::Magic;boot[1]=1;boot[2]=64;boot[3]=w+1;boot[4]=HW::Peer(w+1);
   boot[5]=0x14000;boot[6]=8192;boot[7]=64;for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
   boot[16]=boot[5];for(unsigned i=0;i<3;i++)for(unsigned lane=0;lane<6;lane++)boot[17+i]|=(i*6+lane)<<(lane*5);
   for(unsigned i=0;i<2;i++)for(unsigned lane=0;lane<6;lane++)boot[23+i]|=(i*6+lane)<<(lane*5);
   for(unsigned i=40;i<=50;i++)boot[i]=HW::BootWord(i);assert(HW::BootShape(boot));
   for(unsigned i=0;i<64;i++)m->memory[0x170000+w*256+i*4]=boot[i];
  }
  assert(m->Run(m->build[component],d));
  for(unsigned w:{0U,59U,118U}){
   assert(m->Run(m->job[component],d,w,7));
   uint32_t expected[16];
   if(layer>=48 && layer<=60){PioneerQ38Bfp8WorkerJob::Job j={};assert(binding.Build8(paired,layer,role,0,w,7,j));for(unsigned i=0;i<16;i++)expected[i]=j.words[i];}
   else {PioneerQ38Bfp4WorkerJob::Job j={};assert(binding.Build4(paired,layer,role,0,w,7,j));for(unsigned i=0;i<16;i++)expected[i]=j.words[i];}
   for(unsigned i=0;i<16;i++)assert(m->memory.at(0x78000+(16+i)*4)==expected[i]);
   assert(m->memory.at(0x78000+32*4)==paired.Part(w)->firstRow && m->memory.at(0x78000+35*4)==(layer>=48 && layer<=60?8U:4U));
   uint32_t frame[64],boot[64];for(unsigned i=0;i<64;i++){
    frame[i]=m->memory.at(0x78000+i*4);boot[i]=m->memory.at(0x170000+w*256+i*4);
   }
   assert(HW::JobShape(frame,boot,6,0));
#ifdef PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
   assert(frame[0]==0x37574751U && boot[0]==0x37574751U);
   frame[0]=0x35574751U;assert(!HW::JobShape(frame,boot,6,0));
   frame[0]=HW::Magic;boot[0]=0x35574751U;assert(!HW::JobShape(frame,boot,6,0));
#endif
   compared++;
  }
  const unsigned index=(layer*3+role-2)*7;m->memory[0x16c400+index*4]^=1;
  assert(!m->Run(m->job[component],d,0,8));
 }
 assert(compared==72);
#ifdef PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
 puts("PASS actual linked QGW7 ROOT paired translation and job construction: 72 jobs, both components/formats/widths, boundary layers; old identity and manifest mutation refused; simulated only");
#else
 puts("PASS actual linked hybrid ROOT paired translation and job construction: 72 jobs across both real component functions, both formats/widths and boundary layers; manifest mutation faults; simulated only");
#endif
}
