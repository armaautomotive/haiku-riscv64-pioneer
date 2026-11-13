#!/usr/bin/env python3
"""Actual learned-band lease+PRIVATE real weights+fresh RV32;fake MMIO ONLY."""
import hashlib
import os
import pathlib
import stat
import subprocess
import sys
import tempfile
if not __debug__ or len(sys.argv) not in (3,4) or (len(sys.argv)==4 and sys.argv[3]!='--recurrent-out'):
    raise SystemExit('Use non-optimized Python SDK PRIVATE_EXPORT_DIRECTORY [--recurrent-out]')
here = pathlib.Path(__file__).resolve().parent
sdk, private = map(pathlib.Path, sys.argv[1:3])
recurrent_out=len(sys.argv)==4
assert sdk.is_absolute() and private.is_absolute() and not private.is_symlink()
identity = private.stat()
assert stat.S_ISDIR(identity.st_mode) and identity.st_uid == os.geteuid() and stat.S_IMODE(identity.st_mode) == 0o700
digests = {}
for name in ('PioneerNativeBfpModelBandData.h', 'PioneerNativeBfpModelBandMetadata.h'):
    path = private / name
    info = path.lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o600
    digests[name] = hashlib.sha256(path.read_bytes()).hexdigest()
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-learned-band-owner.'))
if recurrent_out:
    assert digests=={'PioneerNativeBfpModelBandData.h':'1facbbadaef22f9b1fadaa33a4b19f2168bc9b1a244e9df9d7880ddd97b10cb1',
                     'PioneerNativeBfpModelBandMetadata.h':'7cef223c05f1e78b0a4c7c45cd24327b569a7d7e9cd3196895151ab67a18d65d'}
