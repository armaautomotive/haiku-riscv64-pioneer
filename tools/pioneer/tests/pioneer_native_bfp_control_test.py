#!/usr/bin/env python3
"""ACTUAL native Control/Reply/Fault/Dispose/Free with CPU-only endpoint failures."""
import os
import pathlib
import subprocess
import sys
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
assert len(sys.argv)==1 or sys.argv[1:] in (['--band'], ['--model'], ['--pair'], ['--exact-q4'], ['--exact-q4-affine'], ['--srca'])
affine=sys.argv[1:]==['--exact-q4-affine']
srca=sys.argv[1:]==['--srca']
exact = sys.argv[1:] in (['--exact-q4'],['--exact-q4-affine'],['--srca'])
pair = sys.argv[1:]==['--pair']
band = sys.argv[1:] in (['--band'], ['--model'], ['--pair'])
model = sys.argv[1:] in (['--model'], ['--pair'])
name = 'pioneer_native_model_band_driver.cpp' if model else 'pioneer_native_bfp_band_driver.cpp' if band else 'pioneer_native_bfp_driver.cpp'
native = here.parents[2] / 'src/add-ons/kernel/drivers/misc' / name
source = native.read_text()
def extract(a, b):
    start = source.index(a)
    return source[start:source.index(b, start)]
code = extract('static void Fault(', 'static bool FinishMmio(')
if not band:
    alias_begin = source.index('#ifdef PIONEER_NATIVE_SRCA_PROBE_DIAGNOSTIC', source.index('namespace {'))
    code = source[alias_begin:source.index('struct NativeCookie', alias_begin)] + code
