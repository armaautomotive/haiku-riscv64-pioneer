/* SPDX-License-Identifier: MIT */
// Synthetic RAW operands+weights,not model products/native owners. The opaque
// engines are virgin and share actual byte endpoints from their construction.
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#include "pioneer_q38_native_peer_test_bridge.h"
#define PIONEER_Q38_NATIVE_PUSHED_FABRIC_TEST 1
#include "PioneerQ38NativePushedTestFabric.h"
#include "../PioneerQ38TokenSelectedPrograms.h"
#include "../PioneerQ38SelectedGlobalWorker.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramSamplesProbe.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <map>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
namespace W=PioneerQ38PushedWorkerLayout;
static_assert(PioneerQ38SelectedWorkerPushed==1 && PioneerQ38TokenPrograms::Family==0x33505451,"actual separate G3 DATA selection");
struct NativePushedBytes {
 static constexpr unsigned Base=0x04000000,Workspace=0x08000000;
 const unsigned worker,groups,type,inputs,block,tileBytes,scaleBytes;
 std::vector<uint32_t> root=std::vector<uint32_t>(0x180000/4),l1=root;
 std::array<uint32_t,16> source={};std::array<uint32_t,10> commands={};
 std::array<uint32_t,8> counters={},rootMaster={};
 std::array<uint32_t,64> boot={},job={};std::vector<uint8_t> weights;
 std::map<uint32_t,uint32_t> activationRaw;
 bool pending=false,injected=false,faultForked=false;unsigned fault=16,faultCases=0,events=0,reads=0,nonce=0,layers=0;
 Q38NativePushedTestFabric incoming;
 static uint32_t& Word(void* c,unsigned w,uint32_t address){
  auto& m=*static_cast<NativePushedBytes*>(c);assert((!w || w==m.worker) && !(address&3) && address<0x180000);
  return (w?m.l1:m.root).at(address/4);
 }
 NativePushedBytes(unsigned w,unsigned g,unsigned format):worker(w),groups(g),type(format),
  inputs(format==12?17408:format==14?6144:5120),block(format==8?32:256),
  tileBytes(PioneerQ38PanelTileBytes(format)),scaleBytes((inputs/block*4+63)&~63U),incoming(this,Word){
  assert(w && w<=119 && g && g<=7);
  assert(type==8 || type==12 || type==14);
  for(unsigned i=0;i<root.size();i++){root[i]=0x1234579U*(i+1);l1[i]=0xa53179U*(i+1)^w;}
  for(unsigned i=0;i<16;i++)source[i]=i?0x12340000+i+w*0x100:0x16005;
  commands[5]=W::Peer(w);counters.fill(0xfffffff8U);rootMaster.fill(0xfffffff0U);
 }
 Q38NativePeerTestBytes Endpoints(){
  Q38NativePeerTestBytes b;b.context=this;b.word=Word;
  b.noc=[](void* c,unsigned w,unsigned offset){
   auto& m=*static_cast<NativePushedBytes*>(c);assert(w==m.worker);
   if(offset<40 && !(offset&3))return m.commands[offset/4];
   for(unsigned i=0;i<16;i++)if(offset==BlackholeDramSourceOffset(i))return m.source[i];
   const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(offset==offsets[i])return m.counters[i];
   if(offset==0x40)return unsigned(m.pending);
   if(offset==0x840 || offset==0x1040 || offset==0x1840 || (offset>=0x240 && offset<0x2c0))return 0U;
   assert(false);return UINT32_MAX;
  };
  b.events=[](void* c,unsigned w,const uint32_t* delta,const uint32_t* command,unsigned mask,bool changed,bool pending){
   auto& m=*static_cast<NativePushedBytes*>(c);assert(w==m.worker && delta[2]==delta[6] && !(mask&~1023U));m.events++;
   for(unsigned i=0;i<8;i++)m.counters[i]+=delta[i];
   for(unsigned i=0;i<10;i++)if(mask&(1U<<i))m.commands[i]=command[i];
   if(changed)m.pending=pending;
  }; // DELTAS only;an unrelated raw physical fault is never repaired.
  b.read=[](void* c,unsigned route,uint32_t address,uint8_t* out,unsigned bytes){
   auto& m=*static_cast<NativePushedBytes*>(c);assert(out && bytes && bytes<=16384 && !((address|bytes)&3));m.reads++;
   if(route==PioneerQ38ProjectionRoute(0) && m.activationRaw.count(address)){
    for(unsigned i=0;i<bytes/4;i++){const auto value=m.activationRaw.at(address+i*4);for(unsigned j=0;j<4;j++)out[i*4+j]=uint8_t(value>>(j*8));}return true;
   }
   if(route==PioneerQ38ProjectionRoute(0) && address>=Base && address-Base<=m.weights.size()
    && bytes<=m.weights.size()-(address-Base)){memcpy(out,m.weights.data()+address-Base,bytes);return true;}
   if(route==W::Root && ((address>=0xa0000 && address-0xa0000<=m.inputs*2 && bytes<=m.inputs*2-(address-0xa0000))
    || (address>=0xab000 && address-0xab000<=m.scaleBytes && bytes<=m.scaleBytes-(address-0xab000)))){
    for(unsigned i=0;i<bytes/4;i++){const auto value=m.root.at(address/4+i);for(unsigned j=0;j<4;j++)out[i*4+j]=uint8_t(value>>(j*8));}return true;
   }
   return false;
  };
  b.put=[](void* c,unsigned route,uint32_t address,uint32_t value){
   auto& m=*static_cast<NativePushedBytes*>(c);
   assert(m.type==12 && m.worker<=68 && m.job[2]==W::ActivationOperation && route==PioneerQ38ProjectionRoute(0)
    && !(address&3) && ((address>=Workspace+W::NativeOffset && address-Workspace-W::NativeOffset<512)
     || (address>=Workspace+W::LinesOffset && address-Workspace-W::LinesOffset<128)));
   m.activationRaw[address]=value;return true;
  }; // only actual peer-produced activation writes;no host/model product upload
  b.clock=[](void*){return UINT64_C(1)<<40;};
  b.push=[](void* c,unsigned w,const uint32_t* packet,const uint32_t* raw){
   auto& m=*static_cast<NativePushedBytes*>(c);assert(w==m.worker);m.incoming.Incoming(w,packet,raw);
   if(m.worker==1 && m.type==8 && !m.faultForked){
    assert(m.nonce==1 && m.incoming.messages==1 && !m.root.at(W::Slot(w)/4));m.faultForked=true;
    for(unsigned i=0;i<16;i++){
     fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
     if(!pid){m.injected=true;m.fault=i;m.source[i]^=1U<<((i*7+5)%32);return;}
     int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));m.faultCases++;
    }
    assert(m.faultCases==16 && !m.injected && m.incoming.messages==1);
   }
  };return b;
 }
 void Install(){
  namespace P=PioneerQ38TokenPrograms;
  for(unsigned i=0;i<P::CodeWords(worker);i++){uint32_t value;const auto address=P::CodeAddress(worker,i);
   assert(P::ProgramWord(worker,address,value));l1.at(address/4)=value;}
  const uint32_t head[]={W::Magic,1,64,worker,W::Peer(worker),source[0],8192,64};
  for(unsigned i=0;i<8;i++)boot[i]=head[i];for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
  for(unsigned i=0;i<16;i++)boot[16+i]=source[i];for(unsigned i=40;i<55;i++)boot[i]=W::BootWord(i);
  assert(W::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++)l1.at((W::Boot+i*4)/4)=boot[i];
  l1.at(W::Doorbell/4)=0;for(unsigned i=0;i<16;i++)l1.at((W::Status+i*4)/4)=0;
 } // component raw code/boot DATA only,not a native held-image owner
 static unsigned Operand(unsigned i,unsigned sequence){return 1+(i*3+sequence)%7;}
 static unsigned Weight(unsigned tile,unsigned lane,unsigned row,unsigned sequence){return 1+(tile+lane*3+row+sequence)%9;}
 unsigned InputOperand(unsigned i,unsigned sequence)const{return type==12 && worker<=68 && i<256?127U:Operand(i,sequence);}
 void StageActivation(){
  assert(type==12 && worker==1 && !pending);nonce++;job.fill(0);std::array<uint32_t,64> descriptor={};
  const unsigned epoch=2025+nonce;
  const uint32_t head[]={W::Magic,nonce,W::ActivationOperation,worker,W::Peer(worker),layers,0,epoch};
  for(unsigned i=0;i<8;i++){job[i]=head[i];job[8+i]=boot[8+i];}
  job[16]=W::ActivationMagic;job[17]=256;job[18]=576;job[19]=epoch;job[20]=12;job[27]=248319;
  const uint32_t fields[]={W::ActivationMagic,1,67108864,0,Workspace,0,epoch,worker,PioneerQ38ProjectionRoute(0),W::Peer(worker),
   W::GateOffset,W::UpOffset,W::NativeOffset,W::LinesOffset,12,W::Chunks};
  for(unsigned i=0;i<16;i++){descriptor[i]=fields[i];descriptor[16+i]=source[i];}
  for(unsigned i=0;i<8;i++)descriptor[32+i]=boot[8+i];
  descriptor[40]=0;descriptor[41]=248319;descriptor[42]=layers;descriptor[43]=4;
  assert(W::DescriptorShape(descriptor.data()) && W::JobShape(job.data(),boot.data(),nonce-1,0));
  for(unsigned i=0;i<256;i++){
   activationRaw[Workspace+W::GateOffset+i*4]=0x42fe0000; // raw gate127,up1 -> pinned Q8_K signed scale-1/int-127
   activationRaw[Workspace+W::UpOffset+i*4]=0x3f800000;
  }
  for(unsigned i=0;i<64;i++){l1.at((W::Descriptor+i*4)/4)=descriptor[i];l1.at((W::Job+i*4)/4)=job[i];}
  l1.at(W::Doorbell/4)=nonce;
 } // exact raw descriptor/input bytes ONLY;the SAME engine must execute activation itself
 void Stage(unsigned sequence,bool stop=false){
  assert(sequence==nonce+1 && !pending);nonce=sequence;job.fill(0);
  const uint32_t head[]={W::Magic,sequence,stop?unsigned(W::Stop):1U,worker,W::Peer(worker),layers,0,2025+sequence};
  for(unsigned i=0;i<8;i++){job[i]=head[i];job[8+i]=boot[8+i];}
  if(!stop){
   weights.assign(groups*(inputs/block)*tileBytes,0);
   for(unsigned t=0;t<groups*(inputs/block);t++)for(unsigned r=0;r<16;r++){
    auto* tile=weights.data()+t*tileBytes;const unsigned half=type==14?208U:0U;
    tile[(half+1)*16+r]=0x3c; // FP16 coefficient1,not an output product
    if(type==12)for(unsigned i=0;i<4;i++){tile[(4+i)*16+r]=1;tile[(12+i)*16+r]=1;}
    if(type==14)for(unsigned i=0;i<16;i++)tile[(192+i)*16+r]=1;
    for(unsigned k=0;k<block;k++){
     const unsigned value=Weight(t,k,r,sequence);
     if(type==8)tile[(2+k)*16+r]=uint8_t(value);
     else if(type==12)tile[(16+(k/64)*32+k%32)*16+r]|=uint8_t(value<<((k/32%2)*4));
     else{
      const unsigned halfIndex=k/128,plane=(k%128)/32,code=32+value;
      tile[(halfIndex*64+(plane&1)*32+k%32)*16+r]|=uint8_t((code&15)<<((plane/2)*4));
      tile[(128+halfIndex*32+k%32)*16+r]|=uint8_t((code>>4)<<(plane*2));
     }
    }
   }
   for(unsigned i=0;i<inputs/2;i++)root.at(0xa0000/4+i)=PioneerInt8Operand(int(Operand(i*2,sequence)))
    |uint32_t(PioneerInt8Operand(int(Operand(i*2+1,sequence))))<<16;
   for(unsigned i=0;i<scaleBytes/4;i++)root.at(0xab000/4+i)=i<inputs/block?0x3f800000U:0U;
   if(type==12 && worker<=68){
    printf("OBSERVED actual activation output/nativeWord%08x/scale%08x/activated%08x;NO result supplied\n",
     activationRaw.at(Workspace+W::NativeOffset),activationRaw.at(Workspace+W::LinesOffset),l1.at(0x90800/4));
    for(unsigned i=0;i<128;i++){
     const auto produced=activationRaw.at(Workspace+W::NativeOffset+i*4);
     assert(produced==(uint32_t(PioneerInt8Operand(-127))|uint32_t(PioneerInt8Operand(-127))<<16));
     root.at(0xa0000/4+i)=produced;
    }
    assert(activationRaw.at(Workspace+W::LinesOffset)==0xbf800000);root.at(0xab000/4)=activationRaw.at(Workspace+W::LinesOffset);
   } // byte-only transport of actual activation output into the following matrix input
   const unsigned matrix[]={0x31483851,type,tileBytes,0,0,inputs,Base,groups,PioneerQ38ProjectionRoute(0),1};
   for(unsigned i=0;i<10;i++)job[16+i]=matrix[i];job[26]=1;
   for(unsigned i=0;i<64;i++)incoming.Store(W::Slot(worker)+i*4,0);
   assert(!incoming.Read(W::Slot(worker))); // make a genuinely stale cached zero BEFORE remote delivery
  }
  assert(W::JobShape(job.data(),boot.data(),sequence-1,0));
  for(unsigned i=0;i<64;i++)l1.at((W::Job+i*4)/4)=job[i];l1.at(W::Doorbell/4)=sequence;
 }
 void Fault(Q38NativePeerTestCpu& cpu){
  assert(injected && fault<16 && cpu.ObserveFaulted() && incoming.messages==1 && !pending
   && !root.at(W::Slot(worker)/4) && !l1.at((W::Status+4)/4) && rootMaster[0]==0xfffffff0U);
  const auto raw=l1;const auto remote=root;const auto physical=counters;const auto command=commands;const auto slave=incoming.slave;
  const unsigned touches=events,readsBefore=reads,steps=cpu.Stats().steps;
  cpu.AdvanceFaulted(2048);
  assert(cpu.Stats().steps==steps+2048 && cpu.ObserveFaulted() && raw==l1 && remote==root && physical==counters
   && command==commands && slave==incoming.slave && touches==events && readsBefore==reads && incoming.messages==1);
  printf("PASS shared-byte G3/source%u corrupted AFTER actual payload ACK/actual sender refuses commit+DONE/inert2048/no following packet;NOT native ownership/model/physical/Work/TPS\n",fault);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned worker:{1U,96U,119U}){
  const unsigned groups=worker==1?1U:worker==96?3U:7U;NativePushedBytes bytes(worker,groups,type);
  auto endpoints=bytes.Endpoints();const auto cold=bytes.l1,root=bytes.root;
  Q38NativePeerTestCpu cpu(argv[1],argv[2],worker,endpoints);
  assert(cold==bytes.l1 && root==bytes.root && !cpu.Stats().steps && cpu.Stats().pc==0x6000);bytes.Install();
  assert(!cpu.Stack().instructions && cpu.Stack().minimumSp==0x70000);
  auto idle=[&]{const auto s=cpu.Stats();return s.pc==0x2004c || s.pc==0x20050;};
  for(unsigned n=0;n<4096 && !idle();n++)cpu.Advance(256);
  assert(idle() && !cpu.Stats().sequence && !cpu.Stats().requests && !bytes.incoming.messages);
  const auto master=bytes.rootMaster;const auto initialSlave=bytes.incoming.slave;
  std::vector<uint32_t> previous;
  for(unsigned sequence=1;sequence<=2;sequence++){
   if(type==12 && worker<=68){
    const auto before=bytes.counters;const unsigned messages=bytes.incoming.messages;bytes.StageActivation();
    while(!cpu.ObserveFaulted() && !(cpu.Stats().sequence==bytes.nonce && idle() && !cpu.Stats().active
     && bytes.l1.at((W::Status+4)/4)==bytes.nonce))cpu.Advance(256);
    const unsigned delta[]={2,32,4,2,2,10,4,2};
    assert(!cpu.ObserveFaulted() && !cpu.Stats().pending && bytes.incoming.messages==messages);
    for(unsigned i=0;i<8;i++)assert(bytes.counters[i]==before[i]+delta[i]);
    printf("PASS shared-byte G3/worker%u/actual activation nonce%u/full256 gate+up/SILU+native packing+scale writes/BEFORE down projection/SAME engine;NOT all68/model/ROOT/native ownership\n",worker,bytes.nonce);
   }
   printf("BEGIN shared-byte G3/type%u/worker%u/job%u/K%u/groups%u;SYNTHETIC component only\n",type,worker,sequence,bytes.inputs,groups);
   const auto before=bytes.counters;bytes.Stage(bytes.nonce+1);
   while(!cpu.ObserveFaulted() && !(cpu.Stats().sequence==bytes.nonce && cpu.ObserveMatrixOutput()))cpu.Advance(256);
   if(bytes.injected){bytes.Fault(cpu);return 0;}
   const auto stats=cpu.Stats();const unsigned inputs=(bytes.inputs*2+16383)/16384,reads=inputs+1+groups*(bytes.inputs/bytes.block);
   printf("OBSERVED shared-byte G3/type%u/worker%u/job%u/pc%x/fault%u/status%x/sequence%u/active%u/pending%u/tensor%u/requests%u/responses%u/messages%u;NO completion supplied\n",
    type,worker,sequence,stats.pc,unsigned(cpu.ObserveFaulted()),bytes.l1.at((W::Status+8)/4),stats.sequence,stats.active,stats.pending,
    stats.matrixInstructions,stats.requests,stats.responses,bytes.incoming.messages);
   assert(!cpu.ObserveFaulted() && stats.matrixInstructions==groups*(type==12?2U:4U)*(bytes.inputs/16) && stats.requests==reads && stats.responses==reads
    && !stats.pending && !stats.active && stats.minStack>=0x6e000 && bytes.incoming.messages==sequence*2 && bytes.rootMaster==master);
   const auto stack=cpu.Stack();assert(stack.instructions==stats.totalSteps && stack.minimumSp>=0x6e000
    && stack.minimumSp<0x70000 && stack.minimumAccessed==stats.minStack);
   assert(bytes.root.at(W::Slot(worker)/4)==W::ReceiptMagic && !bytes.incoming.Read(W::Slot(worker)) && bytes.incoming.stale);
   bytes.incoming.Fence();std::array<uint32_t,64> receipt={};
   for(unsigned i=0;i<64;i++)receipt[i]=bytes.incoming.Read(W::Slot(worker)+i*4);
   assert(W::Receipt(receipt.data(),bytes.boot.data(),bytes.job.data(),receipt.data()+56));
   const unsigned readFlits=bytes.inputs*2/64+bytes.scaleBytes/64+groups*(bytes.inputs/bytes.block)*(bytes.tileBytes/64);
   const unsigned prePush[]={reads,readFlits,reads,reads,0,0,reads,0},push[]={0,0,2,0,2,5,2,2};
   for(unsigned i=0;i<8;i++)assert(receipt[56+i]==before[i]+prePush[i] && bytes.counters[i]==before[i]+prePush[i]+push[i]);
   const unsigned slaveDelta[]={2,5,2,2};for(unsigned i=0;i<4;i++)assert(bytes.incoming.slave[i]==initialSlave[i]+sequence*slaveDelta[i]);
   std::vector<uint32_t> actual;
   for(unsigned group=0;group<groups;group++)for(unsigned row=0;row<16;row++){
    unsigned sum=0;for(unsigned i=0;i<bytes.inputs;i++)sum+=bytes.InputOperand(i,bytes.nonce)
     *NativePushedBytes::Weight(group*(bytes.inputs/bytes.block)+i/bytes.block,i%bytes.block,row,bytes.nonce);
    const float expected=float(sum);uint32_t bits;memcpy(&bits,&expected,4);
    actual.push_back(bytes.l1.at(0x84000/4+group*16+row));assert(actual.back()==bits);
   } // independent POSTHOC arithmetic ONLY;no result is ever supplied to the engines
   if(sequence==2)assert(actual!=previous);previous=actual;
   if(type==12)bytes.layers++; // HOST observer/stager follows the actual returned down job;no processor state setter
   printf("PASS shared-byte G3/type%u/worker%u/job%u/K%u/groups%u/actual%u tensor instructions/%u read requests/two actual packet deliveries/fenced receipt+all8 counter frontiers/exact independent rows/SAME virgin engine;NOT native ownership/model/physical/Work/TPS\n",type,worker,sequence,bytes.inputs,groups,stats.matrixInstructions,stats.requests);
  }
  if(worker==1 && type==8)assert(bytes.faultCases==16);
  bytes.Stage(bytes.nonce+1,true);while(!cpu.ObserveStopped() && !cpu.ObserveFaulted())cpu.Advance(256);
  assert(cpu.ObserveStopped() && bytes.incoming.messages==4 && bytes.rootMaster==master);
  const auto before=cpu.Stats();const auto commands=bytes.commands;const auto counts=bytes.counters;const unsigned events=bytes.events;
  cpu.AdvanceStopped(2048);assert(cpu.Stats().steps==before.steps+2048 && commands==bytes.commands && counts==bytes.counters && events==bytes.events);
  const auto stack=cpu.Stack();assert(stack.instructions==cpu.Stats().totalSteps && stack.minimumSp>=0x6e000);
  printf("OBSERVED actual component worker%u/type%u/reserved stack%u/accessed%u/8192/instructions%llu incl genuine STOP2048;NOT whole-model or kernel-stack proof\n",
   worker,type,0x70000-stack.minimumSp,0x70000-stack.minimumAccessed,(unsigned long long)stack.instructions);
 }
 puts("PASS G3 opaque native TEST bridge/nine virgin shared-byte engines/types8+12+14/fullK5120+17408+6144/two changed-input jobs EACH/groups1+3+7/two actual worker1 activation predecessors+produced chunk consumption/16 actual post-payload source fault forks/ACK-ordered raw payload+commit/L0 stale until explicit TEST fence/root master unchanged/counter wrap/actual early component STOP+2048 each;SYNTHETIC raw operands,NOT ROOT instructions/all68 activation/native held lifecycle/original model/119STOP/END/physical/Work/TPS qualification");
}