subprocess.run([sys.executable, str(here.parent / 'build_native_bfp_band.py'), str(sdk), str(folder / 'image'), '6144' if recurrent_out else '5120', '4'], check=True)
assert hashlib.sha256((folder / 'image/text.bin').read_bytes()).hexdigest() == ('a6516a81cdb2e40b1850a0e51c3c2c2d706eaa2aa0207b29cbf5c2c3feb1a09e' if recurrent_out else 'f69c2803d854bbef87f37df7cb96536e1f5d940060ed98fec6cb3200d50a9ad5')
old = (here / 'pioneer_native_bfp_band_owner_test.cpp').read_text()
io = old[old.index('struct IO {'):old.index('template<unsigned K>static void NoMoreIO')]
prefix = r'''
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_NATIVE_BFP_BAND_IMAGE_ONLY
#include "pioneer_q38_native_bfp_band_image_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "PioneerNativeModelBandLease.h"
#include "PioneerNativeBfpModelBandMetadata.h"
#include <memory>
#include <set>
using Lease=PioneerNativeModelBandLease;
using Band=PioneerQ38NativeBfpBand;
struct Image {
 static constexpr unsigned Width=5120,Phases=4;
 static inline uint32_t Boot=0;static inline std::vector<uint8_t> code;
 static uint32_t Word(unsigned index){return index<code.size()/4?::Word(code,index*4):0;}
};
struct RoundedImage:BandRV32 {
 using BandRV32::BandRV32;
 void Instruction(uint32_t op)override{
  if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{BandRV32::Instruction(op);});
  else BandRV32::Instruction(op);
 }
};
'''
tests = r'''
static void NoMoreIO(Lease& lease,IO& io,tt_dram_samples_result& baseline,unsigned sample){
 const auto reads=io.reads,writes=io.writes,resets=io.resets,tlb=io.tlbWrites;
 assert((!lease.End(io,baseline) && !lease.Run<IO,Image>(io,baseline,sample)));lease.Abandon();
 assert(reads==io.reads && writes==io.writes && resets==io.resets && tlb==io.tlbWrites);
}
int main(int argc,char** argv){
 assert(argc==2);const std::string root=argv[1];Image::code=Read((root+"/text.bin").c_str());
 const auto boot=Read((root+"/boot.bin").c_str());assert(boot.size()==4);Image::Boot=Word(boot,0);
 assert(Lease::SavedWords(5120)==50500 && !Lease::SavedWords(6144) && !Lease::SavedWords(17408));
 std::set<uint32_t> addresses;
 for(unsigned i=0;i<Lease::MaxSavedWords;i++)assert(addresses.insert(Lease::Address(Image::Width,i)).second);
 assert(addresses.size()==Lease::MaxSavedWords && Lease::Address(Image::Width,Lease::MaxSavedWords)==UINT32_MAX);
 for(unsigned i=0;i<Band::Tiles(Image::Width)*Band::WeightStride/4;i++)assert(addresses.count(Band::Weight-16+i*4));
 for(unsigned i=0;i<Band::Tiles(Image::Width)*Band::InputStride/4;i++)assert(addresses.count(Band::Input-16+i*4));
 for(unsigned sample=0;sample<3;sample++){
  IO io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease>();
  assert((lease->Run<IO,Image>(io,baseline,sample)) && io.resets==2);
  const auto& result=lease->Result();assert(result.state==PIONEER_NATIVE_BFP_RESULT && result.completed && !result.cold);
  const auto* expected=PioneerNativeBfpModelBandMetadata::Golden(sample);assert(expected);
  for(unsigned word=0;word<64;word++)assert(result.output[word]==expected[word%32]);
  assert(lease->End(io,baseline) && lease->Result().state==PIONEER_NATIVE_BFP_CLOSED && io.memory==io.original && io.resets==4);
  for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)assert(io.tlb[s][w]==s*3+w+1);
  NoMoreIO(*lease,io,baseline,sample);
  printf("PASS_OFFBOARD_ACTUAL_LEARNED_BAND_OWNER sample%u first_row%u tensor%u K%u two_math_jobs exact64_words complete202000B_owned_L1_TLB_restore;NO_DEVICE_MODEL_QUALITY_SPEED_PROOF\n",sample,PioneerNativeBfpModelBandMetadata::First(sample),PioneerNativeBfpModelBandMetadata::Tensor,Image::Width);
 }
 unsigned faults=0;
 for(unsigned fault=0;fault<14;fault++){
  IO io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease>();
  if(fault==0)io.timeout=true;if(fault==1)io.badPC=true;if(fault==2)io.faultMarker=true;if(fault==3)io.sourceChanged=true;
  if(fault==4)io.dropL1=1024;if(fault==5)io.dropReset=1;if(fault==6)io.dropReset=2;if(fault==7)io.dropTlb=2;
  if(fault==12)io.oldMarker=true;if(fault==13)io.badCount=true;
  bool ran=lease->Run<IO,Image>(io,baseline,0);
  if(fault>=8 && fault<12){assert(ran);if(fault==8)io.dropReset=3;if(fault==9)io.dropReset=4;
   if(fault==10)io.failRestore=true;if(fault==11)io.dropTlb=io.tlbWrites+370;assert(!lease->End(io,baseline));
  }else assert(!ran);
  assert(lease->Result().state==PIONEER_NATIVE_BFP_UNKNOWN && lease->Result().cold);
  if(fault<8 || fault>=12)assert(io.resets<3);NoMoreIO(*lease,io,baseline,0);faults++;
 }
 for(unsigned sample:{3U,4U,UINT32_MAX}){
  IO io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease>();
  assert((!lease->Run<IO,Image>(io,baseline,sample)) && !io.reads && !io.writes && !io.tlbWrites && !io.resets);faults++;
 }
 printf("PASS_OFFBOARD_LEARNED_OWNER failure_cases%u;fixed_selector_admission,first_uncertainty_retains_NO_END_RETRY_RESET_CLEANUP;PRIVATE_MODEL_BYTES_NOT_PRINTED_OR_PUBLISHED\n",faults);
}
'''
if recurrent_out:
    prefix='#define PIONEER_NATIVE_RECURRENT_OUT_BAND_OWNER_INSPECTION 1\n'+prefix.replace('Width=5120,Phases=4','Width=6144,Phases=4')
    tests=tests.replace('SavedWords(5120)==50500 && !Lease::SavedWords(6144)',
                        'SavedWords(6144)==59972 && !Lease::SavedWords(5120)')
    tests=tests.replace('IO io(5120,','IO io(6144,').replace('complete202000B','complete239888B')
(folder / 'test.cpp').write_text(prefix + io + tests)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                '-fsanitize=address,undefined', '-I' + str(here), '-I' + str(here.parent), '-I' + str(private),
                str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
subprocess.run([str(folder / 'test'), str(folder / 'image')], check=True,
               env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
for name, digest in digests.items():
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
if recurrent_out:
    for forbidden in ('_KERNEL_MODE','PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC','PIONEER_DRIVER_BUILD_TAG',
                      'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
        result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-DPIONEER_NATIVE_RECURRENT_OUT_BAND_OWNER_INSPECTION=1',
                               '-D'+forbidden+'=1','-I'+str(here.parent),'-I'+str(private),'-fsyntax-only','-x','c++','-'],
                              input='#include "PioneerNativeModelBandLease.h"\n',text=True,capture_output=True)
        assert result.returncode,forbidden
print('OFFBOARD actual learned-band owner evidence:', folder)