code += extract('static void DisposeKnown(', 'static status_t Open(')
code += extract('static status_t Free(', 'static status_t Control(')
code += source[source.index('static status_t Control('):source.rindex('\n}\ndevice_hooks* find_device')]
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <new>
#define B_DEVICE_OP_CODES_END 10000
#include "PioneerNativeBfpProtocol.h"
using uint32=uint32_t;using status_t=int;
enum{B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_BUSY=-3,B_NO_MEMORY=-4,B_DEVICE_NOT_READY=-5};
enum{TT_PCI_PROBE_RELOAD_STATUS=10,TT_PCI_PROBE_RELOAD_QUIESCE=11};
static unsigned uid=0,team=7,copyCalls=0,failCopy=0,reserves=0,unreserves=0,runs=0,ends=0,finishes=0,deleted=0,baselines=0;
static int threadError=0,profileError=0,moduleError=0,reserveError=0,lockError=0;
static bool quiesced=false,profileValid=true,mapped=true,baseline=true,baselineClean=true,finishOK=true,endOK=true,mutateInput=false,teardown=false;
static unsigned runState=PIONEER_NATIVE_BFP_RESULT;
static bool locked=false,sColdRecoveryRequired=false;static int sLock=1;static uint32 sOpenCookies=1;
static uint64_t sReloadGeneration=19;struct Guard{unsigned version=1;}guard;static Guard* sReloadGuard=&guard;
struct PioneerProbeCookie{unsigned magic=0x50313030,team=7;void* context=nullptr;};
struct NativeCookie{PioneerProbeCookie* base=nullptr;bool used=false;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
struct thread_info{unsigned team;};
static unsigned geteuid(){return uid;}static int find_thread(void*){return 1;}
static int get_thread_info(int,thread_info* t){t->team=team;return threadError;}
static int acquire_sem(int){if(lockError)return lockError;assert(!locked);locked=true;return 0;}
static void release_sem(int){assert(locked);locked=false;}
static int AcquireProbeLock(PioneerProbeCookie* cookie){
 if(sColdRecoveryRequired)return B_DEVICE_NOT_READY;
 if(quiesced || (sRuntimeOwner && sRuntimeOwner!=cookie))return B_BUSY;
 return acquire_sem(1);
}
static int user_memcpy(void* to,const void* from,size_t n){
 if(++copyCalls==failCopy)return -90;memcpy(to,from,n);
 if(copyCalls==1 && mutateInput)const_cast<PioneerNativeBfpRequest*>(static_cast<const PioneerNativeBfpRequest*>(from))->bits=8;
 return 0;
}
struct module_info{};static module_info module;static module_info* sDeviceManager=nullptr;
constexpr const char* B_DEVICE_MANAGER_MODULE_NAME="fake";
static int get_module(const char*,module_info** m){*m=&module;return moduleError;}
struct Card{unsigned bus=3,device=0,function=0;}sCard;static char sCookie;
constexpr const char* TT_PCI_PROBE_DEVICE="fake-fixed-device";
struct PCI{int reserve_device(...){reserves++;return reserveError;}void unreserve_device(...){assert(!sColdRecoveryRequired);unreserves++;}}pci;
static PCI* sPCI=&pci;
struct tt_pioneer_profile{int status=0;};
static int GetPioneerPciProfile(module_info*,Card&,tt_pioneer_profile&){return profileError;}
static bool PioneerMinimumBarLayoutMatches(const tt_pioneer_profile&){return profileValid;}
struct tt_pioneer_mmio_result{uint32_t node_id[3]={};};
struct tt_pioneer_mmio_transaction{bool config_attempted=false,map_attempted=false;};
struct PioneerComputeNocIO{bool AfterNodeId(uint32_t){baselines++;return baseline;}};
struct NativeIO:PioneerComputeNocIO{bool BaselineClean(){return baselineClean;}};
struct PioneerNativeBfpGeneratedImage{};
struct FakeLease{
 PioneerNativeBfpResult r={};bool owned=false;
 template<class A,class B>bool Run(A&,int&,unsigned){runs++;owned=true;r.state=runState;r.started=1;r.completed=runState==1;return r.state==1;}
 bool End(NativeIO&,int&){ends++;if(!endOK){r.state=4;r.cold=1;return false;}owned=false;r.state=2;return true;}
 void Abandon(){if(owned){r.state=4;r.cold=1;}}
 const auto& Result()const{return r;}
};
struct NativeOwner{
 NativeCookie* cookie;unsigned bits;NativeIO io;FakeLease lease;tt_pioneer_mmio_result mmio;tt_pioneer_mmio_transaction transaction;int dram=0;bool reserved=false;
 NativeOwner(NativeCookie* c,unsigned b,const tt_pioneer_profile&):cookie(c),bits(b){}
 ~NativeOwner(){assert(!sColdRecoveryRequired || teardown);deleted++;}
};
static NativeOwner* owner=nullptr;
static bool BlackholePioneerMmioBegin(NativeIO&,tt_pioneer_mmio_result&,tt_pioneer_mmio_transaction& t){t={true,true};return mapped;}
static bool FinishMmio(NativeOwner&){finishes++;return finishOK;}
static int DriverReloadControl(PioneerProbeCookie*,uint32,void*,size_t){return 0;}
static int ProbeFree(PioneerProbeCookie* p){assert(!locked);delete p;sOpenCookies--;return 0;}
'''
tests = r'''
static NativeCookie* Cookie(){auto* c=new NativeCookie;c->base=new PioneerProbeCookie;return c;}
static PioneerNativeBfpRequest Request(unsigned bits=4){PioneerNativeBfpRequest r={};r.version=1;r.bits=bits;r.expected_tag=PioneerNativeBfpTag;r.expected_generation=19;return r;}
static void Reset(){
 teardown=true;if(owner){delete owner;owner=nullptr;}teardown=false;sRuntimeOwner=nullptr;sColdRecoveryRequired=false;locked=false;
 uid=0;team=7;copyCalls=failCopy=reserves=unreserves=runs=ends=finishes=deleted=baselines=0;
 threadError=profileError=moduleError=reserveError=lockError=0;quiesced=false;profileValid=mapped=baseline=baselineClean=finishOK=endOK=true;mutateInput=false;
 sOpenCookies=1;sDeviceManager=nullptr;sReloadGuard=&guard;guard.version=1;runState=1;
}
static void FinishFixture(NativeCookie* c){Reset();delete c->base;delete c;}
int main(){unsigned cases=0;
 for(unsigned failure=0;failure<12;failure++){
  Reset();auto* c=Cookie();auto r=Request();
  if(failure==0)uid=1;else if(failure==1)team=8;else if(failure==2)c->base->context=c;
  else if(failure==3)r.reserved=1;else if(failure==4)r.result.output[63]=1;
  else if(failure==5)r.bits=16;else if(failure==6)r.nonce=1;
  else if(failure==7)r.expected_tag++;else if(failure==8)r.expected_generation++;
  else if(failure==9)sOpenCookies=2;else if(failure==10)quiesced=true;else failCopy=1;
  assert(Control(c,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r))!=0 && !owner && !reserves && !runs && !finishes && !locked);FinishFixture(c);cases++;
 }
 {Reset();auto* c=Cookie();auto r=Request();assert(Control(c,1234,&r,sizeof(r))==B_NOT_ALLOWED && !copyCalls && !reserves);FinishFixture(c);cases++;}
 {Reset();auto* c=Cookie();auto r=Request();assert(!Control(c,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r)) && owner && sRuntimeOwner==c->base && runs==1 && !finishes && !unreserves);
  auto end=Request();end.accept=1;end.nonce=1;assert(!Control(c,PIONEER_NATIVE_BFP_END,&end,sizeof(end)) && end.result.state==2 && !owner && !sRuntimeOwner && ends==1 && finishes==1 && unreserves==1 && deleted==1);
  auto again=Request();assert(Control(c,PIONEER_NATIVE_BFP_RUN,&again,sizeof(again))==B_BUSY && runs==1);assert(!Free(c));Reset();cases++;}
 for(unsigned failure=0;failure<6;failure++){
  Reset();auto* c=Cookie();auto r=Request();
  if(failure==0)failCopy=2;else if(failure==1)runState=4;else if(failure==2)baselineClean=false;
  else if(failure==3){baseline=false;baselineClean=false;}else if(failure==4){baseline=false;finishOK=false;}
  else mutateInput=true;
  int status=Control(c,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r));
  if(failure==5){assert(!status && owner->bits==4 && r.bits==4);}
  else assert(owner && sRuntimeOwner==c->base && sColdRecoveryRequired && !unreserves && !deleted && !ends);
  const auto f=finishes,e=ends,u=unreserves;
  assert(!Free(c) && owner && !sOpenCookies && sColdRecoveryRequired && finishes==f && ends==e && unreserves==u);
  FinishFixture(c);cases++;
 }
 for(unsigned failure=0;failure<3;failure++){
  Reset();auto* c=Cookie();auto r=Request();if(failure==0)baseline=false;else if(failure==1)mapped=false;else runState=3;
  assert(!Control(c,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r)) && r.result.state==3 && !owner && !sColdRecoveryRequired && finishes==1 && unreserves==1 && deleted==1);FinishFixture(c);cases++;
 }
 for(unsigned failure=0;failure<5;failure++){
  Reset();auto* c=Cookie();auto r=Request();assert(!Control(c,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r)));
  auto end=Request();end.accept=1;end.nonce=1;
  if(failure==0)end.bits=8;else if(failure==1)sOpenCookies=2;else if(failure==2)endOK=false;
  else if(failure==3)finishOK=false;else failCopy=copyCalls+2;
  int status=Control(c,PIONEER_NATIVE_BFP_END,&end,sizeof(end));
  if(failure<2)assert(status && owner && !ends && !finishes);
  else if(failure<4)assert(!status && owner && sColdRecoveryRequired && !unreserves && !deleted);
  else assert(status && !owner && !sColdRecoveryRequired && ends==1 && finishes==1 && unreserves==1 && deleted==1);
  FinishFixture(c);cases++;
 }
 printf("PASS ACTUAL native BFP Control/Reply/Fault/Dispose/Free:%u cases;malformed/foreign/stale/busy inputs no hardware entry,same-FD one-shot,owned user copy,failed RUN delivery retained,explicit END only,END-reply failure known closed;FAKE device/lease endpoints,NO physical proof\n",cases);
}
'''
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-native-bfp-control.'))
if exact:
    prefix = prefix.replace('#include "PioneerNativeBfpProtocol.h"', '#include "PioneerNativeExactQ4Protocol.h"\n#define PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC 1')
    tests = tests.replace('r.expected_tag=PioneerNativeBfpTag', 'r.expected_tag=PioneerNativeExactQ4Tag')
    for old, new in (('PIONEER_NATIVE_BFP_RUN', 'PIONEER_NATIVE_EXACT_Q4_RUN'), ('PIONEER_NATIVE_BFP_END', 'PIONEER_NATIVE_EXACT_Q4_END')):
        tests = tests.replace(old, new)
    tests = tests.replace('printf("PASS ACTUAL native BFP', '''
 for(unsigned kind=0;kind<3;kind++){
  Reset();auto* c=Cookie();auto r=Request();
  if(kind==0)r.expected_tag=PioneerNativeBfpTag;else if(kind==1)r.bits=8;
  const unsigned op=kind==2?PIONEER_NATIVE_BFP_RUN:PIONEER_NATIVE_EXACT_Q4_RUN;
  assert(Control(c,op,&r,sizeof(r)) && !owner && !reserves && !runs);FinishFixture(c);cases++;
 }
 for(unsigned byte=0;byte<sizeof(PioneerNativeBfpResult);byte++){
  Reset();auto* c=Cookie();auto r=Request();reinterpret_cast<unsigned char*>(&r.result)[byte]=1;
  assert(Control(c,PIONEER_NATIVE_EXACT_Q4_RUN,&r,sizeof(r)) && !owner && !reserves && !runs && !baselines);FinishFixture(c);cases++;
 }
 for(unsigned length:{0U,1U,359U,361U,720U}){
  Reset();auto* c=Cookie();auto r=Request();
  assert(Control(c,PIONEER_NATIVE_EXACT_Q4_RUN,&r,length) && !copyCalls && !owner && !reserves && !runs);FinishFixture(c);cases++;
 }
 printf("PASS ACTUAL exact Q4''')
    prefix = prefix.replace('#include <new>', '#include <new>\n#include <initializer_list>')
if affine:
    prefix=prefix.replace('PioneerNativeExactQ4','PioneerNativeExactQ4Affine').replace('PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC','PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC')
    tests=tests.replace('PioneerNativeExactQ4','PioneerNativeExactQ4Affine').replace('PIONEER_NATIVE_EXACT_Q4_RUN','PIONEER_NATIVE_EXACT_Q4_AFFINE_RUN').replace('PIONEER_NATIVE_EXACT_Q4_END','PIONEER_NATIVE_EXACT_Q4_AFFINE_END')
    tests=tests.replace('PASS ACTUAL exact Q4','PASS ACTUAL exact Q4 full-K affine')
if srca:
    prefix=prefix.replace('PioneerNativeExactQ4','PioneerNativeSrcaProbe').replace('PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC','PIONEER_NATIVE_SRCA_PROBE_DIAGNOSTIC')
    tests=tests.replace('PioneerNativeExactQ4','PioneerNativeSrcaProbe').replace('PIONEER_NATIVE_EXACT_Q4_RUN','PIONEER_NATIVE_SRCA_PROBE_RUN').replace('PIONEER_NATIVE_EXACT_Q4_END','PIONEER_NATIVE_SRCA_PROBE_END')
    tests=tests.replace('PASS ACTUAL exact Q4','PASS ACTUAL SrcA')
if band:
    prefix = prefix.replace('#include "PioneerNativeBfpProtocol.h"', '#include "PioneerNativeBfpBandProtocol.h"\n#include "PioneerNativeBfpBandLayout.h"')
    prefix = prefix.replace('struct PioneerNativeBfpGeneratedImage{};', 'template<unsigned K>struct PioneerNativeBfpBandGeneratedImage{static constexpr unsigned Width=K;};')
    prefix = prefix.replace('bool Run(A&,int&,unsigned){runs++;', 'bool Run(A&,int&,unsigned k){assert(B::Width==k);runs++;')
    prefix = prefix.replace('->bits=8', '->bits=6144')
    tests = tests.replace('unsigned bits=4', 'unsigned bits=5120').replace('end.bits=8', 'end.bits=6144').replace('owner->bits==4 && r.bits==4', 'owner->bits==5120 && r.bits==5120')
    prefix = prefix.replace('PioneerNativeBfpRequest', 'PioneerNativeBfpBandRequest')
    tests = tests.replace('PioneerNativeBfpRequest', 'PioneerNativeBfpBandRequest').replace('PioneerNativeBfpTag', 'PioneerNativeBfpBandTag')
    for old, new in (('PIONEER_NATIVE_BFP_SUPPORT', 'PIONEER_NATIVE_BFP_BAND_SUPPORT'), ('PIONEER_NATIVE_BFP_RUN', 'PIONEER_NATIVE_BFP_BAND_RUN'), ('PIONEER_NATIVE_BFP_END', 'PIONEER_NATIVE_BFP_BAND_END')):
        tests = tests.replace(old, new)
    prefix = prefix.replace('bits', 'width')
    tests = tests.replace('bits', 'width')
    tests = tests.replace('printf("PASS ACTUAL native BFP', '''for(unsigned width:{5120U,6144U,17408U}){
  Reset();auto* c=Cookie();auto r=Request(width);assert(!Control(c,PIONEER_NATIVE_BFP_BAND_RUN,&r,sizeof(r)) && owner && owner->width==width);
  auto end=Request(width);end.accept=1;end.nonce=1;assert(!Control(c,PIONEER_NATIVE_BFP_BAND_END,&end,sizeof(end)) && !owner && ends==1);assert(!Free(c));Reset();cases++;
 }
 {Reset();auto* c=Cookie();auto r=Request();assert(Control(c,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r))==B_NOT_ALLOWED && !copyCalls && !reserves);FinishFixture(c);cases++;}
 printf("PASS ACTUAL full-K native BFP''')
    prefix = prefix.replace('#include <new>', '#include <new>\n#include <initializer_list>')
if model:
    prefix = prefix.replace('PioneerNativeBfpBandProtocol.h', 'PioneerNativeModelBandProtocol.h').replace('PioneerNativeBfpBandRequest', 'PioneerNativeModelBandRequest')
    prefix = prefix.replace('assert(B::Width==k)', 'assert(B::Width==5120 && k<3)')
    prefix = prefix.replace('->width=6144', '->width=1').replace('width', 'sample')
    tests = tests.replace('PioneerNativeBfpBandRequest', 'PioneerNativeModelBandRequest').replace('PioneerNativeBfpBandTag', 'PioneerNativeModelBandTag')
    tests = tests.replace('unsigned width=5120', 'unsigned sample=0').replace('end.width=6144', 'end.sample=1')
    tests = tests.replace('owner->width==5120 && r.width==5120', 'owner->sample==0 && r.sample==0').replace('width', 'sample')
    tests = tests.replace('for(unsigned sample:{5120U,6144U,17408U})', 'for(unsigned sample:{0U,1U,2U})')
    for old, new in (('PIONEER_NATIVE_BFP_BAND_RUN', 'PIONEER_NATIVE_MODEL_BAND_RUN'), ('PIONEER_NATIVE_BFP_BAND_END', 'PIONEER_NATIVE_MODEL_BAND_END')):
        tests = tests.replace(old, new)
    tests = tests.replace('PASS ACTUAL full-K native BFP', 'PASS ACTUAL fixed learned-model native BFP')
    prefix += '\nconstexpr unsigned NativeBandWidth=5120;\n'
if pair:
    prefix = prefix.replace('#include "PioneerNativeModelBandProtocol.h"', '''#define PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC 1
#include "PioneerNativeModelBandPairProtocol.h"
#define PioneerNativeModelBandRequest PioneerNativeModelBandPairRequest
#define PioneerNativeModelBandRequestValid PioneerNativeModelBandPairRequestValid
#define PIONEER_NATIVE_MODEL_BAND_SUPPORT PIONEER_NATIVE_MODEL_BAND_PAIR_SUPPORT
#define PIONEER_NATIVE_MODEL_BAND_RUN PIONEER_NATIVE_MODEL_BAND_PAIR_RUN
#define PIONEER_NATIVE_MODEL_BAND_END PIONEER_NATIVE_MODEL_BAND_PAIR_END
constexpr uint32_t PioneerNativeModelBandNone=UINT32_MAX;''')
    start = prefix.index('struct FakeLease{')
    end = prefix.index('struct NativeOwner{', start)
    prefix = prefix[:start] + r'''struct FakeLease{
 PioneerNativeBfpResult r[2]={};bool owned=false;
 template<class A,class B>bool Run(A&,int&,unsigned a,unsigned b){assert(B::Width==5120 && a==0 && b==1);runs++;owned=true;
  for(auto& x:r){x.state=runState;x.started=1;x.completed=runState==1;}return runState==1;}
 bool End(NativeIO&,int&){ends++;if(!endOK){for(auto& x:r){x.state=4;x.cold=1;}return false;}
  owned=false;for(auto& x:r)x.state=2;return true;}
 void Abandon(){if(owned)for(auto& x:r){x.state=4;x.cold=1;}}
 bool OverlapVerified()const{return true;}
 const auto& Result(unsigned i)const{assert(i<2);return r[i];}
};
''' + prefix[end:]
    tests = tests.replace('PioneerNativeModelBandTag', 'PioneerNativeModelBandPairTag')
    tests = tests.replace('r.reserved=1', 'r.overlap_verified=1').replace('r.result.', 'r.result[0].').replace('end.result.', 'end.result[0].')
    tests = tests.replace('for(unsigned sample:{0U,1U,2U})', 'for(unsigned sample:{0U})')
    tests = tests.replace('PASS ACTUAL fixed learned-model native BFP', 'PASS ACTUAL fixed pair diagnostic')
    tests = tests.replace('printf("PASS ACTUAL fixed pair diagnostic', '''for(unsigned failure=0;failure<4;failure++){
  Reset();auto* c=Cookie();auto r=Request();
  if(failure==0)r.sample=1;else if(failure==1)r.sample=2;
  else if(failure==2)r.result[1].state=1;else r.result[1].output[63]=1;
  assert(Control(c,PIONEER_NATIVE_MODEL_BAND_RUN,&r,sizeof(r))!=0 && !owner && !reserves && !runs && !finishes && !locked);
  FinishFixture(c);cases++;
 }
 {Reset();auto* c=Cookie();auto r=Request();runState=PIONEER_NATIVE_BFP_UNKNOWN;
  assert(!Control(c,PIONEER_NATIVE_MODEL_BAND_RUN,&r,sizeof(r)) && owner && sColdRecoveryRequired);
  for(auto& result:r.result)assert(result.state==PIONEER_NATIVE_BFP_UNKNOWN && result.cold==1);
  const auto f=finishes,e=ends,u=unreserves;
  assert(!Free(c) && owner && finishes==f && ends==e && unreserves==u);
  FinishFixture(c);cases++;
 }
 printf("PASS ACTUAL fixed pair diagnostic''')
(folder / 'test.cpp').write_text(prefix + code + tests)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I' + str(native.parent), '-I' + str(here.parent), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
subprocess.run([str(folder / 'test')], check=True, env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
print('CPU-only actual controller evidence:', folder)
