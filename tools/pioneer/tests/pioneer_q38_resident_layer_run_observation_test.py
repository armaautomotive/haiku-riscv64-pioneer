#!/usr/bin/env python3
"""CPU branch/observation test of extracted Run;NOT owner/native/device proof."""
import os
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[3]
KERNEL = ROOT / 'src/add-ons/kernel/drivers/misc'
owner = (KERNEL / 'PioneerQ38ResidentLayerOwner.h').read_text()

def method(text, signature):
    begin = text.index(signature)
    at = text.index('{', begin)
    depth = 1
    end = at + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[begin:end]

run = method(owner, ' template<class IO> bool Run(IO& io,PioneerQ38ResidentLayerResult& r)')
physical = method(owner, ' template<class IO> bool RootPhysical(IO& io)')
fail = method(owner, ' template<class IO> bool RunFail(IO& io,PioneerQ38ResidentLayerResult& r,')
proof = (KERNEL / 'PioneerQ38ResidentLayerProof.h').read_text()
predicates = proof[proof.index('static inline bool PioneerQ38ResidentLayerRootTerminalPc'):
                   proof.index('static inline unsigned PioneerQ38ResidentLayerSequence')]

# Frozen BEFORE-instrumentation methods from pushed a834ead18d. TEST ONLY.
# Deliberately no native owner,source,model,NoC or success capability here.
legacy = r'''
 template<class IO> bool RootPhysical(IO& io){
  uint32_t status[3],pc[2];for(unsigned i=0;i<3;i++)status[i]=io.RuntimeStatusRead(i);
  for(unsigned i=0;i<2;i++){io.Pause();pc[i]=io.RuntimePcRead();if(!PioneerQ38ResidentLayerRootTerminalPc(pc[i]))return false;}
  return PioneerQ38ResidentLayerRootPhysical(status,pc,io.ResetRead());
 }
 template<class IO> bool Run(IO& io,PioneerQ38ResidentLayerResult& r){
  if(!ready || unsafe || closed || ran || running[0] || !r.prepared || r.unsafe || r.closed || r.completed)return false;
  if(!Scope(io) || !operation.LearnedPagesPresent(ledger,PioneerResidentBanks::Pages))return Fail(io,r);
  if(!Guard(io,0) || !Budget(io,began))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  running[0]=cohort->rootRunning=cohort->worker[0].image.running=true;resets[0]=0x87ffff;r.phase=5;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  traffic=true;resets[0]=0x46800;io.RuntimeResetWrite(resets[0]);r.rootStarts++;
  bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0) || !Budget(io,began))return Fail(io,r);
   uint32_t marker=io.RuntimeStatusRead(2);
   if(marker && marker!=TT_Q38_RESIDENT_LAYER_MARKER)return Fail(io,r);
   if(marker){if(!RootReceipt(io,r))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!PioneerQ38ResidentLayerProgramImmutable(0,pc) || PioneerQ38ResidentLayerRootRejectedPc(pc))return Fail(io,r);
  }
  if(!done)return Fail(io,r);
  r.phase=6;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Budget(io,began) || !AcceptPeer(io,w,r))return Fail(io,r);}
  if(!Guard(io,0) || !RootPhysical(io) || !Budget(io,began) || !Hold(io,0))return Fail(io,r);
  r.rootHolds++;ran=true;history->residentLayerNextPublished=recurrent->residentLayerNextPublished=true;r.phase=7;return true;
 }
'''

