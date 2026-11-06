#!/usr/bin/env python3
"""Actual86 client comparators over explicit source/IO fixtures;NO GPU."""
import concurrent.futures
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
paired=sys.argv[-1:] == ['--pair-issue']
timing=paired or sys.argv[-1:] == ['--timing']
if timing:sys.argv.pop()
assert __debug__ and len(sys.argv) == 2
sample_count=2 if timing else 9
result_bytes=416 if timing else 352
private = pathlib.Path(sys.argv[1])
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
meta = private / 'PioneerNativeCompactQ4StreamMetadata.h'
assert hashlib.sha256(meta.read_bytes()).hexdigest() == 'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-q4-client-test.'))
(folder / 'Drivers.h').write_text('#pragma once\n#define B_DEVICE_OP_CODES_END 10000\n')
program = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include "Drivers.h"
#include "PioneerNativeCompactQ4StreamMetadata.h"
#include "PROTOCOL"
#include "RELOAD"
static unsigned scenario=0,target=0,word=0,opens=0,closes=0,queries=0,supports=0,runs=0,ends=0,readies=0,deliveries=0;
static bool poisoned=false,prepared=false,live=false;
static bool Fail(){return opens && opens-1==target;}
class PioneerNativeCompactQ4StreamSource {
public:
 bool Prepare(const char* p,std::string&){assert(!opens && !prepared && !poisoned && !strcmp(p,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf"));prepared=true;return scenario!=1;}
 bool Ready(){assert(prepared);readies++;if(scenario==2 && !opens){poisoned=true;return false;}
  if(Fail() && ((scenario==3 && supports && runs<opens) || (scenario==4 && runs==opens && ends<opens) || (scenario==5 && ends==opens))){poisoned=true;return false;}return true;}
};
static int MockUid(){return scenario==6?1:0;}
static int MockOpen(const char* path,int flags){assert(prepared && !live && !poisoned && readies && !strcmp(path,"/dev/misc/tenstorrent_pci_probe") && flags==(O_RDWR|O_CLOEXEC));opens++;if(Fail() && scenario==7){poisoned=true;return -1;}live=true;return 100;}
static int MockClose(int fd){assert(fd==100 && live);live=false;closes++;return Fail() && scenario==8?-1:0;}
static int MockFlush(FILE* f){deliveries++;if(scenario==200 && deliveries==word){poisoned=true;return EOF;}return fflush(f);}
static int MockIoctl(int fd,unsigned long operation,void* data,size_t bytes){
 assert(fd==100 && live && !poisoned);
 if(operation==TT_PCI_PROBE_RELOAD_STATUS){
  assert(bytes==sizeof(tt_pioneer_driver_reload));auto& m=*static_cast<tt_pioneer_driver_reload*>(data);
  assert(m.version==1 && m.expected_tag==PioneerNativeCompactQ4StreamVariant::Tag && !m.expected_generation && !m.build_tag);
  m={};m.version=1;m.driver=55;m.build_tag=PioneerNativeCompactQ4StreamVariant::Tag;m.generation=37;m.open_count=1;m.guard_version=1;queries++;
  if(Fail() && scenario>=10 && scenario<=26){
   if(scenario==10)m.version=2;if(scenario==11)m.driver=54;if(scenario==12)m.build_tag^=1;if(scenario==13)m.generation=0;
   if(scenario==14)m.open_count=0;if(scenario==15)m.open_count=2;if(scenario==16)m.active_owner=1;if(scenario==17)m.guard_version=0;
   if(scenario==18)m.quiesced=1;if(scenario==19)m.cold=1;if(scenario==20)m.expected_tag=1;if(scenario==21)m.expected_generation=1;
   if(scenario>=22 && scenario<=24)m.reserved[scenario-22]=1;if(scenario==25)m.generation++;
   poisoned=true;return scenario==26?-1:0;
  }return 0;
 }
 assert(bytes==sizeof(PioneerNativeCompactQ4StreamRequest));auto& r=*static_cast<PioneerNativeCompactQ4StreamRequest*>(data);
 assert(PioneerNativeCompactQ4StreamRequestValid(r) && r.expected_generation==37);
 if(operation==PIONEER_NATIVE_COMPACT_Q4_STREAM_SUPPORT){assert(r.sample==PioneerNativeCompactQ4StreamNone && !r.nonce && !r.accept);supports++;
  if(Fail() && scenario==30){reinterpret_cast<unsigned char*>(&r.result)[word]=1;poisoned=true;return 0;}
  if(Fail() && scenario>=31 && scenario<=39){if(scenario==31)r.version=2;if(scenario==32)r.expected_tag^=1;if(scenario==33)r.expected_generation++;
   if(scenario==34)r.sample=0;if(scenario==35)r.nonce=1;if(scenario==36)r.accept=1;if(scenario==37)r.reserved=1;
   poisoned=true;return scenario==39?-1:0;}return 0;
 }
 bool end=operation==PIONEER_NATIVE_COMPACT_Q4_STREAM_END;assert(end || operation==PIONEER_NATIVE_COMPACT_Q4_STREAM_RUN);
 assert(r.sample==opens-1 && r.nonce==unsigned(end) && r.accept==unsigned(end));if(end)ends++;else runs++;
 auto& c=r.result.compute;r.nonce=1;c.state=end?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT;c.started=c.completed=1;c.polls=2;
 c.phase=0x7000c;c.count=PioneerNativeCompactQ4StreamVariant::Count(r.sample);c.marker=PioneerNativeCompactQ4StreamVariant::Marker;c.pc[0]=c.pc[1]=0x70038;
 if(scenario==201)c.pc[0]=0x7003c;
 c.held=c.l1_restored=c.tlb_restored=end;c.controls[0]=2;r.result.phase=end?10:5;r.result.niu_restored=r.result.clock_restored=end;r.result.saved_l1=26908;r.result.gddr_restored=end;r.result.saved_gddr=PioneerNativeCompactQ4StreamVariant::Width(r.sample)/256*1280;r.result.requests=2*(PioneerNativeCompactQ4StreamVariant::Width(r.sample)/256)*26;r.result.flits=2*(PioneerNativeCompactQ4StreamVariant::Width(r.sample)/256)*80;
 for(unsigned i=0;i<64;i++)c.output[i]=PioneerNativeCompactQ4StreamMetadata::Golden(r.sample)[i%32];
 const unsigned base=end?100:50;
 if(Fail() && scenario>=base && scenario<=base+38){unsigned f=scenario-base;
  if(f==0)r.version=2;if(f==1)r.sample++;if(f==2)r.expected_tag^=1;if(f==3)r.expected_generation++;if(f==4)r.nonce=0;if(f==5)r.accept^=1;if(f==6)r.reserved=1;
  if(f==7)c.state=PIONEER_NATIVE_BFP_UNKNOWN;if(f==8)c.cold=1;if(f==9)c.started=0;if(f==10)c.completed=0;if(f==11)c.polls=0;if(f==12)c.polls=20001;
  if(f==13)c.phase^=4;if(f==14)c.count^=1;if(f==15)c.marker^=1;if(f==16)c.pc[0]=0x70040;if(f==17)c.pc[1]=0x70040;
  if(f==18)c.held^=1;if(f==19)c.l1_restored^=1;if(f==20)c.tlb_restored^=1;if(f==21)r.result.phase^=1;
  if(f==22)r.result.niu_restored^=1;if(f==23)r.result.clock_restored^=1;if(f==24)r.result.saved_l1--;
  if(f==25)c.controls[0]|=1;if(f==26)c.controls[1]=1;if(f==27)c.controls[2]=1;if(f==28)c.output[word]^=1;
  if(f==29)c.marker=0x31504651;if(f==30)c.count=0x04000100;
  if(f==31){if(end)c.controls[0]^=2;else c.started=2;}if(f==32){if(end)c.polls++;else c.completed=2;}
  if(f==34)r.result.gddr_restored^=1;if(f==35)r.result.saved_gddr--;if(f==36)r.result.requests--;if(f==37)r.result.flits--;if(f==38)c.count=PioneerNativeCompactQ4StreamVariant::Count(r.sample<6?6:0);
  poisoned=true;return f==33?-1:0;
 }return 0;
}
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_SOURCE_H
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#define geteuid MockUid
#define fflush MockFlush
#define main DiagnosticMain
#include "CLIENT"
#undef main
#undef open
#undef close
#undef ioctl
#undef geteuid
#undef fflush
int main(int argc,char** argv){assert(argc==4);scenario=unsigned(strtoul(argv[1],nullptr,10));target=unsigned(strtoul(argv[2],nullptr,10));word=unsigned(strtoul(argv[3],nullptr,10));
 char name[]="diagnostic104",option[]="--fixed-compact-q4-full-k-stream",bad[]="--old84";char* args[]={name,scenario==9?bad:option};
 const int result=DiagnosticMain(2,args);bool okay=!scenario || scenario==201;
 assert((result==0)==okay && !live);
 if(okay)assert(opens==9 && closes==9 && queries==9 && supports==9 && runs==9 && ends==9);
 else if(scenario==200){unsigned n=word<3?0:word==12?9:word-2;assert(opens==n && closes==n && queries==n && supports==n && runs==n && ends==n);}
 else if(scenario<=2 || scenario==6 || scenario==9)assert(!opens && !queries && !runs && !ends);
 else {assert(opens==target+1 && closes==opens-(scenario==7));
  if(scenario==7)assert(queries==target && supports==target && runs==target && ends==target);
  else if(scenario>=10 && scenario<=26)assert(queries==target+1 && supports==target && runs==target && ends==target);
  else if(scenario==3 || (scenario>=30 && scenario<=39))assert(supports==target+1 && runs==target && ends==target);
  else if(scenario==4 || (scenario>=50 && scenario<=88))assert(runs==target+1 && ends==target);
  else assert(runs==target+1 && ends==target+1);
 }
}
'''
program = program.replace('"PROTOCOL"', '"' + str(root / 'src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h') + '"').replace('"RELOAD"', '"' + str(root / 'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h') + '"').replace('"CLIENT"', '"' + str(here.parent / 'pioneer_native_compact_q4_stream_diagnostic.cpp') + '"')
if timing:
    generation=45 if paired else 43
    program=program.replace('m.generation=37;',f'm.generation={generation};').replace('r.expected_generation==37',f'r.expected_generation=={generation}')
    program=program.replace('r.sample==opens-1','r.sample==(opens==1?0U:6U)')
    program=program.replace('--fixed-compact-q4-full-k-stream','--fixed-compact-q4-pair-issue-full-k-stream' if paired else '--fixed-compact-q4-timing-full-k-stream')
    program=program.replace('bool okay=!scenario || scenario==201;','bool okay=!scenario || scenario==201 || scenario==202 || scenario==203;')
    program=program.replace('opens==9 && closes==9 && queries==9 && supports==9 && runs==9 && ends==9',
                            'opens==2 && closes==2 && queries==2 && supports==2 && runs==2 && ends==2')
    program=program.replace('word==12?9:word-2','word==5?2:word-2')
    marker=' const unsigned base=end?100:50;'
    assert program.count(marker)==1
    program=program.replace(marker,r'''
 for(unsigned pass=0;pass<2;pass++){
  auto* t=r.result.timing+pass*8;t[0]=0x314d5451;t[1]=scenario==202?0x20001:0x10001;
  t[2]=PioneerNativeCompactQ4StreamVariant::Width(r.sample);t[3]=pass;
  if(scenario!=202){t[4]=1000+pass*3000;t[6]=t[4]+2000;}
  if(scenario==203)t[0]^=1;
 }
 if(Fail() && scenario==204 && end){r.result.timing[word]^=1;poisoned=true;return 0;}
''' + marker)
(folder / 'test.cpp').write_text(program)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                *(['-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1'] if timing else []),
                *(['-DPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING=1'] if paired else []),
                '-I' + str(folder), '-I' + str(private), '-I' + str(here.parent), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
cases = [(0, 0, 0), (1, 0, 0), (2, 0, 0), (6, 0, 0), (9, 0, 0), (201, 0, 0)]
cases += [(200, 0, point) for point in range(1,sample_count+4)]
if timing:
    cases += [(202,0,0),(203,0,0)]
    cases += [(204,target,word) for target in range(2) for word in range(16)]
for target in range(sample_count):
    for scenario in (3, 4, 5, 7, 8, *range(10, 25), 26, *range(31, 38), 39, *range(50, 89), *range(100, 139)):
        for word in (range(64) if scenario in (78, 128) else (0,)):
            cases.append((scenario, target, word))
    for byte in range(result_bytes):
        cases.append((30, target, byte))
for target in range(sample_count):
    cases.append((25, target, 0))
def check_case(case):
    scenario, target, word = case
    result = subprocess.run([str(folder / 'test'), str(scenario), str(target), str(word)], capture_output=True,
                            env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    assert result.returncode == 0, (scenario, target, word, result.stdout.decode(), result.stderr.decode())
    assert b'AddressSanitizer' not in result.stderr and b'runtime error:' not in result.stderr
    if not scenario:
        assert not result.stderr and result.stdout.count(b'NATIVE_COMPACT_Q4_STREAM_RETURNED_WORD') == sample_count*64 and result.stdout.count(b'PASS_NATIVE_COMPACT_Q4_STREAM_SAMPLE sample') == sample_count
        if timing:assert result.stdout.count(b'NATIVE_COMPACT_Q4_TIMING sample')==4 and result.stdout.count(b'cycles2000;CARD_CYCLES_ONLY_NO_FREQUENCY_ASSUMPTION')==4
    if timing and scenario in (202,203):
        assert not result.stderr and result.stdout.count(b'UNAVAILABLE_NO_TIMING_OR_CLEANUP_AUTHORITY')==4 and b' cycles' not in result.stdout
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    for index, _ in enumerate(pool.map(check_case, cases)):
        if index and not index % 400:
            print(f'CLIENT_FIXTURES_PROGRESS {index}/{len(cases)};NO GPU', flush=True)

assert hashlib.sha256(meta.read_bytes()).hexdigest() == 'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'
print(f'PASS ACTUAL{107 if timing else 104} client:{len(cases)} ASan/UBSan explicit source/device fixture cases,all{result_bytes} SUPPORT bytes/all64 RUN+END words in all{sample_count} samples plus GDDR journal/traffic/width fields;NO END/query/next sample/retry after uncertainty;NOT original authentication/native/GPU/model/speed proof')
print('CPU-only actual104 client evidence:', folder)
