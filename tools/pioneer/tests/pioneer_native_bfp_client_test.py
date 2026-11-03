#!/usr/bin/env python3
"""Execute ACTUAL diagnostic client with synthetic syscall endpoints, NEVER a device."""
import os
import pathlib
import re
import subprocess
import tempfile

if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-native-bfp-client.'))
(folder / 'Drivers.h').write_text('#pragma once\n#define B_DEVICE_OP_CODES_END 10000\n')
program = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
static unsigned scenario=0,format=4,failedWord=0,opened=0,closed=0,supports=0,runs=0,ends=0,queries=0;
static int FakeOpen(const char*,int);
static int FakeClose(int);
static int FakeIoctl(int,unsigned long,void*,size_t);
static uid_t FakeEuid();
#define open FakeOpen
#define close FakeClose
#define ioctl FakeIoctl
#define geteuid FakeEuid
#define main CapturedDiagnosticMain
#include "CLIENT_SOURCE"
#undef main
#undef geteuid
#undef ioctl
#undef close
#undef open
static uid_t FakeEuid(){return scenario==9?1:0;}
static int FakeOpen(const char* path,int flags){
 assert(!strcmp(path,"/dev/misc/tenstorrent_pci_probe") && flags==(O_RDWR|O_CLOEXEC) && !opened++);
 return scenario==8?-1:123;
}
static int FakeClose(int fd){assert(fd==123 && !closed++);return scenario==7?-1:0;}
static void Fill(PioneerNativeBfpRequest& r,bool end){
 assert(PioneerNativeBfpRequestValid(r) && r.bits==format && r.expected_generation==19);
 assert(end?(r.nonce==1 && r.accept==1):(!r.nonce && !r.accept));
 r.nonce=1;auto& v=r.result;v.state=end?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT;
 v.started=v.completed=1;v.polls=1;v.pc[0]=0x70038;v.pc[1]=0x7003c;
 v.phase=0x7000c;v.count=2;v.marker=0x3142464e;
 v.held=v.l1_restored=v.tlb_restored=end;
 for(unsigned word=0;word<64;word++)v.output[word]=Expected(format,word%32);
}
static int FakeIoctl(int fd,unsigned long op,void* data,size_t bytes){
 assert(fd==123 && !closed);
 if(op==TT_PCI_PROBE_RELOAD_STATUS){
  assert(bytes==sizeof(tt_pioneer_driver_reload) && !queries++ && !supports && !runs && !ends);
  auto& v=*static_cast<tt_pioneer_driver_reload*>(data);
  assert(v.version==1 && v.expected_tag==PioneerNativeBfpTag);
  v.driver=55;v.build_tag=PioneerNativeBfpTag;v.generation=19;v.open_count=v.guard_version=1;
  if(scenario==2)v.cold=1;
  return 0;
 }
 assert(bytes==sizeof(PioneerNativeBfpRequest));auto& r=*static_cast<PioneerNativeBfpRequest*>(data);
 if(op==PIONEER_NATIVE_BFP_SUPPORT){
  assert(queries==1 && !supports++ && !runs && !ends && PioneerNativeBfpRequestValid(r) && !r.bits && !r.nonce && !r.accept);
  return scenario==3?-1:0;
 }
 if(op==PIONEER_NATIVE_BFP_RUN){
  assert(supports==1 && !runs++ && !ends);Fill(r,false);
  if(scenario==1)r.result.output[failedWord]^=1;
  if(scenario==4)r.result.pc[1]=0x70040;
  if(scenario==10)r.nonce=2;
  if(scenario==11)return -1;
  return 0;
 }
 assert(op==PIONEER_NATIVE_BFP_END && runs==1 && !ends++ && scenario!=1 && scenario!=4 && scenario!=10 && scenario!=11);
 Fill(r,true);if(scenario==5)return -1;if(scenario==6)r.result.output[63]^=1;return 0;
}
int main(int argc,char** argv){
 assert(argc==4);scenario=unsigned(atoi(argv[1]));format=unsigned(atoi(argv[2]));failedWord=unsigned(atoi(argv[3]));
 assert(scenario<=11 && (format==4 || format==8) && failedWord<64);
 char name[]="diagnostic",four[]="--fixed-bfp4",eight[]="--fixed-bfp8";
 char* command[]={name,format==4?four:eight,nullptr};
 const int result=CapturedDiagnosticMain(2,command);
 assert(result==int(scenario!=0));
 assert(opened==unsigned(scenario!=9));
 assert(closed==unsigned(scenario!=8 && scenario!=9));
 assert(queries==unsigned(scenario!=8 && scenario!=9));
 assert(supports==unsigned(scenario!=2 && scenario!=8 && scenario!=9));
 assert(runs==unsigned(scenario!=2 && scenario!=3 && scenario!=8 && scenario!=9));
 assert(ends==unsigned(scenario==0 || scenario==5 || scenario==6 || scenario==7));
 return 0;
}
'''.replace('CLIENT_SOURCE', str(here.parent / 'pioneer_native_bfp_diagnostic.cpp'))
(folder / 'test.cpp').write_text(program)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-I' + str(folder),
                str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
cases = 0
for bits in (4, 8):
    for scenario in range(12):
        for index in (range(64) if scenario == 1 else (0,)):
            result = subprocess.run([str(folder / 'test'), str(scenario), str(bits), str(index)],
                                    text=True, capture_output=True, env=dict(os.environ,
                                    ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
            assert result.returncode == 0, (bits, scenario, index, result.stdout, result.stderr)
            if scenario == 0:
                assert not result.stderr and result.stdout.startswith('PASS_NATIVE_BFP_FIXED_KERNEL ')
            elif scenario == 1:
                assert not result.stdout
                lines = result.stderr.splitlines()
                assert len(lines) == 65 and lines[0].startswith('NATIVE_BFP_NUMERICAL_MISMATCH ')
                assert f'pass{index//32} row{index%32} ' in lines[0]
                mismatches = 0
                for word, line in enumerate(lines[1:]):
                    match = re.fullmatch(r'NATIVE_BFP_RETURNED_WORD bits(4|8) pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
                    assert match and tuple(map(int, match.groups()[:3])) == (bits, word//32, word%32)
                    got, expected = map(lambda v: int(v, 16), match.groups()[3:])
                    assert got == (expected ^ int(word == index))
                    mismatches += int(got != expected)
                assert mismatches == 1
            else:
                assert not result.stdout and 'NATIVE_BFP_RETURNED_WORD' not in result.stderr
                assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr
            cases += 1
print(f'PASS ACTUAL native BFP diagnostic client:{cases} cases;all128 per-word numerical failures retain all64 returned words and issue NO END/retry/query after RUN;strict acceptance unchanged;FAKE syscalls,no device')
print('CPU-only actual client evidence:', folder)