fixture = r'''
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "PioneerQ38ResidentLayerFootprint.h"
#include "PioneerResidentBanks.h"
#include "PioneerQ38ResidentLayerRunObservation.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>
using Observation=PioneerQ38ResidentLayerRunObservation;
struct PioneerQ38ResidentLayerResult {
 uint32_t prepared=1,unsafe=0,closed=0,completed=0,phase=0,worker=0,rootStarts=0,rootHolds=0;
}; // TEST branch record ONLY;not the production result ABI
enum Event {ScopeEvent,LearnedEvent,GuardEvent,BudgetEvent,WriteStatus,ReadStatus,
 ResetWrite,PauseEvent,ReadPc,ReceiptEvent,AcceptEvent,HoldEvent,FailEvent,ReadReset,EventCount};
struct Config {
 unsigned failEvent=EventCount,failNth=0;
 unsigned initBad=3,physicalBad=3,markerMode=1;
 uint32_t runningPc=0xb8000,terminalPc=0x72038,reset=0x46800;
};
struct IO {
 Config config;std::vector<std::pair<unsigned,uint32_t>> events;
 unsigned counts[EventCount]={},markers=0;bool released=false,complete=false;
 explicit IO(Config c):config(c){}
 bool Note(Event e,uint32_t v=0){events.emplace_back(e,v);counts[e]++;return config.failEvent!=e || counts[e]!=config.failNth;}
 void RuntimeStatusWrite(unsigned i,uint32_t v){assert(v==0);Note(WriteStatus,i);}
 uint32_t RuntimeStatusRead(unsigned i){
  Note(ReadStatus,i);
  if(!released)return i==config.initBad?1:0;
  if(i<2)return i==config.physicalBad?1:i==0?0x7200c:5120;
  if(complete)return config.physicalBad==2?0:TT_Q38_RESIDENT_LAYER_MARKER;
  markers++;
  if(config.markerMode==2 || (config.markerMode==3 && markers>1))return 0xdeadbeef;
  if(!config.markerMode || markers==1)return 0;
  complete=true;return TT_Q38_RESIDENT_LAYER_MARKER;
 }
 void RuntimeResetWrite(uint32_t value){Note(ResetWrite,value);if(value==0x46800)released=true;}
 void Pause(){Note(PauseEvent);}
 uint32_t RuntimePcRead(){Note(ReadPc);return complete?config.terminalPc:config.runningPc;}
 uint32_t ResetRead(){Note(ReadReset);return config.reset;}
};
struct Common {
 IO& io;bool ready=true,unsafe=false,closed=false,ran=false,running[120]={},traffic=false;
 uint64_t began=0;uint32_t resets[120]={};const uint8_t* ledger=nullptr;
 struct Cohort {bool rootRunning=false;struct Worker {struct Image {bool running=false;} image;} worker[120];} value;
 Cohort* cohort=&value;
 struct State {bool residentLayerNextPublished=false;} h,g;
 State* history=&h;State* recurrent=&g;
 struct Operation {IO& io;bool LearnedPagesPresent(const uint8_t*,unsigned){return io.Note(LearnedEvent);}} operation;
 explicit Common(IO& x):io(x),operation{x}{}
 bool Scope(IO& x){return x.Note(ScopeEvent);}
 bool Guard(IO& x,unsigned w){return x.Note(GuardEvent,w);}
 bool Budget(IO& x,uint64_t){return x.Note(BudgetEvent);}
 bool RootReceipt(IO& x,PioneerQ38ResidentLayerResult&){return x.Note(ReceiptEvent);}
 bool AcceptPeer(IO& x,unsigned w,PioneerQ38ResidentLayerResult& r){if(!x.Note(AcceptEvent,w))return false;r.completed++;return true;}
 bool Hold(IO& x,unsigned w){if(!x.Note(HoldEvent,w))return false;running[w]=false;return true;}
 bool Fail(IO& x,PioneerQ38ResidentLayerResult& r){x.Note(FailEvent);unsafe=true;ready=false;r.unsafe=1;return false;}
}; // All stubs above are TEST branch endpoints,NOT native ownership/retention qualification.
'''

