#!/usr/bin/env python3
"""Execute ACTUAL new client with fake endpoints and independent recorded words."""
import hashlib
import os
import pathlib
import re
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
record = (here.parent / 'benchmarks/qwen38-native-bfp8-observe-20261104/fixed8-returned.stderr').read_bytes()
assert hashlib.sha256(record).hexdigest() == '66112229d3b318d6e79864ecf6d45ce3007a32ae44b7b6c939ce7b05c6ccdf36'
observed = [int(x, 16) for x in re.findall(rb'RETURNED_WORD bits8 pass[01] row[0-9]+ got([0-9a-f]{8}) expected', record)]
assert len(observed) == 64 and observed[:32] == observed[32:]
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-bfp8-documented-client.'))
(folder / 'Drivers.h').write_text('#pragma once\n#define B_DEVICE_OP_CODES_END 10000\n')
driver = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
reload_control = driver.split('// PIONEER_DRIVER_RELOAD_CONTROL_BEGIN\n', 1)[1].split('// PIONEER_DRIVER_RELOAD_CONTROL_END', 1)[0]
program = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "ARITHMETIC_HEADER"
static unsigned scenario=0,failedWord=0,opened=0,closed=0,queries=0,supports=0,runs=0,ends=0,mathCalls=0;
const uint32_t Observed[64]={OBSERVED_WORDS}; // independent immutable physical record
struct InjectArithmetic {
 static bool Instruction(const uint16_t (&a)[16],const uint16_t (&b)[16],unsigned phase,uint32_t prior,uint32_t& result){
  mathCalls++;if(scenario==20)return false;
  bool ok=PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,prior,result);
  if(scenario==21 && ok && mathCalls==8)result^=1;return ok;
 }
};
static uid_t FakeEuid();static int FakeOpen(const char*,int);static int FakeClose(int);static int FakeIoctl(int,unsigned long,void*,size_t);
static int FakeFlush(FILE*);static int FakeError(FILE*);
#define PioneerBlackholeBf16Arithmetic InjectArithmetic
#define geteuid FakeEuid
#define open FakeOpen
#define close FakeClose
#define ioctl FakeIoctl
#define fflush FakeFlush
#define ferror FakeError
#define main CapturedDocumentedMain
#include "CLIENT_SOURCE"
#undef main
#undef ioctl
#undef ferror
#undef fflush
#undef close
#undef open
#undef geteuid
#undef PioneerBlackholeBf16Arithmetic
// ACTUAL driver's metadata control,not a response-shaped fake that echoes
// request selectors. Only its OS lock/team/copy endpoints are replaced.
using status_t=int;using uint32=uint32_t;
enum{B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3,B_BUSY=-4};
constexpr uint64_t PIONEER_DRIVER_BUILD_TAG=PioneerNativeBfpTag;
constexpr unsigned TT_Q38_DISPATCH_DRIVER=55;
struct PioneerProbeCookie{int team=7;void* context=nullptr;}base;
struct thread_info{int team;};
struct Guard{unsigned version=1;}guard;
static Guard* sReloadGuard=&guard;static uint64_t sReloadGeneration=19;
static bool sReloadQuiesced=false,sColdRecoveryRequired=false,locked=false;
static void* sRuntimeOwner=nullptr;static unsigned sOpenCookies=1;static int sLock=1;
static int find_thread(void*){return 1;}
static int get_thread_info(int,thread_info* t){t->team=7;return B_OK;}
static int acquire_sem(int){assert(!locked);locked=true;return B_OK;}
static void release_sem(int){assert(locked);locked=false;}
static int user_memcpy(void* to,const void* from,size_t count){memcpy(to,from,count);return B_OK;}
ACTUAL_RELOAD_CONTROL
static uid_t FakeEuid(){return scenario==22?1:0;}
static int FakeOpen(const char* path,int flags){
 assert(!strcmp(path,"/dev/misc/tenstorrent_pci_probe") && flags==(O_RDWR|O_CLOEXEC) && !opened++ && mathCalls==256);
 return scenario==23?-1:123;
}
static int FakeClose(int fd){assert(fd==123 && !closed++);return scenario==62?-1:0;}
static int FakeFlush(FILE* file){assert(file==stdout && ends==1 && closed==1);int value=fflush(file);return scenario==65?-1:value;}
static int FakeError(FILE* file){assert(file==stdout && ends==1 && closed==1);return scenario==66?1:ferror(file);}
static void Fill(PioneerNativeBfpRequest& r,bool end){
 assert(PioneerNativeBfpRequestValid(r) && r.bits==8 && r.expected_generation==19);
 assert(end?(r.nonce==1 && r.accept==1):(!r.nonce && !r.accept));
 r.nonce=1;auto& v=r.result;v.state=end?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT;
 v.started=v.completed=1;v.polls=1;v.pc[0]=0x70038;v.pc[1]=0x7003c;
 v.phase=0x7000c;v.count=2;v.marker=0x3142464e;
 v.held=v.l1_restored=v.tlb_restored=end;
 for(unsigned word=0;word<64;word++)v.output[word]=Observed[word];
}
static int FakeIoctl(int fd,unsigned long op,void* data,size_t bytes){
 assert(fd==123 && !closed);
 if(op==TT_PCI_PROBE_RELOAD_STATUS){
  assert(bytes==sizeof(tt_pioneer_driver_reload) && !queries++ && !supports && !runs && !ends);
  auto& m=*static_cast<tt_pioneer_driver_reload*>(data);assert(m.version==1 && m.expected_tag==PioneerNativeBfpTag);
  assert(!DriverReloadControl(&base,op,&m,bytes) && !locked && !m.expected_tag && !m.expected_generation);
  if(scenario==2)m.cold=1;if(scenario==3)m.build_tag^=1;if(scenario==4)m.generation=0;
  if(scenario==5)m.active_owner=1;if(scenario==6)m.open_count=2;if(scenario==7)m.quiesced=1;
  if(scenario==8)m.guard_version=0;if(scenario==9)m.driver=54;if(scenario==10)m.version=2;
  if(scenario==67)m.expected_tag=PioneerNativeBfpTag;if(scenario==68)m.expected_generation=19;
  if(scenario>=69 && scenario<=71)m.reserved[scenario-69]=1;
  return scenario==11?-1:0;
 }
 assert(bytes==sizeof(PioneerNativeBfpRequest));auto& r=*static_cast<PioneerNativeBfpRequest*>(data);
 if(op==PIONEER_NATIVE_BFP_SUPPORT){
  assert(queries==1 && !supports++ && !runs && !ends && PioneerNativeBfpRequestValid(r) && !r.bits && !r.nonce && !r.accept);
  if(scenario==13)r.result.output[0]=1;if(scenario==14)r.bits=8;if(scenario==15)r.nonce=1;
  if(scenario==16)r.accept=1;if(scenario==17)r.expected_generation=20;if(scenario==18)r.reserved=1;if(scenario==19)r.expected_tag^=1;
  return scenario==12?-1:0;
 }
 if(op==PIONEER_NATIVE_BFP_RUN){
  assert(supports==1 && !runs++ && !ends);Fill(r,false);
  if(scenario==1)r.result.output[failedWord]^=1;
  if(scenario==25)r.result.state=PIONEER_NATIVE_BFP_CLOSED;if(scenario==26)r.result.cold=1;
  if(scenario==27)r.result.started=0;if(scenario==28)r.result.started=2;
  if(scenario==29)r.result.completed=0;if(scenario==30)r.result.completed=2;
  if(scenario==31)r.result.polls=0;if(scenario==32)r.result.polls=20001;
  if(scenario==33)r.result.pc[0]=0x70040;if(scenario==34)r.result.pc[1]=0x70040;
  if(scenario==35)r.result.phase^=1;if(scenario==36)r.result.count=1;if(scenario==37)r.result.marker^=1;
  if(scenario==38)r.result.held=1;if(scenario==39)r.result.l1_restored=1;if(scenario==40)r.result.tlb_restored=1;
  if(scenario==41)r.nonce=2;if(scenario==42)r.reserved=1;if(scenario==43)r.accept=1;
  if(scenario==44)r.bits=4;if(scenario==45)r.expected_tag^=1;if(scenario==46)r.expected_generation=20;if(scenario==47)r.version=2;
  return scenario==24?-1:0;
 }
 assert(op==PIONEER_NATIVE_BFP_END && runs==1 && !ends++ && (scenario==0 || (scenario>=48 && scenario<=62) || scenario==65 || scenario==66));
 Fill(r,true);
 if(scenario==49)r.result.output[failedWord]^=1;
 if(scenario==50)r.result.held=0;if(scenario==51)r.result.l1_restored=0;if(scenario==52)r.result.tlb_restored=0;
 if(scenario==53)r.result.cold=1;if(scenario==54)r.nonce=2;if(scenario==55)r.result.marker^=1;
 if(scenario==56)r.version=2;if(scenario==57)r.accept=0;if(scenario==58)r.reserved=1;
 if(scenario==59)r.expected_tag^=1;if(scenario==60)r.expected_generation=20;if(scenario==61)r.result.state=PIONEER_NATIVE_BFP_RESULT;
 return scenario==48?-1:0;
}
int main(int argc,char** argv){
 assert(argc==3);scenario=unsigned(atoi(argv[1]));failedWord=unsigned(atoi(argv[2]));assert(scenario<=71 && failedWord<64);
 for(unsigned row=0;row<32;row++)assert(Golden[row]==Observed[row]);
 char name[]="new-diagnostic",option[]="--fixed-bfp8-documented-rounding",oldOption[]="--fixed-bfp8";
 char* command[]={name,scenario==64?oldOption:option,nullptr};
 const int result=CapturedDocumentedMain(scenario==63?1:2,command);
 assert(result==(scenario==0?0:(scenario==63 || scenario==64)?2:1));
 bool noOpen=(scenario>=20 && scenario<=22) || scenario==63 || scenario==64;
 assert(opened==unsigned(!noOpen));
 assert(closed==unsigned(!noOpen && scenario!=23));
 assert(queries==unsigned(!noOpen && scenario!=23));
 assert(supports==unsigned(!noOpen && scenario!=23 && !(scenario>=2 && scenario<=11) && scenario<67));
 assert(runs==unsigned(!noOpen && scenario!=23 && !(scenario>=2 && scenario<=19) && scenario<67));
 assert(ends==unsigned(scenario==0 || (scenario>=48 && scenario<=62) || scenario==65 || scenario==66));
 assert(mathCalls==(scenario==20?1:scenario==21?8:scenario==22 || scenario==63 || scenario==64?0:256));
 return 0;
}
'''.replace('ARITHMETIC_HEADER', str(here.parent / 'PioneerBlackholeBf16Arithmetic.h'))
program = program.replace('CLIENT_SOURCE', str(here.parent / 'pioneer_native_bfp8_documented_diagnostic.cpp'))
program = program.replace('ACTUAL_RELOAD_CONTROL', reload_control)
program = program.replace('OBSERVED_WORDS', ','.join(f'0x{x:08x}' for x in observed))
(folder / 'test.cpp').write_text(program)
compiler = os.environ.get('CXX', 'c++')
subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I' + str(folder), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
cases = 0
for scenario in range(72):
    for index in (range(64) if scenario in (1, 49) else (0,)):
        result = subprocess.run([str(folder / 'test'), str(scenario), str(index)], text=True, capture_output=True,
                                env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
        assert result.returncode == 0, (scenario, index, result.stdout, result.stderr)
        if scenario in (0, 65, 66):
            assert not result.stderr
            lines = result.stdout.splitlines()
            assert len(lines) == 65 and lines[-1].startswith('PASS_NATIVE_BFP8_DOCUMENTED_FIXED_KERNEL ')
            records = lines[:64]
        elif scenario == 1:
            assert not result.stdout
            lines = result.stderr.splitlines()
            assert len(lines) == 65 and f'pass{index//32} row{index%32} ' in lines[0]
            records = lines[1:]
        else:
            assert not result.stdout and 'DOCUMENTED_RETURNED_WORD' not in result.stderr
            assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr
            records = []
        for word, line in enumerate(records):
            match = re.fullmatch(r'NATIVE_BFP8_DOCUMENTED_RETURNED_WORD pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
            assert match and tuple(map(int, match.group(1, 2))) == (word // 32, word % 32)
            got, expected = (int(x, 16) for x in match.group(3, 4))
            assert expected == observed[word] and got == (expected ^ int(scenario == 1 and word == index))
        cases += 1
for selection in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    check = subprocess.run([compiler, '-std=c++17', '-D' + selection, '-I' + str(folder), '-c', str(here.parent / 'pioneer_native_bfp8_documented_diagnostic.cpp'),
                            '-o', str(folder / (selection + '.o'))], text=True, capture_output=True)
    assert check.returncode and 'Unqualified Blackhole arithmetic characterization is offboard only' in check.stderr
print(f'PASS ACTUAL separate documented client:{cases} ASan/UBSan cases;ACTUAL driver metadata control clears request selectors,independent recorded positives,all64 RUN/END word failures,pre-open spec/golden checks,protocol/owner/metadata/close/CLI failures,NO END/query/retry after failed RUN;3 runtime-role guards;NO device proof')
print('CPU-only actual documented client evidence:', folder)
