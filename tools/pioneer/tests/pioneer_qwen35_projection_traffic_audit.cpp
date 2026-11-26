/* SPDX-License-Identifier: MIT */
// Count projection weight requests in an authenticated recorded fixture.
// This never runs a model, accesses the card, or measures bandwidth.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY 1
#define PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY 1
#include "PioneerQwen35DataflowResidentLifecycle.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdint>
#include <vector>
using namespace PioneerQwen35;
struct Span { unsigned bank,address,bytes; };
struct Step {
 uint32_t magic=0,active=0,tiles=0;
 uint64_t banks[Banks]={};
 std::vector<Span> spans;
};
static bool NonOverlapping(std::vector<Span>& spans) {
 std::sort(spans.begin(),spans.end(),[](const Span& a,const Span& b) {
  return a.bank<b.bank || (a.bank==b.bank && a.address<b.address);
 });
 for(size_t n=1;n<spans.size();n++) {
  const auto& a=spans[n-1];const auto& b=spans[n];
  if(a.bank==b.bank && uint64_t(a.address)+a.bytes>b.address)return false;
 }
 return true;
}
int main(int argc,char** argv) {
 if(argc!=3) return 2;
 std::vector<DataflowGraphRunDescriptor> desc(DataflowCoreCount);
 FILE* f=std::fopen(argv[1],"rb");if(!f)return 1;
 if(std::fread(desc.data(),sizeof(desc[0]),desc.size(),f)!=desc.size()||std::fgetc(f)!=EOF||std::fclose(f))return 1;
 std::array<Step,DataflowGraphDecodeSteps> steps{};
 f=std::fopen(argv[2],"rb");if(!f)return 1;
 for(unsigned core=0;core<DataflowCoreCount;core++) {
  if(!DataflowGraphRunValid(desc[core],core) || desc[core].ticket.count!=DataflowGraphDecodeSteps)return 1;
  for(unsigned i=0;i<steps.size();i++) {
   DataflowStreamRecord record{};if(std::fread(&record,1,sizeof(record),f)!=sizeof(record)
    ||!DataflowStreamRecordStructureValid(record,desc[core].binding.epoch,i+1,core))return 1;
   auto& s=steps[i];if(!s.magic)s.magic=record.header[5];else if(s.magic!=record.header[5])return 1;
   if(s.magic!=DataflowProgramMagic)continue;
   DataflowCoreInvocation inv{};DataflowDynamicRead(record,inv);
   if(!inv.Valid() || inv.core!=core)return 1;
   if(!inv.assignment.active)continue;
   s.active++;s.tiles+=inv.assignment.groups*inv.blocks;
   for(unsigned j=0;j<inv.assignment.segmentCount;j++) {
    const auto& w=inv.assignment.weight[j];s.banks[w.bank]+=w.bytes;
    s.spans.push_back({w.bank,w.address,w.bytes});
   }
  }
 }
 if(std::fgetc(f)!=EOF||std::ferror(f)||std::fclose(f))return 1;
 uint64_t total=0,critical=0,bankTotals[Banks]={};unsigned count=0,hist[121]={};
 for(unsigned i=0;i<steps.size();i++) {auto& s=steps[i];if(s.magic!=DataflowProgramMagic)continue;
  if(!NonOverlapping(s.spans))return 1;
  count++;hist[s.active]++;uint64_t max=0;for(unsigned b=0;b<Banks;b++){total+=s.banks[b];bankTotals[b]+=s.banks[b];max=std::max(max,s.banks[b]);}critical+=max;
  std::printf("step=%u active=%u bytes=%llu maxbank=%llu tiles=%u\n",i+1,s.active,(unsigned long long)(s.banks[0]+s.banks[1]+s.banks[2]+s.banks[3]+s.banks[4]+s.banks[5]+s.banks[6]),(unsigned long long)max,s.tiles);
 }
 std::printf("SUMMARY steps=%u projections=%u weight_bytes=%llu sum_step_busiest_bank=%llu\n",unsigned(steps.size()),count,(unsigned long long)total,(unsigned long long)critical);
 std::printf("BANKS");for(auto v:bankTotals)std::printf(" %llu",(unsigned long long)v);std::puts("");
 std::printf("OCCUPANCY");for(unsigned i=1;i<=120;i++)if(hist[i])std::printf(" %u:%u",i,hist[i]);std::puts("");
 std::puts("OFFLINE RECORDED PROJECTION WEIGHTS ONLY; NOT BANDWIDTH, SPEED OR HARDWARE EVIDENCE");return 0;
}
