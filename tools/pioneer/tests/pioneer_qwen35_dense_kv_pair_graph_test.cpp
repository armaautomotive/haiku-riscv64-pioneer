/* SPDX-License-Identifier: MIT */
// Offline fused graph recorder: pair K and V on disjoint output scratch.
#define PIONEER_QWEN35_DENSE_KV_PAIR_COMPILE_ONLY 1
#define PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY 1
#define PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY 1
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define main q35_dense_pair_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowDynamicToken.h"
#include "../PioneerQwen35DataflowGraphRunProtocol.h"

struct DensePairSink {
 const Fixture& fixture;unsigned writes=0,key=0,value=0,pairs=0,packKey=0,packValue=0,attention=0;
 bool sealed=false,aborted=false,failPair=false;
 DataflowDynamicTokenContext dynamic{};
 explicit DensePairSink(const Fixture& f):fixture(f) {}
 bool Begin(const DataflowStreamPlan&) {return true;}
 bool StageRecord(const DataflowStreamBinding& b,const Range&,unsigned step,const DataflowStreamRecord& record) {
  assert(b.core==writes%DataflowCoreCount && step==writes/DataflowCoreCount);
  assert(DataflowStreamRecordStructureValid(record,b.epoch,step+1,b.core));++writes;
  auto patched=record;
  assert(DataflowDynamicPatch(patched,dynamic,b.epoch,step+1,b.core));
  assert(DataflowStreamRecordStructureValid(patched,b.epoch,step+1,b.core));
  if (record.header[5]==DataflowProgramMagic) {
   DataflowCoreInvocation inv{};DataflowDynamicRead(record,inv);assert(inv.Valid());
   if (inv.assignment.active) for (unsigned layer=0;layer<Layers;layer++) if (Dense(layer)) {
    if (DataflowRowSpaceMatches(inv.weights,fixture.manifest.layers[layer].weights[Key].place)) {
     assert(DataflowRowSpaceMatches(inv.output,fixture.workspace.space[DataflowGraphRecurrentGateWorkspaceIndex]));++key;
    }
    if (DataflowRowSpaceMatches(inv.weights,fixture.manifest.layers[layer].weights[Value].place)) {
     assert(DataflowRowSpaceMatches(inv.output,fixture.workspace.space[DataflowGraphUpWorkspaceIndex]));++value;
    }
   }
   if (failPair && (key || value) && b.core==80) return false;
  } else if (record.header[5]==DataflowDenseAttentionMagic && b.core==0) {
   DataflowDenseAttentionInvocation inv{};DataflowDynamicRead(record,inv);assert(inv.Valid());
   if (inv.program.mode==DataflowDenseAttentionPackKey) {
    assert(DataflowRowSpaceMatches(inv.program.gate,fixture.workspace.space[DataflowGraphRecurrentGateWorkspaceIndex]));
    ++packKey;
   } else if (inv.program.mode==DataflowDenseAttentionPackValue) {
    assert(DataflowRowSpaceMatches(inv.program.gate,fixture.workspace.space[DataflowGraphUpWorkspaceIndex]));
    assert(DataflowRowSpaceMatches(inv.program.gate,inv.program.contextOutput));++packValue;
   } else if (inv.program.mode==DataflowDenseAttentionRun) ++attention;
  } else if (record.header[5]==DataflowFlashHybridMagic && b.core==0) {
   DataflowFlashHybridInvocation inv{};DataflowDynamicRead(record,inv);assert(inv.Valid());++attention;
  }
  if (b.core==DataflowCoreCount-1) {
   if (key || value) {assert(key==60 && value==60);++pairs;}
   key=value=0;
  }
  return true;
 }
 bool Seal(const DataflowStreamPlan&) {sealed=true;return true;}
 void Abort() {aborted=true;}
};

int main() {
 (void)&q35_dense_pair_fixture_main;
 static_assert(DataflowStreamVersion==7 && DataflowGraphDecodeSteps==428 && DataflowGraphPromptSteps==425,
  "dense K/V pairing has its own stream version and removes one step per dense layer");
 auto fixture=std::make_unique<Fixture>();
 for (bool decode:{false,true}) {
  auto live=std::make_unique<DataflowProjectionAuthority>();auto candidate=std::make_unique<DataflowProjectionAuthority>();
  auto plan=std::make_unique<DataflowStreamPlan>();auto record=std::make_unique<DataflowStreamRecord>();
  auto projection=std::make_unique<DataflowProjectionProgram>();auto sink=std::make_unique<DensePairSink>(*fixture);unsigned stage=0;
  assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
  assert(DataflowDynamicTokenContext::Build(fixture->manifest,43,1,sink->dynamic));
  const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
  assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,fixture->rope,
   fixture->routes,99,count,*plan));
  assert(DataflowStreamRecording::Build(fixture->packed,*live,*candidate,*plan,fixture->routes,
   0,42,0,decode,*sink,*record,*projection,stage));
  assert(sink->sealed && !sink->aborted && sink->pairs==8 && sink->packKey==8
   && sink->packValue==8 && sink->attention==16 && sink->writes==count*DataflowCoreCount);
  if (decode) {
   DataflowGraphAuthorityState prior{};assert(candidate->CaptureGraphState(prior));
   auto next=std::make_unique<DataflowProjectionAuthority>();auto warm=std::make_unique<DensePairSink>(*fixture);
   assert(DataflowDynamicTokenContext::Build(fixture->manifest,44,2,warm->dynamic));
   assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,fixture->rope,
    fixture->routes,100,count,*plan));
   assert(DataflowStreamRecording::Build(fixture->packed,*candidate,*next,*plan,fixture->routes,
    1,43,0,true,*warm,*record,*projection,stage));
   assert(warm->sealed && warm->pairs==8 && warm->packKey==8 && warm->packValue==8
    && warm->attention==16 && candidate->GraphStateMatches(prior));
  }
 }
 {
  auto live=std::make_unique<DataflowProjectionAuthority>();auto candidate=std::make_unique<DataflowProjectionAuthority>();
  auto plan=std::make_unique<DataflowStreamPlan>();auto record=std::make_unique<DataflowStreamRecord>();
  auto projection=std::make_unique<DataflowProjectionProgram>();auto sink=std::make_unique<DensePairSink>(*fixture);unsigned stage=0;
  assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
  DataflowGraphAuthorityState before{};assert(live->CaptureGraphState(before));sink->failPair=true;
  assert(DataflowDynamicTokenContext::Build(fixture->manifest,43,1,sink->dynamic));
  assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,fixture->rope,
   fixture->routes,101,DataflowGraphDecodeSteps,*plan));
  assert(!DataflowStreamRecording::Build(fixture->packed,*live,*candidate,*plan,fixture->routes,
   0,42,0,true,*sink,*record,*projection,stage));
  assert(sink->aborted && !sink->sealed && live->GraphStateMatches(before));
 }
 std::puts("PASS eight paired K/V projections use 120 workers with disjoint scratch; SIMULATED ONLY");
}
