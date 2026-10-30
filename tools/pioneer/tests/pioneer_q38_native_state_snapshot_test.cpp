/* SPDX-License-Identifier: MIT */
// Synthetic COMPONENT bytes ONLY. This cannot grant an owner,model state,
// processor history,terminal/END result or original-model numerical proof.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#define PIONEER_Q38_DECODER_RUNTIME 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvHistoryBanks.h"
#include "pioneer_q38_state_snapshot_data.h"
#include <memory>
#include <map>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#define PIONEER_Q38_TOKEN_NATIVE_CONTEXT 1
#define PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE 1
#define PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE 1
#define PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE 1
#define PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY 1
#define PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE 1
#define PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE 1
#include "pioneer_q38_native_state_test_bridge.h"
struct PioneerSessionIO {
 PioneerQ38GdnRegion recurrent[672]={};PioneerQ38ConvHistoryExtent history[336]={};unsigned recurrentFirst[672]={};
 std::vector<uint32> recurrentWords=std::vector<uint32>(73728*1024),historyWords=std::vector<uint32>(1680*1024);
 std::map<uint64,uint32> kvWords;std::map<uint64,unsigned> kvSpans;
};
#include "pioneer_q38_token_native_state_snapshot.inc"
static uint32 Pattern(unsigned completed,unsigned layer,unsigned kind,unsigned i){
 if(kind<2)return 0x3f000000U|((completed*7919+layer*271+kind*65537+i*17)&0x7fffff);
 return (0x3000U|((layer*13+kind*67+i*7)&1023))|((0x3400U|((layer*19+kind*73+i*11)&1023))<<16);
}
int main(int argc,char** argv){
 assert(argc==5);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;(void)tokenNativeStateSnapshotDirectory;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());auto bytes=std::make_unique<PioneerSessionIO>();auto& io=*bytes;
 unsigned pages=0;
 for(unsigned i=0;i<672;i++){
  const auto& r=plan->RecurrentRegions()[i];io.recurrent[i]=r;io.recurrent[i].offset+=plan->Base;io.recurrentFirst[i]=pages;
  // Seed physical half-head order from independent LOGICAL key/value
  // coordinates. A linear-copy capture must now fail the CPU comparison.
  for(unsigned h=0;h<r.heads;h++)for(unsigned half=0;half<2;half++)for(unsigned key=0;key<128;key++)for(unsigned value=0;value<64;value++)
   io.recurrentWords[pages*1024+h*16384+half*8192+key*64+value]=Pattern(r.slot?3:2,r.layer,0,(r.firstHead+h)*16384+key*128+half*64+value);
  pages+=r.heads*16;
 }assert(pages==73728);
 assert(io.recurrentWords[64]==Pattern(2,0,0,128) && io.recurrentWords[64]!=Pattern(2,0,0,64));
 unsigned history=0;
 for(unsigned i=0;i<plan->Count();i++){
  const auto& p=*plan->Part(i);
  if(p.kind==plan->ConvKind){
   io.history[history]={p.bank,plan->Base+p.offset,PioneerQ38ProjectionRoute(p.bank),0,p.allocated/4096,p.allocated};
   for(unsigned j=0;j<p.rows*3;j++)io.historyWords[history*5120+j]=Pattern(3,p.layer,1,p.firstRow*3+j);history++;
  }else if(p.kind==plan->KvKind){
   assert(io.kvSpans.emplace(uint64(p.bank)<<32|p.offset,p.allocated).second);
   for(unsigned row=0;row<p.rows;row++)for(unsigned pos=0;pos<3;pos++)for(unsigned word=0;word<128;word++){
    const unsigned logical=p.firstRow+row,address=plan->Base+p.offset+row*p.rowBytes+pos*512+word*4;
    assert(io.kvWords.emplace(uint64(PioneerQ38ProjectionRoute(p.bank))<<32|address,
     Pattern(3,p.layer,logical<4?2:3,pos*512+(logical%4)*128+word)).second);
   }
  }
 }assert(history==336 && io.kvWords.size()==49152);
 unsigned refused=0;
 for(unsigned fault=0;fault<7;fault++){
  const auto saved=io.recurrent[671];const unsigned first=io.recurrentFirst[671];
  uint32* fields=reinterpret_cast<uint32*>(&io.recurrent[671]);if(fault<6)fields[fault]^=1;else io.recurrentFirst[671]++;
  Q38NativeStateTest bad;assert(!CaptureNativeStateSnapshots(io,*plan,bad,argv[3]));refused++;
  io.recurrent[671]=saved;io.recurrentFirst[671]=first;
 }
 Q38NativeStateTest native;assert(CaptureNativeStateSnapshots(io,*plan,native,argv[2]) && native.Finish(argv[2]) && !native.Finish(argv[2]));
 unsigned records=0;
 for(unsigned completed=2;completed<=3;completed++)for(unsigned layer=0;layer<64;layer++)for(unsigned kind=0;kind<4;kind++){
  unsigned count=Q38StateSnapshotData::Count(completed,layer,kind);if(!count)continue;std::vector<uint32> words(count);
  for(unsigned i=0;i<count;i++)words[i]=Pattern(completed,layer,kind,i);
  assert(Q38StateSnapshotData::Save(argv[1],false,completed,layer,kind,words.data(),words.size()));records++;
  // Reproduce the old v1 collector's literal physical-head copies,not the
  // inverse of the new Python decoder. This is SYNTHETIC raw DATA only.
  auto legacy=words;
  if(!kind)for(unsigned r=0;r<672;r++){
   const auto& region=io.recurrent[r];if(region.layer!=layer || region.slot!=(completed&1))continue;
   for(unsigned head=region.firstHead;head<region.firstHead+region.heads;head++){
    const auto* begin=io.recurrentWords.data()+io.recurrentFirst[r]*1024+(head-region.firstHead)*16384;
    std::copy(begin,begin+16384,legacy.begin()+head*16384);
   }
  }
  const uint32_t oldHeader[16]={Q38StateSnapshotData::NativeMagic,1,completed,layer,kind,count};
  assert(Q38StateSnapshotData::Write(argv[4],Q38StateSnapshotData::Name(completed,layer,kind),oldHeader,legacy.data(),legacy.size()));
 }
 assert(records==176 && Q38StateSnapshotData::Finish(argv[1],false,records));
 const uint32_t oldMarker[16]={Q38StateSnapshotData::NativeMagic,1,176,3,64};
 assert(Q38StateSnapshotData::Write(argv[4],"state-complete.bin",oldMarker,nullptr,0));
 std::vector<uint32> small(1536,0x3c003800);
 for(unsigned fault=0;fault<7;fault++){
  Q38NativeStateTest bad;auto words=small;unsigned completed=3,layer=3,kind=2,count=1536;
  if(fault==0)completed=1;if(fault==1)layer=64;if(fault==2)kind=0;if(fault==3)count--;
  if(fault==4)words[100]=0x7c003800;if(fault==5)words[100]=0x3c007c00;
  assert(!bad.Record(argv[3],completed,layer,kind,fault==6?nullptr:words.data(),count) && !bad.Finish(argv[3]));refused++;
 }
 printf("PASS SYNTHETIC state DATA component/176 striped GDN+Conv+packedKV records/new nativev2logical+old nativev1physical/independent logical-coordinate patterns/%u geometry+identity+nonfinite+null refusals/one-shot marker;NOT original model,executed generations,native END,physical,Work or TPS qualification\n",refused);
}
