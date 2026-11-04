#!/usr/bin/env python3
"""Actual LOAD-ONLY82 full journal+PRIVATE bytes+actual RV32;fake MMIO,NO GPU."""
import hashlib
import os
import pathlib
import stat
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 3:
    raise SystemExit('Use non-optimized Python SDK PRIVATE_EXPORT')
here = pathlib.Path(__file__).resolve().parent
sdk, private = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and private.is_absolute() and not private.is_symlink()
assert stat.S_IMODE(private.stat().st_mode) == 0o700 and private.stat().st_uid == os.geteuid()
pins = {'PioneerNativeBfpModelBandData.h': 'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5',
        'PioneerNativeBfpModelBandMetadata.h': 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'}
for name, digest in pins.items():
    info = (private / name).lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_uid == os.geteuid() and info.st_nlink == 1 and stat.S_IMODE(info.st_mode) == 0o600
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-dram-load-only-lease.'))
subprocess.run([sys.executable, str(here.parent / 'build_native_dram_load.py'), str(sdk), str(folder / 'image')], check=True)
assert hashlib.sha256((folder / 'image/text.bin').read_bytes()).hexdigest() == '966507e23929a1d28f62f56de0ea3c3a6c9148d9ff8f59c84138f746eea779a0'
# Reuse explicit old fake-MMIO endpoints,NOT its FPU execution or algorithm.
old = (here / 'pioneer_native_dram_model_band_lease_test.cpp').read_text()
io = old[old.index('struct LeaseIO {'):old.index('static void NoMore(')]
io = io.replace('DramBandRV32::Range', 'DramLoadRV32::Range')
first = io.index('\tvoid Execute() {')
last = io.index('\tbool NativeResetWrite(', first)
execute = r'''
 void Execute() {
  DramLoadRV32 image(directory.c_str(),proof->translated[0][0]|(proof->translated[0][1]<<6));
  assert(memory[0x6000/4]==Image::Boot);
  for(unsigned i=0;i<2048;i++)assert(memory[(0x70000+i*4)/4]==Image::Word(i));
  for(unsigned i=0;i<Lease::L1Words;i++) {
   const unsigned a=Lease::L1Address(i),v=memory[a/4];
   if(DramLoadRV32::Range(a,L::Weight-32,160*L::WeightStride) || DramLoadRV32::Range(a,L::Input-32,160*L::InputStride)) {
    for(unsigned b=0;b<4;b++)image.transfer.l1[a+b]=uint8_t(v>>(b*8));
   } else if(!DramLoadRV32::Range(a,0x70000+unsigned(image.code.size()),8192-unsigned(image.code.size())))image.l1[image.Index(a)]=v;
  }
  for(unsigned i=0;i<image.transfer.dram.size();i++)image.transfer.dram[i]=uint8_t(gddr[i/4]>>((i%4)*8));
  for(unsigned i=0;i<16;i++)image.transfer.counters[i]=counters[i];
  assert(image.Execute());
  for(unsigned i=0;i<Lease::L1Words;i++) {
   const unsigned a=Lease::L1Address(i);
   if(DramLoadRV32::Range(a,L::Weight-32,160*L::WeightStride) || DramLoadRV32::Range(a,L::Input-32,160*L::InputStride))memory[a/4]=::Word(image.transfer.l1,a);
   else if(!DramLoadRV32::Range(a,0x70000+unsigned(image.code.size()),8192-unsigned(image.code.size())))memory[a/4]=image.l1[image.Index(a)];
  }
  for(unsigned i=0;i<16;i++)counters[i]=image.transfer.counters[i];
  for(unsigned i=0;i<10;i++)command[i]=image.transfer.command[i];executed=true;
 }
'''
io = io[:first] + execute + io[last:]
io = io.replace('0x3342464e', '0x3142444e') # old math81 marker must refuse
assert io.count('bool timeout=false') == 1 and io.count('reset=v;}return true;') == 1
io = io.replace('bool timeout=false', 'bool damageAfterHold=false;bool timeout=false')
io = io.replace('reset=v;}return true;', 'reset=v;}if(damageAfterHold && started && v==0x47800)memory[L::Weight/4]^=1;return true;')
prefix = r'''
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_NATIVE_DRAM_LOAD_IMAGE_ONLY
#include "pioneer_native_dram_load_image_test.cpp"
#include "PioneerNativeDramLoadLease.h"
#include <memory>
#include <type_traits>
using Lease=PioneerNativeDramLoadLease;
struct Image {
 static constexpr unsigned Width=5120,Phases=0,LoadOnly=1;
 static inline unsigned Boot=0;static inline std::vector<uint8_t> code;
 static unsigned Word(unsigned i){return i<code.size()/4?::Word(code,i*4):0;}
};
'''
tests = r'''
static void NoMore(Lease& lease,LeaseIO& io){
 const unsigned operations=io.operations,scopes=io.scopeCalls;
 assert((!lease.Run<LeaseIO,Image>(io,*io.proof,0) && !lease.End<LeaseIO,Image>(io)));
 lease.Abandon();assert(io.operations==operations && io.scopeCalls==scopes);
}
int main(int argc,char** argv){
 assert(argc==2);const std::string folder=argv[1];Image::code=Read((folder+"/text.bin").c_str());
 const auto boot=Read((folder+"/boot.bin").c_str());assert(boot.size()==4);Image::Boot=::Word(boot,0);
 static_assert(!std::is_copy_constructible<Lease>::value && !std::is_copy_assignable<Lease>::value,"journal exclusive");
 for(unsigned sample=0;sample<3;sample++){
  auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
  assert((lease->Run<LeaseIO,Image>(io,baseline,sample)) && io.executed && io.resets==2);
  const auto& r=lease->Result();assert(r.compute.state==PIONEER_NATIVE_BFP_RESULT && r.compute.count==0x80001400
   && r.compute.marker==0x314c444e && !r.payload_checked && !r.headers_checked && !r.mismatch_kind);
  for(unsigned v:r.compute.output)assert(v==0x7fc00001); // NEVER learned results
  assert((lease->End<LeaseIO,Image>(io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED && !r.compute.cold);
  assert(r.payload_checked==46080 && r.headers_checked==2560 && !r.mismatch_kind && !r.mismatch_offset
   && r.gddr_restored && r.niu_restored && r.clock_restored && r.compute.l1_restored && r.compute.tlb_restored);
  assert(io.memory==io.original && io.gddr==io.originalDram && io.resets==4 && !io.retained && !io.fifo && !io.busyClock);
  NoMore(*lease,io);
  printf("PASS OFFBOARD actual LOAD ONLY sample%u:real private bytes/actual RV32,46080 payload+2560 header words exact,complete L1/GDDR/NIU/TLB/clock restore;NO FPU/arithmetic observer/native/hardware proof\n",sample);
 }
 unsigned failed=0;
 for(unsigned operand=0;operand<2;operand++)for(unsigned tile:{0U,1U,79U,80U,158U,159U})for(unsigned site=0;site<5;site++){
  auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
  assert((lease->Run<LeaseIO,Image>(io,baseline,0)));
  const unsigned address=(operand?L::Input:L::Weight)+tile*(operand?L::InputStride:L::WeightStride);
  const unsigned offset=site==0?address-32:site==1?address-4:site==2?address:site==3?address+(operand?32:544):address+(operand?60:1084);
  io.memory[offset/4]^=1;
  assert((!lease->End<LeaseIO,Image>(io)) && io.retained && lease->Result().mismatch_kind==(site<2?1U:2U)
   && lease->Result().mismatch_offset==offset && !lease->Result().gddr_restored && io.dramWrites==46080 && io.resets==2);
  NoMore(*lease,io);failed++;
 }
 for(unsigned fault=0;fault<26;fault++){
  auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
  if(fault==0)io.dropTlb=1;if(fault==1)io.dropL1=1;if(fault==2)io.dropDram=1;
  if(fault==3)io.dropNiu=1;if(fault==4)io.dropReset=1;if(fault==5)io.dropReset=2;
  if(fault==6)io.timeout=true;if(fault==7)io.badMarker=true;if(fault==8)io.badPC=true;
  if(fault==9)io.clockBeginFail=true;if(fault==10)io.badSource=true;if(fault==11)io.badCounter=true;
  if(fault==25)io.damageEarlierDram=true;
  const bool ran=lease->Run<LeaseIO,Image>(io,baseline,0);
  if(fault>=12 && fault<25){
   assert(ran);if(fault==12)io.badSource=true;if(fault==13)io.scopeValid=false;if(fault==14)io.clockEndFail=true;
   if(fault==15)io.failDramRestore=true;if(fault==16)io.failL1Restore=true;
   if(fault==17)io.dropReset=3;if(fault==18)io.dropReset=4;if(fault==19)io.dropNiu=2;if(fault==20)io.dropNiu=12;
   if(fault==21)io.gddr[0]^=1;if(fault==22)io.memory[L::Weight/4]^=1;if(fault==23)io.memory[0x70000/4]^=1;if(fault==24)io.memory[0x74010/4]^=1;
   assert((!lease->End<LeaseIO,Image>(io)));
  }else assert(!ran);
  assert(io.retained && io.retains==1 && lease->Result().compute.state==PIONEER_NATIVE_BFP_UNKNOWN && lease->Result().compute.cold);
  if(fault==21)assert(lease->Result().mismatch_kind==3 && lease->Result().mismatch_offset==L::DramWeight);
  if(fault==22)assert(lease->Result().mismatch_kind==2 && lease->Result().mismatch_offset==L::Weight);
  if(fault==23 || fault==24)assert(lease->Result().mismatch_kind==4);
  NoMore(*lease,io);failed++;
 }
 for(unsigned bad:{3U,4U,UINT32_MAX}){
  auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
  assert((!lease->Run<LeaseIO,Image>(io,baseline,bad)) && !io.operations && !io.scopeCalls);failed++;
 }
 {
  auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
  assert((lease->Run<LeaseIO,Image>(io,baseline,0)));io.damageAfterHold=true;
  assert((!lease->End<LeaseIO,Image>(io)) && io.retained && lease->Result().mismatch_kind==5
   && lease->Result().mismatch_offset==L::Weight && lease->Result().payload_checked==46080
   && lease->Result().headers_checked==2560 && io.resets==4 && io.dramWrites==46080);
  NoMore(*lease,io);failed++;
 }
 printf("PASS actual LOAD ONLY journal:%u mismatch/failure/admission cases,address/kind ONLY,no private word dump;first failure NO later IO/END/retry/reset/cleanup;NO native/physical/quality/speed proof\n",failed);
}
'''
(folder / 'test.cpp').write_text(prefix + io + tests)
compiler = os.environ.get('CXX', 'c++')
common = [compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
          '-fsanitize=address,undefined', '-I' + str(here), '-I' + str(here.parent), '-I' + str(private)]
subprocess.run(common + [str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
subprocess.run([str(folder / 'test'), str(folder / 'image')], check=True,
               env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
for selection in ('_KERNEL_MODE', 'PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    result = subprocess.run(common + ['-D' + selection, '-x', 'c++', '-c', '-', '-o', str(folder / 'forbidden.o')],
                            input='#define B_DEVICE_OP_CODES_END 10000\n#include "PioneerNativeDramLoadLease.h"\n', text=True, capture_output=True)
    assert result.returncode and 'Load-only GDDR lease requires distinct diagnostic82' in result.stderr
for name, digest in pins.items():
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
print('PASS LOAD ONLY4 forbidden roles,PRIVATE data unchanged;OFFBOARD evidence:', folder)
