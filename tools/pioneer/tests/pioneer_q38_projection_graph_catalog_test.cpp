/* SPDX-License-Identifier: MIT */
// Exact frozen bytes/full L1 footprint DATA;NO device/authority/lifetime.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphImages.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
static std::vector<uint8_t> Read(const std::string& path){
 FILE* f=fopen(path.c_str(),"rb");assert(f && !fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0 && !(size&3));
 assert(!fseek(f,0,SEEK_SET));std::vector<uint8_t> d(static_cast<size_t>(size));assert(fread(d.data(),1,d.size(),f)==d.size() && !fclose(f));return d;
}
static uint32_t Word(const std::vector<uint8_t>& d,unsigned at){
 assert(at+4<=d.size());return uint32_t(d[at])|(uint32_t(d[at+1])<<8)|(uint32_t(d[at+2])<<16)|(uint32_t(d[at+3])<<24);
}
int main(int argc,char** argv){
 assert(argc==3);unsigned code=0,snapshot=0,refusals=0;
 const std::string root=argv[1],peer=argv[2];
 const auto rb=Read(root+"/boot.bin"),rm=Read(root+"/text.bin"),rp=Read(root+"/prefix.bin"),rg=Read(root+"/graph.bin");
 const auto pb=Read(peer+"/boot.bin"),pm=Read(peer+"/text.bin"),pa=Read(peer+"/affine.bin"),ph=Read(peer+"/ffn.bin");
 assert(rb==pb && rb.size()==4 && Word(rb,0)==TT_Q38_PROJECTION_GRAPH_BOOT);
 assert(PioneerQ38FfnGraphProgramWord(0,0x6000)==TT_Q38_PROJECTION_GRAPH_BOOT); // NOT authority
 for(unsigned w=0;w<120;w++){
  assert(PioneerQ38ProjectionGraphCodeBase(w)==code && PioneerQ38ProjectionGraphSnapshotBase(w)==snapshot);
  assert(PioneerQ38ProjectionGraphPrefixBytes(w)==(w?16384U:40960U));
  unsigned previous=0;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i);assert(a>previous);previous=a;
   const std::vector<uint8_t>* section=nullptr;unsigned at=0;
   if(!i){section=&rb;}
   else if(!w){
    if(a>=0x64000 && a-0x64000<rp.size()){section=&rp;at=a-0x64000;}
    if(a>=0x68000 && a-0x68000<rg.size()){section=&rg;at=a-0x68000;}
    if(a>=0x72000 && a-0x72000<rm.size()){section=&rm;at=a-0x72000;}
   }else{
    if(a>=0x64000 && a-0x64000<ph.size()){section=&ph;at=a-0x64000;}
    if(a>=0x70b00 && a-0x70b00<pa.size()){section=&pa;at=a-0x70b00;}
    if(a>=0x72000 && a-0x72000<pm.size()){section=&pm;at=a-0x72000;}
   }
   assert(PioneerQ38ProjectionGraphProgramAddress(w,a) && PioneerQ38ProjectionGraphSnapshotContains(w,a));
   assert(PioneerQ38ProjectionGraphProgramWord(w,a)==(section?Word(*section,at):0));
   assert(PioneerQ38ProjectionGraphExecutableAddress(w,a)==bool(section));
   assert(PioneerQ38ProjectionGraphProgramImmutable(w,a)==(!w || a<0x70000 || a>=0x70b00));
   for(unsigned byte=1;byte<4;byte++){
    assert(!PioneerQ38ProjectionGraphProgramAddress(w,a+byte) && !PioneerQ38ProjectionGraphProgramImmutable(w,a+byte)
     && !PioneerQ38ProjectionGraphExecutableAddress(w,a+byte) && PioneerQ38ProjectionGraphProgramWord(w,a+byte)==UINT32_MAX);refusals++;
   }
   code++;
  }
  unsigned ordinal=0;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSpanCount(w);i++){
   auto s=PioneerQ38ProjectionGraphSnapshotSpan(w,i);assert(!(s.address&3) && s.words && s.address<0x180000 && s.words<=(0x180000-s.address)/4);
   for(unsigned j=0;j<i;j++){
    auto p=PioneerQ38ProjectionGraphSnapshotSpan(w,j);assert(s.address>=p.address+p.words*4 || p.address>=s.address+s.words*4);
   }
   for(unsigned word=0;word<s.words;word++,ordinal++){
    uint32_t a=s.address+word*4;assert(PioneerQ38ProjectionGraphSnapshotAddress(w,ordinal)==a && PioneerQ38ProjectionGraphSnapshotContains(w,a));
   }
  }
  assert(ordinal==PioneerQ38ProjectionGraphSnapshotWords(w));snapshot+=ordinal;
  // Stacks/mutable views/manifest data never become installable code.
  for(uint32_t a:{0x6e000U,0x6fffcU,0x74000U,0x7ffffU,0x80000U,0xabffcU,0xb0000U,0xb82fcU,0xc0000U,0xc82fcU,0x17ffc0U}){
   assert(!PioneerQ38ProjectionGraphProgramAddress(w,a) && !PioneerQ38ProjectionGraphExecutableAddress(w,a));refusals++;
  }
  for(uint32_t a:{0U,0x5ffcU,0x6004U,0x63ffcU,0xac000U,0xafffcU,0xb8300U,0xbfffcU,0xc8300U,0x17ffbcU,0x17ffe0U,0x180000U,0xffb20000U,UINT32_MAX}){
   assert(!PioneerQ38ProjectionGraphSnapshotContains(w,a));refusals++;
  }
  assert(PioneerQ38ProjectionGraphSnapshotContains(w,0xb0000)==!w && PioneerQ38ProjectionGraphSnapshotContains(w,0xc0000)==!w);
  assert(PioneerQ38ProjectionGraphSnapshotContains(w,0x68000)==!w && PioneerQ38ProjectionGraphSnapshotContains(w,0x6e000));
  assert(PioneerQ38ProjectionGraphCodeAddress(w,PioneerQ38ProjectionGraphCodeWords(w))==UINT32_MAX
   && PioneerQ38ProjectionGraphSnapshotAddress(w,ordinal)==UINT32_MAX);
  auto bad=PioneerQ38ProjectionGraphSnapshotSpan(w,PioneerQ38ProjectionGraphSpanCount(w));assert(bad.address==UINT32_MAX && !bad.words);
  if(w){assert(!PioneerQ38FfnGraphProgramAddress(w,0x67000) && PioneerQ38ProjectionGraphProgramAddress(w,0x67000));
   assert(PioneerQ38ProjectionGraphCodeWords(w)==PioneerQ38FfnGraphPrefixBytes(w)/4+4097+1024);}
 }
 for(unsigned w:{120U,121U,UINT32_MAX}){
  assert(!PioneerQ38ProjectionGraphPrefixBytes(w) && !PioneerQ38ProjectionGraphCodeWords(w) && !PioneerQ38ProjectionGraphSpanCount(w)
   && !PioneerQ38ProjectionGraphSnapshotWords(w) && PioneerQ38ProjectionGraphCodeBase(w)==UINT32_MAX && PioneerQ38ProjectionGraphSnapshotBase(w)==UINT32_MAX);
  for(uint32_t a:{0x6000U,0x64000U,0x70000U,0x74000U,0xb0000U,0xc0000U,0x17ffc0U}){
   assert(!PioneerQ38ProjectionGraphProgramAddress(w,a) && !PioneerQ38ProjectionGraphExecutableAddress(w,a)
    && !PioneerQ38ProjectionGraphProgramImmutable(w,a) && !PioneerQ38ProjectionGraphSnapshotContains(w,a)
    && PioneerQ38ProjectionGraphProgramWord(w,a)==UINT32_MAX);refusals++;
  }
  assert(PioneerQ38ProjectionGraphCodeAddress(w,0)==UINT32_MAX && PioneerQ38ProjectionGraphSnapshotAddress(w,0)==UINT32_MAX);
 }
 assert(code==989304 && snapshot==8134072);
 printf("PASS frozen complete QPR1/QFP3 DATA ALL120/codeWords%u/snapshotWords%u/refusals%u;actual bytes+zero padding,full16KiB peer/40KiB ROOT/ALL120stacks+trailers/new primary+shadow,MMIO/holes excluded;NOT_NATIVE_OWNER_MAPPING_PRODUCER_OR_SPEED\n",code,snapshot,refusals);
}