checks = r'''
unsigned cases=0;
void Compare(Config c){
 IO before(c),after(c);Legacy old(before);Current now(after);PioneerQ38ResidentLayerResult a,b;
 bool expected=old.Run(before,a),actual=now.Run(after,b);
 assert(expected==actual && before.events==after.events && !std::memcmp(&a,&b,sizeof(a)));
 assert(old.unsafe==now.unsafe && old.ready==now.ready && old.ran==now.ran && old.traffic==now.traffic);
 assert(old.h.residentLayerNextPublished==now.h.residentLayerNextPublished && old.g.residentLayerNextPublished==now.g.residentLayerNextPublished);
 if(!actual){
  assert(now.runObservation.failure!=Observation::None && after.events.back().first==FailEvent);
  auto saved=now.runObservation;size_t count=after.events.size();
  assert(!now.Run(after,b) && after.events.size()==count && !std::memcmp(&saved,&now.runObservation,sizeof(saved)));
 }else assert(now.runObservation.failure==Observation::None && now.runObservation.polls==2);
 cases++;
}
void Snapshot(uint32_t pc){
 Config c;c.runningPc=pc;
 IO before(c),after(c),ordinary(Config{});Legacy old(before);Current now(after),normal(ordinary);
 PioneerQ38ResidentLayerResult a,b,baseline;
 assert(!old.Run(before,a)); // documents the intentional former false rejection
 assert(now.Run(after,b) && normal.Run(ordinary,baseline));
 assert(after.events==ordinary.events && !std::memcmp(&b,&baseline,sizeof(b)));
 const auto& observed=now.runObservation;
 assert(observed.failure==Observation::None && observed.unexpectedPcCount==1
  && observed.firstUnexpectedPc==pc && observed.firstUnexpectedPoll==1
  && observed.outsideImageCount==unsigned(!PioneerQ38ResidentLayerProgramImmutable(0,pc))
  && observed.rejectedPcCount==unsigned(PioneerQ38ResidentLayerRootRejectedPc(pc)));
 cases++;
 // A snapshot NEVER substitutes for a later marker,receipt,peer,physical
 // terminal,guard,budget or HOLD. Each failed attempt is frozen/inert.
 for(unsigned bad=0;bad<8;bad++){
  c=Config{};c.runningPc=pc;
  if(!bad)c.markerMode=0; // persistent unexpected snapshots still exhaust
  if(bad==1)c.markerMode=3; // genuine fault marker AFTER one snapshot
  if(bad==2){c.failEvent=ReceiptEvent;c.failNth=1;}
  if(bad==3){c.failEvent=AcceptEvent;c.failNth=119;}
  if(bad==4)c.terminalPc=pc;
  if(bad==5){c.failEvent=HoldEvent;c.failNth=1;}
  if(bad==6){c.failEvent=GuardEvent;c.failNth=4;}
  if(bad==7){c.failEvent=BudgetEvent;c.failNth=3;}
  IO endpoint(c);Current value(endpoint);PioneerQ38ResidentLayerResult out;
  assert(!value.Run(endpoint,out) && out.unsafe && !out.rootHolds && !value.ran
   && !value.h.residentLayerNextPublished && !value.g.residentLayerNextPublished
   && value.runObservation.failure!=Observation::None);
  if(!bad)assert(value.runObservation.failure==Observation::PollsExhausted
   && value.runObservation.polls==65536 && value.runObservation.unexpectedPcCount==65536);
  auto frozen=value.runObservation;auto count=endpoint.events.size();
  assert(!value.Run(endpoint,out) && endpoint.events.size()==count
   && !std::memcmp(&frozen,&value.runObservation,sizeof(frozen)));
  cases++;
 }
}
int main(){
 IO baseline(Config{});Legacy old(baseline);PioneerQ38ResidentLayerResult r;assert(old.Run(baseline,r));Compare(Config{});
 for(unsigned e:{ScopeEvent,LearnedEvent,GuardEvent,BudgetEvent,ReceiptEvent,AcceptEvent,HoldEvent}){
  for(unsigned n=1;n<=baseline.counts[e];n++){Config c;c.failEvent=e;c.failNth=n;Compare(c);}
 }
 for(unsigned index=0;index<3;index++){Config c;c.initBad=index;Compare(c);c=Config{};c.physicalBad=index;Compare(c);}
 {Config c;c.markerMode=2;Compare(c);c=Config{};c.markerMode=0;Compare(c);}
 for(uint32_t pc:{0U,1U,0x58644U,0x72040U,0x72060U,0x7207cU,0x17e000U,0xdeadc0dcU})Snapshot(pc);
 {Config c;c.runningPc=0x1585ec;Compare(c);}
 for(uint32_t pc:{0U,0x72040U,0x72060U,0x72000U}){Config c;c.terminalPc=pc;Compare(c);}
 for(uint32_t reset:{0U,0x47800U,0x87ffffU}){Config c;c.reset=reset;Compare(c);}
 for(unsigned which=0;which<9;which++){
  IO io(Config{});Current value(io);PioneerQ38ResidentLayerResult out;
  if(which==0)value.ready=false;else if(which==1)value.unsafe=true;else if(which==2)value.closed=true;
  else if(which==3)value.ran=true;else if(which==4)value.running[0]=true;else if(which==5)out.prepared=0;
  else if(which==6)out.unsafe=1;else if(which==7)out.closed=1;else out.completed=1;
  assert(!value.Run(io,out) && io.events.empty() && value.runObservation.failure==Observation::None);cases++;
 }
 Observation observed;observed.Status(3,5);assert(!observed.statusValid && !observed.statusReads);
 observed.Poll();observed.Status(2,0x46524c51);observed.Pc(0x72074);observed.Pc(0x7207c);observed.Reset(0x46800);
 assert(observed.statusValid==4 && observed.statusReads==1 && observed.pcReads==2 && observed.pc[0]==0x72074 && observed.pc[1]==0x7207c);
 observed.Poll();observed.RunningPc(0x58644,false,false);observed.Poll();observed.RunningPc(0x72060,true,true);
 assert(observed.unexpectedPcCount==2 && observed.outsideImageCount==1 && observed.rejectedPcCount==1
  && observed.firstUnexpectedPc==0x58644 && observed.firstUnexpectedPoll==2);
 observed.Fail(Observation::Marker);auto frozen=observed;
 observed.Fail(Observation::Marker);observed.Status(0,1);observed.Pc(1);observed.Reset(1);observed.Poll();observed.RunningPc(0,false,false);
 assert(!std::memcmp(&observed,&frozen,sizeof(frozen)));cases++;
 printf("PASS actual Run/RootPhysical CPU observation: %u cases;unchanged predicates/order except no-marker speculative PC rejection;raw snapshots NEVER complete/hold/publish;finite exhaustion+all later failures retain/freeze;NO added reads/retries after failure;TEST branches ONLY,NOT native/device/cleanup/Work/speed qualification\n",cases);
}
'''

source = fixture + predicates + '\nstruct Legacy:Common {using Common::Common;\n' + legacy + '\n};\n'
source += 'struct Current:Common {using Common::Common;Observation runObservation;\n' + fail + '\n' + physical + '\n' + run + '\n};\n' + checks
with tempfile.TemporaryDirectory(prefix='pioneer-layer-run-observer-', dir='/private/tmp') as directory:
    path = pathlib.Path(directory)
    cpp = path / 'branch.cpp'
    cpp.write_text(source)
    binary = path / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(KERNEL), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
