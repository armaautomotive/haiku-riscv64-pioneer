#!/usr/bin/env python3
"""Exercise the real Assistant upload progress formatter without hardware."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = (here.parent / 'pioneer_assistant_qwen27.cpp').read_text()
begin = source.index(' static bool UploadProgress(')
method = source[begin:source.index('\n static bool Multiply(', begin)]
program = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
namespace A { enum { Progress=10 }; }
static double now=1;
static double Now(){return now;}
struct Session {uint64_t sourceReadNanos=2500000000ULL,bulkUploadNanos=4250000000ULL;};
struct Runtime {
 Session session; bool cancelled=false,poll=true; double lastProgress=0;unsigned emissions=0,value=0;std::string text;
 bool PollCancel(){return poll;}
 bool Emit(unsigned kind,unsigned v,const char* s){assert(kind==A::Progress);emissions++;value=v;text=s;return true;}
 METHOD
};
int main(){
 Runtime r;assert(Runtime::UploadProgress(&r,1048576,4194304));
 assert(r.emissions==1 && r.value==250 && r.text.find("1.0/4.0 MiB")!=std::string::npos);
 assert(r.text.find("Weight reads 2.5s; device upload calls 4.2s")!=std::string::npos);
 now=1.1;assert(Runtime::UploadProgress(&r,2097152,4194304) && r.emissions==1);
 r.cancelled=true;assert(Runtime::UploadProgress(&r,4194304,4194304) && r.emissions==2 && r.value==1000);
 assert(r.text.find("Stop pending")!=std::string::npos);
 r.poll=false;assert(!Runtime::UploadProgress(&r,4194304,4194304) && r.emissions==2);
 puts("PASS actual Assistant upload progress timing, throttling, final progress and lost channel; NO device");
}
'''.replace('METHOD', method)
with tempfile.TemporaryDirectory(prefix='assistant-qwen27-upload-progress-') as tmp:
    folder = Path(tmp)
    (folder / 'test.cpp').write_text(program)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
    subprocess.run([str(folder / 'test')], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
