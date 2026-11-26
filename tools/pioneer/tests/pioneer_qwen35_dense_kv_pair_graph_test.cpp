/* SPDX-License-Identifier: MIT */
// Offline fused graph recorder: pair K and V on disjoint output scratch.
#include <algorithm>
#include <array>
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
#include "../PioneerQwen35DataflowBlackholeDenseAttentionWorker.h"
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
    auto invalidAlias=inv.program;invalidAlias.gate=invalidAlias.contextOutput;assert(!invalidAlias.ShapeValid());
    ++packKey;
   } else if (inv.program.mode==DataflowDenseAttentionPackValue) {
    assert(DataflowRowSpaceMatches(inv.program.gate,fixture.workspace.space[DataflowGraphUpWorkspaceIndex]));
    assert(DataflowRowSpaceMatches(inv.program.gate,inv.program.contextOutput));++packValue;
    auto invalidSource=inv.program;invalidSource.gate=fixture.workspace.space[DataflowGraphRecurrentGateWorkspaceIndex];
    assert(!invalidSource.ShapeValid());
   } else if (inv.program.mode==DataflowDenseAttentionRun) {
    auto invalidAlias=inv.program;invalidAlias.gate=invalidAlias.contextOutput;assert(!invalidAlias.ShapeValid());
    ++attention;
   }
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

struct DenseValuePackIO {
 const DataflowDenseAttentionInvocation* invocation=nullptr;
 std::array<float,DenseHeadWidth> projected{};
 std::array<uint32_t,DenseHeadWidth/2> packed{};
 unsigned reads=0,writes=0;
 bool Bind(const DataflowDenseAttentionInvocation& value) {
  invocation=&value;
  return value.program.mode==DataflowDenseAttentionPackValue
   && DataflowRowSpaceMatches(value.program.gate,value.program.contextOutput);
 }
 bool ReadProjectedRows(unsigned first,float* output) {
  if (!invocation || first!=0 || !output) return false;
  std::copy(projected.begin(),projected.end(),output);++reads;return true;
 }
 bool ReadNorm(bool,float*) {return false;}
 bool ReadRope(float*) {return false;}
 bool WriteKv(unsigned head,bool value,const uint32_t* source) {
  if (!invocation || head!=invocation->headCount-1 || !value || !source) return false;
  std::copy(source,source+packed.size(),packed.begin());++writes;return true;
 }
 bool ReadKvTile(unsigned,bool,unsigned,unsigned,float*) {return false;}
 bool WriteContext(unsigned,const float*) {return false;}
};

int main() {
 (void)&q35_dense_pair_fixture_main;
 static_assert(DataflowStreamVersion==7 && DataflowGraphDecodeSteps==428 && DataflowGraphPromptSteps==425,
  "dense K/V pairing has its own stream version and removes one step per dense layer");
 auto fixture=std::make_unique<Fixture>();
 {
  unsigned layer=0;while (layer<Layers && !Dense(layer)) ++layer;assert(layer<Layers);
  DataflowDenseAttentionProgram program{};
  assert(DataflowDenseAttentionProgramBuilder::Build(fixture->manifest,fixture->workspace,
   fixture->rope,layer,0,DataflowDenseAttentionPackValue,fixture->routes,program));
  DataflowDenseAttentionInvocation invocation{};
  assert(DataflowDenseAttentionProgramBuilder::BindCore(program,0,invocation));
  DenseValuePackIO io;
  for (unsigned lane=0;lane<DenseHeadWidth;lane++) io.projected[lane]=float(int(lane)-64)*0.125f;
  alignas(64) float tile[DenseHeadWidth],valueTile[DenseHeadWidth],query[DenseHeadWidth];
  alignas(64) float gate[DenseHeadWidth],norm[DenseHeadWidth],rope[Rotary],output[DenseHeadWidth];
  assert((PioneerQwen35DataflowDenseAttentionWorker(invocation,tile,valueTile,query,
   gate,norm,rope,output,io)));
  assert(io.reads==1 && io.writes==1);
  for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
   const unsigned actual=(io.packed[lane/2]>>((lane&1)*16))&0xffffU;
   assert(actual==PioneerAttentionToHalf(io.projected[lane]));
  }
 }
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
 std::puts("PASS paired K/V schedule, value-pack worker and workspace alias guards; SIMULATED ONLY");
}
