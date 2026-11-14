#!/usr/bin/env python3
"""Actual three-sample client+driver metadata;fake source/device endpoints only."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
if not __debug__ or len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2] not in ('--qgwd-synthetic','--recurrent-synthetic','--recurrent-out-synthetic','--small-alpha','--small-beta')):
    raise SystemExit('Use non-optimized Python PRIVATE_EXPORT_DIRECTORY [--qgwd-synthetic|--recurrent-synthetic|--recurrent-out-synthetic|--small-alpha|--small-beta]')
here = pathlib.Path(__file__).resolve().parent
private = pathlib.Path(sys.argv[1])
small_alpha=len(sys.argv)==3 and sys.argv[2]=='--small-alpha'
small_beta=len(sys.argv)==3 and sys.argv[2]=='--small-beta'
synthetic=len(sys.argv)==3 and not (small_alpha or small_beta)
recurrent=synthetic and sys.argv[2]=='--recurrent-synthetic'
recurrent_out=synthetic and sys.argv[2]=='--recurrent-out-synthetic'
if synthetic:
    assert private.resolve()==(here/('fixtures/recurrent-out-source' if recurrent_out else 'fixtures/qgwd-dense-source')).resolve(), 'only explicit synthetic metadata'
elif small_alpha or small_beta:
    expected='630caa504d3b3fc96b2aa38ec090954c1cbb470b2baa59d84957af4e7d4915e1' if small_alpha else '70940f99172a098a48ab18b978e9183a6ade1e1543f16fb06f42fa6d993421e4'
    assert hashlib.sha256((private / 'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest() == expected
else:
    assert hashlib.sha256((private / 'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest() == 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'
driver = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
control = driver.split('// PIONEER_DRIVER_RELOAD_CONTROL_BEGIN\n', 1)[1].split('// PIONEER_DRIVER_RELOAD_CONTROL_END', 1)[0]
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-learned-band-client.'))
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
#include "PioneerNativeModelBandSource.h"
static unsigned scenario=0,target=0,failedWord=0,opened=0,closed=0,queries=0,supports=0,runs=0,ends=0,prepared=0,readyCalls=0,activeReady=0,flushes=0;
static bool poisoned=false;
struct FakeSource {
 bool Prepare(const char* path,std::string& error){assert(!prepared++ && !opened && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf"));error="fake-source";return scenario!=60;}
 bool Ready(){
  readyCalls++;
  if(scenario==61 && readyCalls==1)return false;
  if(scenario==62 && readyCalls==2)return false;
  if(scenario==63 && supports==target+1 && runs==target && ends==target)return false;
  if(runs==target+1 && ends==target){activeReady++;if(scenario==21 || (scenario==22 && activeReady==2))return false;}
  if(scenario==45 && ends==target+1)return false;
  return true;
 }
};
static uid_t FakeEuid();static int FakeOpen(const char*,int);static int FakeClose(int);static int FakeIoctl(int,unsigned long,void*,size_t);
static int FakeFlush(FILE*);static int FakeError(FILE*);
#define PioneerNativeModelBandSource FakeSource
#define geteuid FakeEuid
#define open FakeOpen
#define close FakeClose
#define ioctl FakeIoctl
#define fflush FakeFlush
#define ferror FakeError
#define main CapturedLearnedMain
#include "CLIENT_SOURCE"
#undef main
#undef ferror
#undef fflush
#undef ioctl
#undef close
#undef open
#undef geteuid
#undef PioneerNativeModelBandSource
using status_t=int;using uint32=uint32_t;
enum{B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3,B_BUSY=-4};
constexpr uint64_t PIONEER_DRIVER_BUILD_TAG=PioneerNativeModelBandTag;
constexpr unsigned TT_Q38_DISPATCH_DRIVER=55;
struct PioneerProbeCookie{int team=7;void* context=nullptr;}base;
struct thread_info{int team;};struct Guard{unsigned version=1;}guard;
static Guard* sReloadGuard=&guard;static uint64_t sReloadGeneration=19;
static bool sReloadQuiesced=false,sColdRecoveryRequired=false,locked=false;
static void* sRuntimeOwner=nullptr;static unsigned sOpenCookies=1;static int sLock=1;
static int find_thread(void*){return 1;}
static int get_thread_info(int,thread_info* t){t->team=7;return B_OK;}
static int acquire_sem(int){assert(!locked);locked=true;return B_OK;}
static void release_sem(int){assert(locked);locked=false;}
static int user_memcpy(void* to,const void* from,size_t count){memcpy(to,from,count);return B_OK;}
ACTUAL_RELOAD_CONTROL
static uid_t FakeEuid(){return scenario==70?1:0;}
static int FakeOpen(const char* path,int flags){
 assert(!poisoned && prepared==1 && opened==closed && opened<3 && !strcmp(path,"/dev/misc/tenstorrent_pci_probe") && flags==(O_RDWR|O_CLOEXEC));
 return 123+int(opened++);
}
static int FakeClose(int fd){assert(opened==closed+1 && fd==123+int(closed));closed++;return scenario==46 && closed==target+1?-1:0;}
static int FakeFlush(FILE* file){
 assert(file==stdout);flushes++;const bool fail=(scenario==50 && flushes==1)||(scenario==51 && flushes==2)
  ||(scenario==52 && closed==target+1 && flushes==target+3)||(scenario==53 && flushes==6);
 const int result=fflush(file);return fail?-1:result;
}
static int FakeError(FILE* file){assert(file==stdout);return ferror(file);}
static void Fill(PioneerNativeModelBandRequest& r,bool end){
 const unsigned sample=opened-1;assert(PioneerNativeModelBandRequestValid(r) && r.sample==sample && r.expected_generation==19);
 assert(end?(r.nonce==1 && r.accept==1):(!r.nonce && !r.accept));
 r.nonce=1;auto& v=r.result;v.state=end?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT;
 v.started=v.completed=1;v.polls=1;v.pc[0]=0x70038;v.pc[1]=0x7003c;v.phase=0x7000c;v.count=0x04000000U|PioneerNativeBfpModelBandMetadata::Width;v.marker=0x3342464e;
 v.held=v.l1_restored=v.tlb_restored=end;
 for(unsigned word=0;word<64;word++)v.output[word]=PioneerNativeBfpModelBandMetadata::Golden(sample)[word%32];
}
static int FakeIoctl(int fd,unsigned long op,void* data,size_t bytes){
 assert(!poisoned && opened==closed+1 && fd==123+int(opened-1));const unsigned sample=opened-1;const bool fail=sample==target;
 if(op==TT_PCI_PROBE_RELOAD_STATUS){
  assert(bytes==sizeof(tt_pioneer_driver_reload) && queries++==sample && supports==sample && runs==sample && ends==sample);
  auto& m=*static_cast<tt_pioneer_driver_reload*>(data);assert(m.version==1 && m.expected_tag==PioneerNativeModelBandTag);
  assert(!DriverReloadControl(&base,op,&m,bytes) && !locked && !m.expected_tag && !m.expected_generation);
  if(fail && scenario==1)m.build_tag^=1;
  if(fail && (scenario==1 || scenario==2)){poisoned=true;return scenario==2?-1:0;}return 0;
 }
 assert(bytes==sizeof(PioneerNativeModelBandRequest));auto& r=*static_cast<PioneerNativeModelBandRequest*>(data);
 if(op==PIONEER_NATIVE_MODEL_BAND_SUPPORT){
  assert(queries==sample+1 && supports++==sample && runs==sample && ends==sample && PioneerNativeModelBandRequestValid(r) && r.sample==PioneerNativeModelBandNone && !r.nonce && !r.accept);
  if(fail && scenario==3)r.sample=0;
  if(fail && (scenario==3 || scenario==4)){poisoned=true;return scenario==4?-1:0;}return 0;
 }
 if(op==PIONEER_NATIVE_MODEL_BAND_RUN){
  assert(supports==sample+1 && runs++==sample && ends==sample);Fill(r,false);
  if(fail){
   if(scenario==6)r.result.started=2;if(scenario==7)r.result.cold=1;if(scenario==8)r.result.state=2;
   if(scenario==9)r.result.pc[0]=0x70040;if(scenario==10)r.result.polls=0;if(scenario==11)r.result.count^=1;
   if(scenario==12)r.result.marker=0x3142464e;if(scenario==13)r.sample=(sample+1)%3;if(scenario==14)r.expected_tag^=1;
   if(scenario==15)r.nonce=2;if(scenario==16)r.reserved=1;if(scenario==17)r.accept=1;
   if(scenario==20)r.result.output[failedWord]^=1;
   if((scenario>=5 && scenario<=17)||scenario==20||scenario==21||scenario==22)poisoned=true;
  }
  return fail && scenario==5?-1:0;
 }
 assert(op==PIONEER_NATIVE_MODEL_BAND_END && runs==sample+1 && ends++==sample);Fill(r,true);
 if(fail){
  if(scenario==31)r.result.output[failedWord]^=1;if(scenario==32)r.result.held=0;if(scenario==33)r.result.l1_restored=0;
  if(scenario==34)r.result.tlb_restored=0;if(scenario==35)r.result.cold=1;if(scenario==36)r.nonce=2;
  if(scenario==37)r.accept=0;if(scenario==38)r.reserved=1;if(scenario==39)r.expected_generation=20;
  if(scenario==40)r.result.state=PIONEER_NATIVE_BFP_RESULT;
  if(scenario>=30 && scenario<=45)poisoned=true;
 }
 return fail && scenario==30?-1:0;
}
int main(int argc,char** argv){
 assert(argc==4);scenario=unsigned(atoi(argv[1]));target=unsigned(atoi(argv[2]));failedWord=unsigned(atoi(argv[3]));assert(target<3 && failedWord<64);
 char name[]="learned-client",option[]="--fixed-learned-bands",bad[]="--fixed-k5120";char* command[]={name,scenario==80?bad:option,nullptr};
 const int result=CapturedLearnedMain(2,command);assert(result==(scenario==0?0:scenario==80?2:1));
 if(scenario==0 || scenario==53){assert(opened==3 && closed==3 && queries==3 && supports==3 && runs==3 && ends==3);}
 else if(scenario==70 || scenario==80 || scenario==50){assert(!prepared && !opened && !closed && !queries && !supports && !runs && !ends);}
 else if(scenario==51 || scenario==60 || scenario==61 || scenario==62){assert(prepared==1 && !opened && !closed && !queries && !supports && !runs && !ends);}
 else{
  assert(opened==target+1 && closed==opened && queries==opened);
  assert(supports==target+unsigned(scenario!=1 && scenario!=2));
  const unsigned wantRun=target+unsigned(scenario!=1 && scenario!=2 && scenario!=3 && scenario!=4 && scenario!=63);
  const unsigned wantEnd=target+unsigned((scenario>=30 && scenario<=46)||scenario==52);
  assert(runs==wantRun && ends==wantEnd);
 }
 return 0;
}
'''.replace('CLIENT_SOURCE', str(here.parent / 'pioneer_native_model_band_diagnostic.cpp')).replace('ACTUAL_RELOAD_CONTROL', control)
(folder / 'test.cpp').write_text(program)
compiler = os.environ.get('CXX', 'c++')
roles = (['-DPIONEER_NATIVE_SMALL_ALPHA_BAND_DIAGNOSTIC=1','-DPIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE=1'] if small_alpha else
         ['-DPIONEER_NATIVE_SMALL_BETA_BAND_DIAGNOSTIC=1','-DPIONEER_NATIVE_SMALL_BETA_BAND_SOURCE=1'] if small_beta else
         ['-DPIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC=1','-DPIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE=1'] if recurrent_out else
         ['-DPIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC=1','-DPIONEER_NATIVE_RECURRENT_BAND_SOURCE=1'] if recurrent else
         ['-DPIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC=1','-DPIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE=1'] if synthetic else [])
subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                *roles,
                '-I' + str(folder), '-I' + str(here.parent), '-I' + str(private), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
cases = [(0, 0, 0)]
for scenario in (*range(1, 18), 20, 21, 22, *range(30, 41), 45, 46, 52, 63):
    for target in range(3):
        for word in (range(64) if scenario in (20, 31) else (0,)):
            cases.append((scenario, target, word))
for scenario in (50, 51, 53, 60, 61, 62, 70, 80):
    cases.append((scenario, 0, 0))
for scenario, target, word in cases:
    result = subprocess.run([str(folder / 'test'), str(scenario), str(target), str(word)], text=True, capture_output=True,
                            env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    assert result.returncode == 0, (scenario, target, word, result.stdout, result.stderr)
    assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr
    if scenario == 0:
        assert not result.stderr and result.stdout.count('NATIVE_LEARNED_BAND_RETURNED_WORD') == 192 and result.stdout.count('PASS_NATIVE_LEARNED_BAND sample') == 3
    elif scenario == 20:
        assert result.stderr.count('NATIVE_LEARNED_BAND_RETURNED_WORD') == 64 and f'sample{target} pass{word//32} row{word%32} ' in result.stderr
for selection in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    result = subprocess.run([compiler, '-std=c++17', '-D' + selection,
                             *roles,
                             '-I' + str(folder), '-I' + str(private),
                             '-c', str(here.parent / 'pioneer_native_model_band_diagnostic.cpp'), '-o', str(folder / (selection + '.o'))], text=True, capture_output=True)
    assert result.returncode and 'Learned-band CPU reference is a separate diagnostic' in result.stderr
if small_alpha or small_beta:
    wrong = 'PIONEER_NATIVE_SMALL_BETA_BAND_DIAGNOSTIC' if small_alpha else 'PIONEER_NATIVE_SMALL_ALPHA_BAND_DIAGNOSTIC'
    result = subprocess.run([compiler, '-std=c++17', *roles, '-D'+wrong+'=1', '-I'+str(folder), '-I'+str(private),
                             '-E', str(here.parent/'pioneer_native_model_band_diagnostic.cpp')],capture_output=True,text=True)
    assert result.returncode and 'distinct identities' in result.stderr
    result = subprocess.run([compiler, '-std=c++17', roles[0], '-I'+str(folder), '-I'+str(private),
                             '-E', str(here.parent/'pioneer_native_model_band_diagnostic.cpp')],capture_output=True,text=True)
    assert result.returncode and 'authenticated source must be selected together' in result.stderr
print(f'PASS ACTUAL learned-band client:{len(cases)} ASan/UBSan cases;ACTUAL driver metadata,all64 RUN/END word mutations in ALL3 samples,source mutation at boundaries,strict protocol/owner/close/channel failures,NO next sample/END/query/retry after uncertainty;3 runtime-role guards;NO device proof')
print('CPU-only actual learned client evidence:', folder)
