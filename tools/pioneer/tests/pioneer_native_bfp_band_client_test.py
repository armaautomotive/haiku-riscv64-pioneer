#!/usr/bin/env python3
"""Execute actual full-K client+driver metadata with fake endpoints;NO card."""
import hashlib
import os
import pathlib
import re
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
record = (here.parent / 'benchmarks/qwen38-native-bfp-band-owner-20261104/full-k-owner-r2.log').read_bytes()
assert hashlib.sha256(record).hexdigest() == 'e16f4282406ea8f21ea3b7bdf3f9100482e3eae1b9fc2e16cbcf4b1c7a3e00cb'
observed = [int(x, 16) for x in re.findall(rb'BAND_LEASE_SYNTHETIC_REFERENCE k(?:5120|6144|17408) row[0-9]+ word([0-9a-f]{8})', record)]
assert len(observed) == 96
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
static unsigned selectedWidth=5120;
const uint32_t Observed[96]={OBSERVED_WORDS}; // independent immutable physical record
struct InjectArithmetic {
 static bool Instruction(const uint16_t (&a)[16],const uint16_t (&b)[16],unsigned phase,uint32_t prior,uint32_t& result){
  mathCalls++;if(scenario==20)return false;
  bool ok=PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,prior,result);
  if(scenario==21 && ok && mathCalls==selectedWidth/4)result^=1;return ok;
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
constexpr uint64_t PIONEER_DRIVER_BUILD_TAG=PioneerNativeBfpBandTag;
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
 assert(!strcmp(path,"/dev/misc/tenstorrent_pci_probe") && flags==(O_RDWR|O_CLOEXEC) && !opened++ && mathCalls==selectedWidth*8);
 return scenario==23?-1:123;
}
static int FakeClose(int fd){assert(fd==123 && !closed++);return scenario==62?-1:0;}
static int FakeFlush(FILE* file){assert(file==stdout && ends==1 && closed==1);int value=fflush(file);return scenario==65?-1:value;}
static int FakeError(FILE* file){assert(file==stdout && ends==1 && closed==1);return scenario==66?1:ferror(file);}
static void Fill(PioneerNativeBfpBandRequest& r,bool end){
 assert(PioneerNativeBfpBandRequestValid(r) && r.width==selectedWidth && r.expected_generation==19);
 assert(end?(r.nonce==1 && r.accept==1):(!r.nonce && !r.accept));
 r.nonce=1;auto& v=r.result;v.state=end?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT;
 v.started=v.completed=1;v.polls=1;v.pc[0]=0x70038;v.pc[1]=0x7003c;
 v.phase=0x7000c;v.count=0x04000000|selectedWidth;v.marker=0x3342464e;
 v.held=v.l1_restored=v.tlb_restored=end;
 for(unsigned word=0;word<64;word++)v.output[word]=Observed[(selectedWidth==5120?0:selectedWidth==6144?32:64)+word%32];
}
static int FakeIoctl(int fd,unsigned long op,void* data,size_t bytes){
 assert(fd==123 && !closed);
 if(op==TT_PCI_PROBE_RELOAD_STATUS){
  assert(bytes==sizeof(tt_pioneer_driver_reload) && !queries++ && !supports && !runs && !ends);
  auto& m=*static_cast<tt_pioneer_driver_reload*>(data);assert(m.version==1 && m.expected_tag==PioneerNativeBfpBandTag);
  assert(!DriverReloadControl(&base,op,&m,bytes) && !locked && !m.expected_tag && !m.expected_generation);
  if(scenario==2)m.cold=1;if(scenario==3)m.build_tag^=1;if(scenario==4)m.generation=0;
  if(scenario==5)m.active_owner=1;if(scenario==6)m.open_count=2;if(scenario==7)m.quiesced=1;
  if(scenario==8)m.guard_version=0;if(scenario==9)m.driver=54;if(scenario==10)m.version=2;
  if(scenario==67)m.expected_tag=PioneerNativeBfpBandTag;if(scenario==68)m.expected_generation=19;
  if(scenario>=69 && scenario<=71)m.reserved[scenario-69]=1;
  return scenario==11?-1:0;
 }
 assert(bytes==sizeof(PioneerNativeBfpBandRequest));auto& r=*static_cast<PioneerNativeBfpBandRequest*>(data);
 if(op==PIONEER_NATIVE_BFP_BAND_SUPPORT){
  assert(queries==1 && !supports++ && !runs && !ends && PioneerNativeBfpBandRequestValid(r) && !r.width && !r.nonce && !r.accept);
  if(scenario==13)r.result.output[0]=1;if(scenario==14)r.width=8;if(scenario==15)r.nonce=1;
  if(scenario==16)r.accept=1;if(scenario==17)r.expected_generation=20;if(scenario==18)r.reserved=1;if(scenario==19)r.expected_tag^=1;
  return scenario==12?-1:0;
 }
 if(op==PIONEER_NATIVE_BFP_BAND_RUN){
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
  if(scenario==44)r.width=4;if(scenario==45)r.expected_tag^=1;if(scenario==46)r.expected_generation=20;if(scenario==47)r.version=2;
  return scenario==24?-1:0;
 }
 assert(op==PIONEER_NATIVE_BFP_BAND_END && runs==1 && !ends++ && (scenario==0 || (scenario>=48 && scenario<=62) || scenario==65 || scenario==66));
 Fill(r,true);
 if(scenario==49)r.result.output[failedWord]^=1;
 if(scenario==50)r.result.held=0;if(scenario==51)r.result.l1_restored=0;if(scenario==52)r.result.tlb_restored=0;
 if(scenario==53)r.result.cold=1;if(scenario==54)r.nonce=2;if(scenario==55)r.result.marker^=1;
 if(scenario==56)r.version=2;if(scenario==57)r.accept=0;if(scenario==58)r.reserved=1;
 if(scenario==59)r.expected_tag^=1;if(scenario==60)r.expected_generation=20;if(scenario==61)r.result.state=PIONEER_NATIVE_BFP_RESULT;
 return scenario==48?-1:0;
}
int main(int argc,char** argv){
 assert(argc==4);selectedWidth=unsigned(atoi(argv[3]));assert(selectedWidth==5120 || selectedWidth==6144 || selectedWidth==17408);scenario=unsigned(atoi(argv[1]));failedWord=unsigned(atoi(argv[2]));assert(scenario<=71 && failedWord<64);
 for(unsigned row=0;row<32;row++)assert(PioneerNativeBfpBandGolden(selectedWidth)[row]==Observed[(selectedWidth==5120?0:selectedWidth==6144?32:64)+row]);
 char name[]="full-K-diagnostic",option[32],oldOption[]="--fixed-bfp8";snprintf(option,sizeof(option),"--fixed-k%u",selectedWidth);
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
 assert(mathCalls==(scenario==20?1:scenario==21?selectedWidth/4:scenario==22 || scenario==63 || scenario==64?0:selectedWidth*8));
 return 0;
}
'''.replace('ARITHMETIC_HEADER', str(here.parent / 'PioneerBlackholeBf16Arithmetic.h'))
program = program.replace('CLIENT_SOURCE', str(here.parent / 'pioneer_native_bfp_band_diagnostic.cpp'))
program = program.replace('ACTUAL_RELOAD_CONTROL', reload_control)
program = program.replace('OBSERVED_WORDS', ','.join(f'0x{x:08x}' for x in observed))
(folder / 'test.cpp').write_text(program)
compiler = os.environ.get('CXX', 'c++')
subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I' + str(folder), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
cases = 0
for scenario in range(72):
    for index in (range(64) if scenario in (1, 49) else (0,)):
        result = subprocess.run([str(folder / 'test'), str(scenario), str(index), '5120'], text=True, capture_output=True,
                                env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
        assert result.returncode == 0, (scenario, index, result.stdout, result.stderr)
        if scenario in (0, 65, 66):
            assert not result.stderr
            lines = result.stdout.splitlines()
            assert len(lines) == 65 and lines[-1].startswith('PASS_NATIVE_BFP_FULL_K ')
            records = lines[:64]
        elif scenario == 1:
            assert not result.stdout
            lines = result.stderr.splitlines()
            assert len(lines) == 65 and f'pass{index//32} row{index%32} ' in lines[0]
            records = lines[1:]
        else:
            assert not result.stdout and 'BAND_RETURNED_WORD' not in result.stderr
            assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr
            records = []
        for word, line in enumerate(records):
            match = re.fullmatch(r'NATIVE_BFP_BAND_RETURNED_WORD k5120 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
            assert match and tuple(map(int, match.group(1, 2))) == (word // 32, word % 32)
            got, expected = (int(x, 16) for x in match.group(3, 4))
            assert expected == observed[word%32] and got == (expected ^ int(scenario == 1 and word == index))
        cases += 1
for width, offset in ((6144, 32), (17408, 64)):
    result = subprocess.run([str(folder / 'test'), '0', '0', str(width)], text=True, capture_output=True,
                            env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    assert result.returncode == 0 and not result.stderr, (width, result.stdout, result.stderr)
    records = re.findall(r'NATIVE_BFP_BAND_RETURNED_WORD k' + str(width) + r' pass[01] row[0-9]+ got([0-9a-f]{8}) expected[0-9a-f]{8}', result.stdout)
    assert len(records) == 64 and [int(word, 16) for word in records] == observed[offset:offset + 32] * 2
    cases += 1
for selection in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    check = subprocess.run([compiler, '-std=c++17', '-D' + selection, '-I' + str(folder), '-c', str(here.parent / 'pioneer_native_bfp_band_diagnostic.cpp'),
                            '-o', str(folder / (selection + '.o'))], text=True, capture_output=True)
    assert check.returncode and 'Unqualified Blackhole arithmetic characterization is offboard only' in check.stderr
print(f'PASS ACTUAL full-K client:{cases} ASan/UBSan cases;ACTUAL driver metadata control clears request selectors,independent Fraction-qualified synthetic positives,all64 RUN/END word failures,pre-open spec/golden checks,protocol/owner/metadata/close/CLI failures,NO END/query/retry after failed RUN;3 runtime-role guards;NO device proof')
print('CPU-only actual full-K client evidence:', folder)
