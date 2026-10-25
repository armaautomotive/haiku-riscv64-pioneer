#!/usr/bin/env python3
"""Actual IO-wrapper/clock component tests,NOT native scope or execution proof."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
owner = (kernel/'PioneerQ38NextFfnResidualOwner.h').read_text()
start, end = ' template<class IO> bool NextFfnAlive(', '\n bool NextFfnData()const{'
assert owner.count(start) == owner.count(end) == 1
methods = start+owner.split(start, 1)[1].split(end, 1)[0]
csr = (kernel/'PioneerQ38GraphCsrProbe.h').read_text()
start = 'template<class IO> bool PioneerQ38GraphCsrIdle('
assert csr.count(start) == 1
idle = start+csr.split(start, 1)[1].split('\ntemplate<class IO>', 1)[0]
code = ''.join('#include "'+str(kernel/name)+'"\n' for name in (
    'PioneerQ38HostDeadline.h', 'PioneerQ38ProjectionGraphFootprint.h'))
code += '#include <cassert>\n#include <cstdio>\n#include <initializer_list>\n'
code += idle+'\n'
code += r'''
// TEST byte endpoints only. Guard is a bounded mock endpoint,NOT the
// production native Scope,source,initialization,lease or ready authority.
struct IO {
 uint64_t now=1000;unsigned touches=0,observedBadAt=UINT32_MAX,fault=0;uint32_t value=7;
 uint64_t Q38CohortNow(){if(now<1000 || now>=30001000)observedBadAt=touches;return now;}
 void Touch(){touches++;if(fault)now=fault==1?30001000:999;}
 bool ProjectionGraphMemoryReadChecked(unsigned,uint32_t,uint32_t& v){Touch();v=value;return true;}
 bool ProjectionGraphMemoryWrite(unsigned,uint32_t,uint32_t v){Touch();value=v;return true;}
 uint32_t Q38PanelRead(uint32_t){Touch();return 7;}
 uint32_t RuntimeStatusRead(unsigned){Touch();return 7;}
 uint32_t RuntimePcRead(){Touch();return 7;}
 uint32_t ResidentNocRead(unsigned a){Touch();return a==0x234?7:0;}
 void RuntimeStatusWrite(unsigned,uint32_t){Touch();}
 void ResidentNocWrite(unsigned,uint32_t){Touch();}
 void RuntimeResetWrite(uint32_t){Touch();}
 void Pause(){Touch();}
};
struct State {bool timing=true,uncertain=false;PioneerQ38HostDeadline deadline;};
struct Component {
 State s;State* nextFfn=&s;
 Component(){assert(s.deadline.Begin(1000));}
 bool Guard(IO& io,unsigned){io.Touch();return true;}
'''
code += methods
code += r'''
};
bool Invoke(Component& c,IO& io,unsigned op){
 uint32_t v=0,words[]={7,7};
 switch(op){
  case 0:return c.NextFfnGuard(io,0);
  case 1:return c.NextFfnMemory(io,0,0x64000,v);
  case 2:return c.NextFfnWrite(io,0,0x64000,7);
  case 3:return c.NextFfnPanel(io,0x64000)==7;
  case 4:return c.NextFfnStatus(io,2)==7;
  case 5:return c.NextFfnPc(io)==7;
  case 6:return c.NextFfnNoc(io,0x234)==7;
  case 7:return c.NextFfnStatusWrite(io,2,7);
  case 8:return c.NextFfnNocWrite(io,0x234,7);
  case 9:return c.NextFfnReset(io,0x46800);
  case 10:return c.NextFfnPause(io);
  case 11:return c.NextFfnQuiet(io);
  case 12:return c.NextFfnStage(io,0x64000,words,2);
 }
 assert(false);return false;
}
int main(){
 unsigned checks=0,observations=0;
 for(unsigned op=0;op<13;op++){
  Component c;IO io;assert(Invoke(c,io,op) && !c.s.uncertain);checks++;
  assert(io.touches==(op==2?2U:op==11?36U:op==12?4U:1U));checks++;
  for(unsigned fault:{1U,2U})for(bool before:{false,true}){
   Component bad;IO endpoint;endpoint.fault=fault;
   if(before)endpoint.now=fault==1?30001000:999;
   assert(!Invoke(bad,endpoint,op) && bad.s.uncertain && endpoint.observedBadAt!=UINT32_MAX);checks++;
   unsigned at=endpoint.touches;
   assert(at==endpoint.observedBadAt && (before?!at:at==(op==11?36U:1U)));checks++;
   // The bounded CSR-idle GROUP observes its clock after36reads. This does
   // NOT claim a clock observation between each CSR read. All following
   // calls must do ZERO IO after that observed expiry/rollback.
   for(unsigned next=0;next<13;next++){
    assert(!Invoke(bad,endpoint,next) && endpoint.touches==at);checks++;
   }
   endpoint.now=1000;assert(!bad.NextFfnAlive(endpoint) && endpoint.touches==at);checks++;observations++;
  }
 }
 Component absent;IO io;absent.nextFfn=nullptr;
 for(unsigned op=0;op<13;op++){assert(!Invoke(absent,io,op) && !io.touches);checks++;}
 PioneerQ38HostDeadline overflow;assert(!overflow.Begin(UINT64_MAX-29999999) && !overflow.Observe(1000));checks++;
 printf("PASS actual native65 IO-wrapper component checks%u/13operations/%u observed expiry-rollback cases/ZERO IO after observation/sticky failure/no renewal/actual36-read CSR group;TEST endpoints only,NO native owner or device grant\n",checks,observations);
}
'''
with tempfile.TemporaryDirectory(prefix='pioneer-native65-deadline-component-', dir='/private/tmp') as temporary:
    path, binary = pathlib.Path(temporary)/'test.cpp', pathlib.Path(temporary)/'test'
    path.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
