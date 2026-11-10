/* SPDX-License-Identifier: MIT */
// Execute the actual linked ROOT producer-frame builders. Synthetic metadata
// only: no card, live worker, full model, ownership or throughput claim.
#define PIONEER_Q38_HYBRID_ACTUAL_ROOT_MACHINE_ONLY
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#include "pioneer_q38_hybrid_actual_root_test.cpp"
#include "../PioneerQ38DenseQ8MatrixWorkerLayout.h"
#include "../PioneerQ38DenseQ8MatrixRootJobs.h"
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
#include "../PioneerQ38Bf16ProducerLayout.h"
#include "../PioneerQ38DenseQ8MatrixManifest.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DenseProjectionPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8KernelLayout.h"
#include <fstream>
namespace Q=PioneerQ38DenseQ8MatrixWorkerLayout;
namespace P=PioneerQ38Bf16ProducerLayout;
struct Schedule {
 uint32_t table[7][3]={},parts[119][4]={};
 unsigned Table(unsigned bank,unsigned field){return table[bank][field];}
 void Output(unsigned worker,unsigned field,unsigned value){parts[worker][field]=value;}
};
static std::vector<unsigned> Symbols(const char* folder,const char* name){
 std::ifstream f(std::string(folder)+"/program.demangled");std::string line;std::vector<unsigned> out;
 while(std::getline(f,line))if(line.find(name)!=std::string::npos && line.find("[clone")==std::string::npos)out.push_back(unsigned(std::stoul(line.substr(0,8),nullptr,16)));
 assert(out.size()==2);return out;
}
static bool Call(ActualRoot& m,unsigned pc,unsigned d,unsigned worker,unsigned sequence,unsigned output,bool four){
 std::fill(std::begin(m.regs),std::end(m.regs),0);m.regs[1]=0x60004;m.regs[2]=0x70000;
 m.regs[10]=d;m.regs[11]=worker;m.regs[12]=four?sequence:output;if(four)m.regs[13]=output;
 m.pc=pc;m.steps=0;while(m.pc!=0x60004 && m.pc!=0x60000){m.Step();assert(m.regs[2]>=0x6e000 && m.regs[2]<=0x70000);}
 return m.pc==0x60004;
}
static bool Reaches(ActualRoot& m,unsigned start,unsigned d,unsigned target){
 std::fill(std::begin(m.regs),std::end(m.regs),0);m.regs[1]=0x60004;m.regs[2]=0x70000;m.regs[10]=d;
 m.pc=start;m.steps=0;
 while(m.pc!=target && m.pc!=0x60000 && m.pc!=0x60004){m.Step();assert(m.regs[2]>=0x6e000 && m.regs[2]<=0x70000);}
 return m.pc==target;
}
int main(int argc,char** argv){
 assert(argc==2);auto m=std::make_unique<ActualRoot>(argv[1]);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridDenseQ8MatrixPlan::Proposal physical;
 std::array<uint32_t,PioneerQ38DenseQ8MatrixManifest::Words> physicalManifest={};
 assert(PioneerQ38HybridDenseQ8MatrixPlan::Build(*canonical,physical)
  && PioneerQ38DenseQ8MatrixManifest::Build(*canonical,physical,physicalManifest.data(),physicalManifest.size()));
 auto descriptors=Symbols(argv[1],"Projection::BfDescriptor(");auto jobs=Symbols(argv[1],"Projection::BfProducerJob(");
 const unsigned d=0x74e00,out=0x78000;for(unsigned i=0;i<64;i++)m->memory[d+4*i]=0;
 m->memory[d+4*3]=0;m->memory[d+4*4]=0x04000000;m->memory[d+4*6]=2025;
 m->memory[d+4*8]=PioneerQ38ProjectionRoute(0);m->memory[d+4*14]=5120;m->memory[d+4*15]=8;
 for(unsigned i=0;i<4;i++)m->memory[d+4*(32+i*2)]=i+1;
 m->memory[d+4*41]=248319;m->memory[d+4*42]=3;
 for(unsigned w:{0U,19U}){
  uint32_t boot[64]={};boot[0]=Q::Magic;boot[1]=1;boot[2]=64;boot[3]=w+1;boot[4]=Q::Peer(w+1);
  boot[5]=boot[16]=0x14000;boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
  for(unsigned i=0;i<3;i++)for(unsigned lane=0;lane<6;lane++)boot[17+i]|=(i*6+lane)<<(lane*5);
  for(unsigned i=0;i<2;i++)for(unsigned lane=0;lane<6;lane++)boot[23+i]|=(i*6+lane)<<(lane*5);
  for(unsigned i=40;i<=50;i++)boot[i]=Q::BootWord(i);assert(Q::BootShape(boot));
  for(unsigned i=0;i<64;i++)m->memory[0x170000+w*256+i*4]=boot[i];
  for(unsigned role=5;role<=7;role++)for(unsigned component=0;component<2;component++){
   m->memory[d+4*43]=role;
   if(!Call(*m,descriptors[component],d,w,0,out,false)){
    fprintf(stderr,"descriptor fault w=%u role=%u component=%u\n",w,role,component);
    for(unsigned i=0;i<64;i++)fprintf(stderr,"%u:%08x%c",i,m->memory[out+4*i],i%8==7?'\n':' ');
    assert(false);
   }
   uint32_t descriptor[64];
   for(unsigned i=0;i<64;i++)descriptor[i]=m->memory.at(out+4*i);
   assert(P::DescriptorShape(descriptor) && descriptor[5]==w && descriptor[7]==w+1 && descriptor[43]==role);
   assert(Call(*m,jobs[component],d,w,7,out,true));uint32_t job[64];
   for(unsigned i=0;i<64;i++)job[i]=m->memory.at(out+4*i);
   assert(Q::JobShape(job,boot,6,0) && job[28]==role && job[29]==5120 && job[1]==7);
  }
 }
 m->memory[d+4*43]=8;assert(!Call(*m,descriptors[0],d,0,0,out,false));
 m->memory[d+4*43]=5;m->memory[d+4*42]=2;assert(!Call(*m,descriptors[0],d,0,0,out,false));
 m->memory[d+4*42]=3;assert(!Call(*m,descriptors[0],d,20,0,out,false));
 assert(!Call(*m,jobs[0],d,0,0,out,true));
 auto seals=Symbols(argv[1],"Projection::BfDenseSeal(");
 auto validators=Symbols(argv[1],"PioneerQ38DenseQ8MatrixWorkerLayout::JobShape(");
 auto stores=Symbols(argv[1],"Projection::BfStoreDenseMatrix(");
 auto runs=Symbols(argv[1],"Projection::Run(");auto prepares=Symbols(argv[1],"Projection::BfPrepare(");
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned role=5;role<=7;role++)for(unsigned component=0;component<2;component++){
  m->memory[d+4*42]=layer;m->memory[d+4*43]=role;m->memory[d+4*11]=role==5?12288:1024;Schedule expected;
  const unsigned index=((layer/4)*3+role-5)*21;
  for(unsigned bank=0;bank<7;bank++){
   for(unsigned field=0;field<3;field++){
    expected.table[bank][field]=physicalManifest[index+bank*3+field];
    m->memory[0x16dc00+4*(index+bank*3+field)]=expected.table[bank][field];
   }
  }
  assert(PioneerQ38DenseQ8MatrixRootJobs::Build(expected,layer,role));
  assert(m->Run(m->build[component],d));
  for(unsigned w=0;w<119;w++)for(unsigned field=0;field<4;field++){
   assert(m->memory.at(0x16ebc0+w*16+field*4)==expected.parts[w][field]);
   assert(m->memory.at(0x16f330+w*16+field*4)==expected.parts[w][field]);
  }
  for(unsigned w:{0U,19U}){
   uint32_t boot[64],frame[64];for(unsigned i=0;i<64;i++)boot[i]=m->memory.at(0x170000+w*256+i*4);
   assert(PioneerQ38DenseQ8MatrixRootFrame::Build(expected.parts[w],boot,layer,role,0,2025,6,frame));
   assert(m->Run(m->job[component],d,w,7));
   if(layer==3 && w==0)printf("QGWD_DENSE_JOB_SIMULATED_INSTRUCTIONS role=%u component=%u instructions=%llu; NOT_CYCLES_OR_TPS\n",
    role,component,static_cast<unsigned long long>(m->steps));
   for(unsigned i=0;i<64;i++)assert(m->memory.at(out+4*i)==frame[i]);
   assert(Call(*m,validators[component],out,0x170000+w*256,6,0,true) && m->regs[10]==1);
   m->memory[out+34*4]=8;assert(Call(*m,validators[component],out,0x170000+w*256,6,0,true) && !m->regs[10]);
   m->memory[out+34*4]=role;
   const unsigned count=expected.parts[w][3]*32,first=expected.parts[w][2];
   for(unsigned i=0;i<count;i++)m->memory[out+256+i*4]=0x3f000000+i;
   assert(m->Run(stores[component],d,w));
   if(layer==3 && w==0)printf("QGWD_DENSE_STORE_SIMULATED_INSTRUCTIONS role=%u component=%u instructions=%llu; NOT_CYCLES_OR_TPS\n",
    role,component,static_cast<unsigned long long>(m->steps));
   for(unsigned i=0;i<count;i++)assert(m->memory.at(0x80000+(first+i)*4)==0x3f000000+i);
   m->memory[out+256+(count-1)*4]=0x7fc00000;
   for(unsigned i=0;i<count;i++)m->memory[0x80000+(first+i)*4]=0xa5a5a5a5;
   assert(!m->Run(stores[component],d,w));
   for(unsigned i=0;i<count;i++)assert(m->memory.at(0x80000+(first+i)*4)==0xa5a5a5a5);
  }
  assert(m->Run(seals[component],d));
  if(layer==3)printf("QGWD_DENSE_SEAL_SIMULATED_INSTRUCTIONS role=%u component=%u instructions=%llu; NOT_CYCLES_OR_TPS\n",
   role,component,static_cast<unsigned long long>(m->steps));
  if(layer==3 && role==5){
   for(unsigned w=0;w<119;w++)for(unsigned field=0;field<4;field++){
    const unsigned address=0x16ebc0+w*16+field*4;
    m->memory[address]^=1;assert(!m->Run(seals[component],d));m->memory[address]^=1;
    const unsigned shadow=0x16f330+w*16+field*4;
    m->memory[shadow]^=1;assert(!m->Run(seals[component],d));m->memory[shadow]^=1;
   }
   for(unsigned i=0;i<21;i++){
    const unsigned address=0x16dc00+4*(index+i);
    m->memory[address]^=1;assert(!m->Run(seals[component],d));m->memory[address]^=1;
    const unsigned shadow=0x16fac0+i*4;
    m->memory[shadow]^=1;assert(!m->Run(seals[component],d));m->memory[shadow]^=1;
   }
  }
  m->memory[0x16ebc0]^=1;assert(!m->Run(seals[component],d));
  assert(!m->Run(m->job[component],d,0,7));m->memory[0x16ebc0]^=1;
  if(role!=5)assert(!m->Run(m->job[component],d,32,7));
  m->memory[0x16dc00+4*index]^=1;assert(!m->Run(seals[component],d));m->memory[0x16dc00+4*index]^=1;
  m->memory[0x16dc00+4*index]++;assert(!m->Run(m->build[component],d));m->memory[0x16dc00+4*index]--;
 }
 // Exercise the actual dense-component entry through canonical recipe
 // capture and stop at the BF16 producer entry, before any NoC/MMIO.
 auto parent=std::make_unique<PioneerQ38Qgw8KernelLayout>();auto relocated=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(parent->Build(*canonical) && relocated->BuildDenseQ8LogicalParentInspection(*parent) && relocated->Qgw8InspectionLayout());
 std::array<uint32_t,1920> sources={};for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 for(const auto* geometry:{canonical.get(),relocated.get()})for(unsigned layer=3;layer<64;layer+=4){
 auto plan=std::make_unique<PioneerQ38DenseProjectionPlan>();assert(plan->Build(*geometry,layer,0,248319,0,1000,{1,2,3,4},sources.data()));
 for(unsigned slot=0;slot<3;slot++){
  uint32_t runtime[64];for(unsigned i=0;i<64;i++)runtime[i]=plan->Frame(slot)[i];
  runtime[0]=0x31504d51;runtime[13]=0x76800;runtime[48]=1;runtime[49]=119;runtime[50]=0x75800;
  runtime[51]=0x76b00;runtime[52]=0x76a00;runtime[53]=0x140000;runtime[54]=0x80000;
  runtime[55]=0x178000;runtime[56]=0x123000;runtime[57]=0x170000;
  for(unsigned i=0;i<64;i++){m->memory[d+4*i]=runtime[i];m->memory[0x74600+4*i]=runtime[i];}
  for(unsigned i=0;i<16;i++)m->memory[0x76b00+4*i]=plan->Header(slot)[i];
  for(unsigned i=0;i<8;i++)m->memory[0x76a00+4*i]=plan->Banks()[i];
  for(unsigned w=0;w<119;w++){
   for(unsigned i=0;i<8;i++)m->memory[0x75800+w*32+i*4]=plan->Parts(slot)[w][i];
   m->memory[0x76800+w*4]=plan->Rows(slot)[w];
  }
  assert(Reaches(*m,runs[1],d,prepares[1]));
  m->memory[0x75800]++;assert(!Reaches(*m,runs[1],d,prepares[1]));m->memory[0x75800]--;
 }
 }
 puts("PASS actual QGWD dense dispatch accepts 16-layer original/relocated recipes and planned physical manifest; producer/schedule/job simulation; NO HARDWARE OR TPS");
}
