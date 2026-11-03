#!/usr/bin/env python3
"""Actual Runtime::Multiply policy with fake sessions, no model/driver/device."""
import os
import pathlib
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_assistant_qwen27.cpp').read_text()
begin = source.index(' static bool Multiply(')
method = source[begin:source.index('\n bool Load(', begin)]
scope_begin = source.index('  struct GenerateScope {')
scope = source[scope_begin:source.index('  } computeScope(generating);', scope_begin)] + '\n  };\n'
assert source.index('if (count<1') < scope_begin < source.index('llama_memory_clear(')
assert source.index('if (generating.load(std::memory_order_acquire)) return false;') < scope_begin
assert 'bool Close() {\n  generating.store(false,std::memory_order_release);' in source
assert '!snapshot.failed && !snapshot.projections && snapshot.recognizedWeights==497' in source
program = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <atomic>
namespace A{enum{Progress=10};}
struct FakeSession{
 unsigned opens=0,observes=0,primes=0,multiplies=0,aborts=0;bool openOK=true,primeOK=true,multiplyOK=true;
 bool Open(){opens++;return openOK;}
 void Observe(bool(*)(void*,uint64_t,uint64_t),void*){observes++;}
 bool PrimeSpan(){primes++;return primeOK;}
 bool Multiply(unsigned,const float*,unsigned,float*,unsigned){multiplies++;return multiplyOK;}
 void Abort(){aborts++;}
};
struct Runtime{
 std::atomic<bool> generating{false};bool opened=false,pollOK=true,emitOK=true;unsigned polls=0,emits=0;FakeSession session;
 bool PollCancel(){polls++;return pollOK;}
 bool Emit(unsigned,unsigned,const char*){emits++;return emitOK;}
 static bool UploadProgress(void*,uint64_t,uint64_t){return true;}
METHOD
};
SCOPE
int main(){unsigned cases=0;
 for(unsigned scenario=0;scenario<10;scenario++){
  Runtime r;r.generating=scenario>=2;r.opened=scenario==1 || scenario==7 || scenario==9;
  if(scenario==2 || scenario==9)r.pollOK=false;
  if(scenario==3)r.session.openOK=false;
  if(scenario==4)r.session.primeOK=false;
  if(scenario==5)r.emitOK=false;
  if(scenario==8)r.session.multiplyOK=false;
  bool result=Runtime::Multiply(&r,11,nullptr,32,nullptr,32);
  assert(result==(scenario==6 || scenario==7));
  if(scenario<2)assert(!r.polls && !r.emits && !r.session.opens && !r.session.primes && !r.session.multiplies && !r.session.aborts);
  else{
   assert(r.polls==1 && r.session.opens==unsigned(scenario>=3 && scenario<=6 || scenario==8));
   assert(r.session.primes==unsigned(scenario>=4 && scenario<=6 || scenario==8));
   assert(r.session.observes==r.session.primes);
   assert(r.emits==unsigned(scenario==5 || scenario==6 || scenario==8));
   assert(r.session.multiplies==unsigned(scenario==6 || scenario==7 || scenario==8));
   assert(r.session.aborts==unsigned(scenario==2 || scenario==5 || scenario==9));
  }
  cases++;
 }
 assert(cases==10);std::atomic<bool> flag{false};{GenerateScope permit(flag);assert(flag.load());}assert(!flag.load());
 puts("PASS actual Assistant compute gate:10 fake-session cases;Load/reserve/idle/close cannot poll/open/prime/multiply;only validated Generate permits existing cold/warm Work dispatch;scope resets on exit;NO device/model/CPU fallback");
}
'''.replace('METHOD', method).replace('SCOPE', scope)
folder = pathlib.Path(tempfile.mkdtemp(prefix='assistant-qwen27-compute-gate.'))
(folder / 'test.cpp').write_text(program)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
subprocess.run([str(folder / 'test')], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
print('CPU-only actual compute-gate evidence:', folder)
