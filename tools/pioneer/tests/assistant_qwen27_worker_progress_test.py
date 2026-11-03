#!/usr/bin/env python3
"""Actual worker log/progress methods with fake clock/channel; no model/device."""
import os
import pathlib
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_assistant_qwen27.cpp').read_text()
logger = source[source.index('static void Log('):source.index('\nstruct Runtime {')]
start = source.index(' static bool HostModelProgress(')
callback = source[start:source.index('\n static bool UploadProgress(', start)]
assert 'mp.progress_callback=HostModelProgress; mp.progress_callback_user_data=this;' in source
assert source.index('mp.progress_callback=') < source.index('model=llama_model_load_from_file_ptr(')
program = r'''
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
using std::isfinite;
enum ggml_log_level{GGML_LOG_LEVEL_NONE,GGML_LOG_LEVEL_DEBUG,GGML_LOG_LEVEL_INFO,GGML_LOG_LEVEL_WARN,GGML_LOG_LEVEL_ERROR,GGML_LOG_LEVEL_CONT};
namespace A{enum{Progress=10};}
static double now=1;static double Now(){return now;}
LOGGER
struct Runtime{
 bool lost=false,emitOK=true;double lastProgress=0;unsigned emits=0,value=9999;std::string text;
 bool Emit(unsigned kind,unsigned v,const std::string& t){assert(kind==A::Progress);emits++;value=v;text=t;if(!emitOK)lost=true;return !lost;}
CALLBACK
};
int main(){
 Log(GGML_LOG_LEVEL_NONE,"NO_NONE\n",nullptr);Log(GGML_LOG_LEVEL_DEBUG,"NO_DEBUG\n",nullptr);
 Log(GGML_LOG_LEVEL_INFO,"NO_INFO\n",nullptr);Log(GGML_LOG_LEVEL_CONT,"NO_CONT\n",nullptr);
 Log(GGML_LOG_LEVEL_WARN,"WARN\n",nullptr);Log(GGML_LOG_LEVEL_ERROR,"ERROR\n",nullptr);
 Runtime r;unsigned cases=0;
 assert(Runtime::HostModelProgress(0,&r) && r.emits==1 && !r.value);cases++;
 now=1.1;assert(Runtime::HostModelProgress(.5,&r) && r.emits==1);cases++;
 now=1.5;assert(Runtime::HostModelProgress(.5,&r) && r.emits==2 && r.value==500 && r.text=="Loading Qwen27B host model:50.0%; P100A upload deferred until first reply.");cases++;
 now=1.6;assert(Runtime::HostModelProgress(1,&r) && r.emits==3 && r.value==1000);cases++;
 for(float invalid:{-.01f,1.01f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){
  assert(!Runtime::HostModelProgress(invalid,&r) && r.emits==3);cases++;
 }
 r.lost=true;assert(!Runtime::HostModelProgress(1,&r) && r.emits==3);cases++;r.lost=false;
 now=-1;assert(!Runtime::HostModelProgress(1,&r) && r.emits==3);cases++;
 now=1.4;assert(!Runtime::HostModelProgress(1,&r) && r.emits==3);cases++;
 now=2;r.emitOK=false;assert(!Runtime::HostModelProgress(1,&r) && r.emits==4 && r.lost);cases++;
 assert(!Runtime::HostModelProgress(1,&r) && r.emits==4);cases++;
 assert(cases==13);
 puts("PASS actual worker host progress:13 fake clock/channel cases;framed bounded progress/final100%,invalid/lost/backwards/error refusal;WARN/ERROR preserved and numeric-CONT not misclassified;NO model/card/driver authority");
}
'''.replace('LOGGER', logger).replace('CALLBACK', callback)
folder = pathlib.Path(tempfile.mkdtemp(prefix='assistant-qwen27-worker-progress.'))
(folder / 'test.cpp').write_text(program)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
run = subprocess.run([str(folder / 'test')], text=True, capture_output=True)
assert run.returncode == 0 and run.stderr == 'WARN\nERROR\n', (run.returncode, run.stdout, run.stderr)
print(run.stdout, end='')
print('CPU-only actual worker progress evidence:', folder)
