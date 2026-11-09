/* SPDX-License-Identifier: MIT */
// Geometry/alias arithmetic only; no live mapping, save/restore or device.
#define PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION 1
#define PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC 1
#include "../PioneerQ38DenseQ8TokenFootprint.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
namespace D=PioneerQ38DenseQ8TokenL1;namespace H=PioneerQ38HybridTokenL1;
static bool Alias(uint32_t a){
 if(a==0x6000)return true;
 if(a>=0x20000 && a<0x60000)return true;
 if(a>=0x60000 && a<0x6e000)return true;
 if(a>=0x6e000 && a<0xac000)return true;
 if(a>=0xac000 && a<0x17ffc0)return true;
 return a>=0x17ffc0 && a<0x180000;
}
static std::vector<unsigned char> Read(const std::string& path){
 std::ifstream input(path,std::ios::binary);assert(input);
 return std::vector<unsigned char>(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>());
}
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && argv[1][0]=='/'));
 for(unsigned worker=0;worker<120;worker++){
  unsigned total=0;uint32_t end=0;
  for(unsigned i=0;i<D::SnapshotSpans(worker);i++){
   const auto s=D::SnapshotSpan(worker,i),old=H::SnapshotSpan(worker,i);
   assert(s.address==old.address && s.words>=old.words && (!i || s.address>=end));
   assert(s.address<=UINT32_MAX-s.words*4);end=s.address+s.words*4;
   assert(Alias(s.address) && Alias(end-4));
   assert(D::SnapshotAddress(worker,total)==s.address && D::SnapshotAddress(worker,total+s.words-1)==end-4);
   assert(D::SnapshotContains(worker,old.address) && D::SnapshotContains(worker,old.address+(old.words-1)*4));
   total+=s.words;
  }
  assert(total==D::SnapshotWords(worker) && D::SnapshotAddress(worker,total)==UINT32_MAX);
  if(worker){
   assert(D::SnapshotSpan(worker,1).words==4096 && D::SnapshotContains(worker,0x20000+13148-4));
   assert(!H::SnapshotContains(worker,0x20000+13148-4) && !D::SnapshotContains(worker,0x24000));
  }
 }
 assert(D::SnapshotBase(119)+D::SnapshotWords(119)==D::SavedWords && D::SnapshotWords(120)==0);
 assert(D::SnapshotBase(120)==UINT32_MAX && !D::SnapshotContains(120,0x20000));
 if(argc==2){
  // Load exact compiled QGWD peer sections into a disposable fake L1. The
  // ROOT is deliberately absent: this is not an executable mixed bundle or
  // the production TokenMemory algorithm, only an exact footprint check.
  struct Segment{const char* name;unsigned address;};
  const Segment code[]={{"boot",0x6000},{"globalworker",0x20000},{"bf16producer",0x30000},
   {"dense",0x40000},{"cachedfpu",0x50000},{"ffn",0x60000},{"service",0x68000},
   {"affine",0x70b00},{"text",0x72000},{"gdn",0x170000}};
  std::vector<std::vector<unsigned char>> sections;
  for(const auto& part:code){
   sections.push_back(Read(std::string(argv[1])+"/"+part.name+".bin"));
   assert(!sections.back().empty() && !(sections.back().size()&3));
  }
  assert(sections[1].size()==13148);
  for(unsigned worker=1;worker<120;worker++){
   std::vector<uint32_t> l1(0x180000/4),saved(D::SnapshotWords(worker));
   for(unsigned i=0;i<l1.size();i++)l1[i]=0x9e3779b9U*(i+1)^worker;
   const auto original=l1;
   for(unsigned n=0;n<saved.size();n++)saved[n]=l1[D::SnapshotAddress(worker,n)/4];
   for(unsigned segment=0;segment<sections.size();segment++){
    const auto& bytes=sections[segment];const unsigned first=code[segment].address;
    for(unsigned n=0;n<bytes.size();n+=4){
     const unsigned address=first+n;assert(D::SnapshotContains(worker,address));
     l1[address/4]=unsigned(bytes[n])|(unsigned(bytes[n+1])<<8)|(unsigned(bytes[n+2])<<16)|(unsigned(bytes[n+3])<<24);
    }
   }
   assert(l1[(0x20000+13148-4)/4]!=original[(0x20000+13148-4)/4]);
   for(unsigned n=0;n<saved.size();n++)l1[D::SnapshotAddress(worker,n)/4]=saved[n];
   assert(l1==original);
  }
  puts("PASS exact QGWD peer binary spans fit enlarged journal and simulated snapshot/restore on all 119 workers; NO native algorithm or device");
 }
 std::printf("PASS QGWB proposed journal %u bytes; old spans and new code covered, alias geometry only; NO NATIVE OR DEVICE\n",D::SavedBytes);
}
