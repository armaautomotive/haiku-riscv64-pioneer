#!/usr/bin/env python3
"""Actual method component regression,NOT a native producer/initialization proof."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
source = (kernel/'PioneerQ38NextFfnResidualOwner.h').read_text()
methods = ''
for signature in (' template<class IO> int NextFfnPeer(', ' template<class IO> bool NextFfnRootTerminal('):
    assert source.count(signature) == 1
    methods += signature+source.split(signature, 1)[1].split('\n template<class IO>', 1)[0]+'\n'
code = ''.join('#include "'+str(kernel/name)+'"\n' for name in (
    'PioneerQ38DispatchImages.h', 'PioneerQ38FfnResidualGraphImages.h', 'PioneerQ38FfnResidualGraphShape.h'))
code += r'''
#include <cassert>
#include <cstdio>
struct IO {};
struct Projection {bool running[120]={};};
struct State {unsigned sequence[119]={};bool stopped=false,stopReceipt[120]={},held[120]={},rootHeld=false,rootProof=false,stopping=false;};
struct Component {
 Projection p;State s;Projection* projection=&p;State* nextFfn=&s;
 unsigned resets[120]={},marker=TT_Q38_FFN_PERSISTENT_MARKER,requested=7,finished=7,operation=2;
 unsigned rootMarker=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER,entry=0x7200c,count=119,pc=0;
 unsigned pcReads=0,pauses=0;bool alive=true;
 bool NextFfnAlive(IO&){return alive;}
 unsigned NextFfnPanel(IO&,unsigned a){
  if(a==0x7414c)return marker;if(a==0x74140)return requested;if(a==0x74144)return finished;
  if(a==0x74150)return operation;return 0;
 }
 unsigned NextFfnStatus(IO&,unsigned a){return a==2?rootMarker:a==0?entry:count;}
 bool NextFfnPause(IO&){pauses++;return true;}
 unsigned NextFfnPc(IO&){pcReads++;return pc;}
'''
code += methods
code += r'''
};
int main(){
 IO io;Component c;c.s.sequence[118]=7;c.resets[119]=0x47800;c.s.stopped=true;
 unsigned checks=0;
 // Held state WITHOUT actual retained STOP+hold evidence MUST refuse.
 assert(c.NextFfnPeer(io,119,true)==-1 && !c.pcReads && !c.pauses);checks++;
 c.s.stopReceipt[119]=true;assert(c.NextFfnPeer(io,119,true)==-1 && !c.pcReads);checks++;
 c.s.held[119]=true;assert(c.NextFfnPeer(io,119,true)==1 && !c.pcReads && !c.pauses);checks++;
 for(unsigned fault=0;fault<8;fault++){
  Component x=c;
  // Copy only TEST component data;no native owner,lease or producer exists.
  x.projection=&x.p;x.nextFfn=&x.s;
  if(fault==0)x.s.stopped=false;if(fault==1)x.s.stopReceipt[119]=false;if(fault==2)x.s.held[119]=false;
  if(fault==3)x.resets[119]=0x87ffff;if(fault==4)x.requested=8;if(fault==5)x.finished=6;
  if(fault==6)x.operation=0;if(fault==7)x.alive=false;
  assert(x.NextFfnPeer(io,119,true)==-1 && !x.pcReads && !x.pauses);checks++;
 }
 assert(c.NextFfnPeer(io,119,false)==-1 && !c.pcReads);checks++;
 // Running cores MUST still pass two actual terminal-PC observations.
 c.p.running[119]=true;c.pc=TT_Q38_PEER_STOP;
 assert(c.NextFfnPeer(io,119,true)==1 && c.pcReads==2 && c.pauses==2);checks++;
 c.pc=TT_Q38_PEER_FAULT_PARK;assert(c.NextFfnPeer(io,119,true)==-1 && c.pcReads==3);checks++;
 Component root;root.s.stopping=true;root.resets[0]=0x47800;
 assert(!root.NextFfnRootTerminal(io) && !root.pcReads && !root.pauses);checks++;
 root.s.rootHeld=true;root.s.held[0]=true;root.s.rootProof=true;
 assert(root.NextFfnRootTerminal(io) && !root.pcReads && !root.pauses);checks++;
 for(unsigned fault=0;fault<9;fault++){
  Component x=root;x.projection=&x.p;x.nextFfn=&x.s;
  if(fault==0)x.s.rootHeld=false;if(fault==1)x.s.held[0]=false;if(fault==2)x.s.rootProof=false;
  if(fault==3)x.resets[0]=0x87ffff;if(fault==4)x.rootMarker^=1;if(fault==5)x.entry^=1;
  if(fault==6)x.count=5120;if(fault==7)x.s.stopping=false;if(fault==8)x.alive=false;
  assert(!x.NextFfnRootTerminal(io) && !x.pcReads && !x.pauses);checks++;
 }
 root.p.running[0]=true;root.pc=TT_Q38_DISPATCH_PARK;
 assert(root.NextFfnRootTerminal(io) && root.pcReads==2 && root.pauses==2);checks++;
 root.pc=TT_Q38_DISPATCH_FAULT_PARK;assert(!root.NextFfnRootTerminal(io) && root.pcReads==3);checks++;
 printf("PASS actual native65 peer+ROOT method component checks%u/held PC NEVER sampled/missing retained STOP-hold proofs refused/running terminal PCs still checked;TEST DATA ONLY,NO native owner or device grant\n",checks);
}
'''
with tempfile.TemporaryDirectory(prefix='pioneer-native65-held-component-', dir='/private/tmp') as temporary:
    path, binary = pathlib.Path(temporary)/'test.cpp', pathlib.Path(temporary)/'test'
    path.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
