/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FACTORED_OPERATOR_FIXTURE_ONLY
#include "pioneer_q38_q4_factored_operator_test.cpp"
#define PIONEER_Q38_Q4_FACTORED_TRACE_DIAGNOSTIC 1
#include "../PioneerQ38Q4FactoredTrace.h"
#include "../PioneerQ38Q4FactoredTraceReference.h"
struct TraceIssue {
 FactoredOperatorMachine& m;std::vector<unsigned> sequence;
 void Op(unsigned op){sequence.insert(sequence.end(),{1,op});m.Op(op);}
 void Config(unsigned i,unsigned v){sequence.insert(sequence.end(),{2,i,v});m.Config(i,v);}
 void Drain(){sequence.push_back(3);m.Drain();}
};
struct TraceIO {
 FactoredOperatorMachine& m;unsigned reads=0,writes=0;std::array<unsigned,64> output{};
 unsigned Read(unsigned a){
  assert(m.Clean());reads++;
  if(a>=Tile::Native-16 && a<Tile::Native+Tile::TileBytes){assert(!(a&3));return unsigned(m.l1[a])|(unsigned(m.l1[a+1])<<8)|(unsigned(m.l1[a+2])<<16)|(unsigned(m.l1[a+3])<<24);}
  assert(a>=0xffbd8000 && a<0xffbd8000+256*64 && !(a&3));
  const unsigned relative=a-0xffbd8000,row=relative/64,column=relative%64/4;
  assert(row==32 || row==40 || row==80 || row==88 || row>=128);
  return m.FloatDst(row,column);
 }
 void Write(unsigned a,unsigned v){assert(a>=0x98000 && a<0x98100 && !(a&3));assert((a-0x98000)/4==writes);output[writes++]=v;}
};
int main(){
 auto traced=std::make_unique<FactoredOperatorMachine>(),control=std::make_unique<FactoredOperatorMachine>();
 TraceIssue e{*traced,{}},old{*control,{}};TraceIO io{*traced};
 PioneerQ38Q4FactoredTrace trace;PioneerQ38Q4FactoredTraceReference expected;
 Compute::Constants(*traced);Compute::Begin(e);Compute::Constants(*control);Compute::Begin(old);
 assert(!trace.Publish(io) && !trace.EndBlock(0) && !io.reads && !io.writes);
 for(unsigned block=0;block<20;block++){
  TileFixture data(block+3);const unsigned buffer=block&1;
  StageOperator(data,*traced,buffer);StageOperator(data,*control,buffer);
  assert(Compute::LoadMetadata(e,buffer) && Compute::LoadMetadata(old,buffer));
  assert(!trace.Group(e,io,block,buffer^1,0) && !trace.Group(e,io,block+1,buffer,0));
  for(unsigned group=0;group<8;group++){
   const auto reads=io.reads;const auto ops=e.sequence.size();
   assert(!trace.Group(e,io,block,buffer,8) && !trace.Group(e,io,block,buffer,(group+1)%8));
   assert(io.reads==reads && e.sequence.size()==ops);
   assert(trace.Group(e,io,block,buffer,group) && Compute::Group(old,buffer,group));
   assert(e.sequence==old.sequence);
  }
  assert(trace.EndBlock(block) && !trace.EndBlock(block));
  assert(expected.Block(data.original.data(),data.input.data(),block));
 }
 std::array<unsigned,64> reference{};assert(expected.Ready() && expected.Export(reference.data()));
 assert(trace.Publish(io) && io.writes==64 && io.output==reference);
 const auto reads=io.reads;assert(!trace.Publish(io) && !trace.Group(e,io,20,0,0) && io.reads==reads);
 assert(traced->Results()==control->Results());
 printf("PASS_FACTORED_TRACE K5120 groups160 final32_exact block20_stage4_first8 independentOriginalReference64 exactProductionIssueStream opConfigDrain%zu diagnosticReads%u noPrematureReads invalidOrderZeroIO singlePublish;LOCAL_NOT_HARDWARE_OWNER_OR_SPEED\n",e.sequence.size(),io.reads);
 static_cast<void>(&Round);static_cast<void>(&ReferenceGroup);
}
