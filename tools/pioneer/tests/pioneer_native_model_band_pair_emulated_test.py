#!/usr/bin/env python3
"""Run both fixed QKV images through actual RV32/matrix instruction simulation.

This is host simulation, NOT physical overlap, a driver test, or an inference
benchmark. Private model-band bytes and generated fixtures stay in tmp.
"""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 3:
    raise SystemExit('usage: python3 pioneer_native_model_band_pair_emulated_test.py SDK PRIVATE_QKV_EXPORT')
sdk, private = map(pathlib.Path, sys.argv[1:])
if not sdk.is_absolute() or not private.is_absolute() or private.is_symlink():
    raise SystemExit('SDK and private export must be absolute; private export must not be a symlink')
digests = {
    'PioneerNativeBfpModelBandData.h': '396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a',
    'PioneerNativeBfpModelBandMetadata.h': 'a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d',
}
for name, digest in digests.items():
    if (private / name).is_symlink() or hashlib.sha256((private / name).read_bytes()).hexdigest() != digest:
        raise SystemExit('private QKV export identity mismatch')
here = pathlib.Path(__file__).resolve().parent
old = (here / 'pioneer_native_bfp_band_owner_test.cpp').read_text()
core = old[old.index('struct IO {'):old.index('template<unsigned K>static void NoMoreIO')]
core = core.replace('struct IO {', 'struct CoreIO {', 1).replace('IO(unsigned k,const char* path)', 'CoreIO(unsigned k,const char* path)', 1)
core = core.replace('RoundedImage image(directory.c_str(),width,4,false);',
    'RoundedImage image(directory.c_str(),width,4,false,128);')
prefix = r'''
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_NATIVE_BFP_BAND_IMAGE_ONLY
#define PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION 1
#include "pioneer_q38_native_bfp_band_image_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "PioneerNativeModelBandPairInspection.h"
#include "PioneerNativeBfpModelBandMetadata.h"
#include <memory>
using Band=PioneerQ38NativeBfpBand;
struct Image {
 static constexpr unsigned Width=5120,Phases=4;
 static inline uint32_t Boot=0;static inline std::vector<uint8_t> code;
 static uint32_t Word(unsigned n){return n<code.size()/4?::Word(code,n*4):0;}
};
struct RoundedImage:BandRV32 {
 using BandRV32::BandRV32;
 void Instruction(uint32_t op)override{
  if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{BandRV32::Instruction(op);});
  else BandRV32::Instruction(op);
 }
};
'''
test = r'''
struct PairIO {
 CoreIO core[2];uint32_t tlb[2][3]={{1,2,3},{4,5,6}};unsigned starts=0;uint64_t now=1000000;
 explicit PairIO(const char* path):core{CoreIO(5120,path),CoreIO(5120,path)}{
  for(auto& c:core){assert(BlackholeDramConfigure(c,0,0,1,2,false));assert(BlackholeDramConfigure(c,1,0xffa00000,1,2,false));}
 }
 bool Match(unsigned s,unsigned x,unsigned y,bool ro){for(unsigned w=0;w<3;w++)if(tlb[s][w]!=BlackholeDramFixedTlbWord(s?0xffa00000:0,x,y,ro,w))return false;return true;}
 int Selected(unsigned s,bool ro){for(unsigned i=0;i<2;i++)if(Match(s,1,2+i,ro))return int(i);return -1;}
 CoreIO& Window(){int a=Selected(0,false),b=Selected(1,false);assert(a>=0 && a==b);return core[a];}
 uint32_t SourceRead(unsigned i){return core[0].SourceRead(i);}
 uint32_t DramTlbRead(unsigned s,unsigned w){assert(s<2 && w<3);return tlb[s][w];}
 void DramTlbWrite(unsigned s,unsigned w,uint32_t v){assert(s<2 && w<3);tlb[s][w]=v;}
 uint32_t ResetRead(){int i=Selected(1,false);if(i>=0)return core[i].reset;i=Selected(1,true);if(i>=0)return core[i].reset;
  const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};for(unsigned x:xs)for(unsigned y=2;y<=11;y++)if(Match(1,x,y,true))return 0x47800;assert(false);return 0;}
 uint32_t BfpRead(uint32_t a){return Window().BfpRead(a);}
 void BfpWrite(uint32_t a,uint32_t v){Window().BfpWrite(a,v);}
 uint32_t BfpControlRead(unsigned i){return Window().BfpControlRead(i);}
 uint32_t BfpPcRead(){return Window().BfpPcRead();}
 void BfpResetWrite(uint32_t v){auto& c=Window();if(v!=0x46800){c.BfpResetWrite(v);return;}
  assert(c.reset==0x87ffff && !c.started);c.started=true;c.reset=v;
  starts|=(&c==&core[0]?1U:2U);
  // Execute the real image only after the pair has both been released.
  // Host execution is serial; no claim of physical overlap follows.
  if(starts==3){core[0].Execute();core[1].Execute();}
 }
 uint64_t BfpNow(){return now;}void Pause(){now+=100000;for(auto& c:core)c.Pause();}
 bool BfpHealthy()const{return true;}
 tt_dram_samples_result Baseline(){return core[0].Baseline();}
};
int main(int argc,char** argv){
 assert(argc==2);std::string root=argv[1];Image::code=Read((root+"/text.bin").c_str());
 auto boot=Read((root+"/boot.bin").c_str());assert(boot.size()==4);Image::Boot=Word(boot,0);
 PairIO io(argv[1]);auto baseline=io.Baseline();auto owner=std::make_unique<PioneerNativeModelBandPairInspection>();
 assert((owner->Run<PairIO,Image>(io,baseline,0,1)));assert(io.starts==3 && io.core[0].executed && io.core[1].executed);
 for(unsigned i=0;i<2;i++){
  auto& r=owner->Result(i);assert(r.completed && r.state==PIONEER_NATIVE_BFP_RESULT);
  const auto* expected=PioneerNativeBfpModelBandMetadata::Golden(i);assert(expected);
  for(unsigned n=0;n<64;n++)assert(r.output[n]==expected[n%32]);
 }
 assert(owner->End(io,baseline));
 for(auto& c:io.core)assert(c.memory==c.original && c.reset==0x47800);
 for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)assert(io.tlb[s][w]==s*3+w+1);
 puts("PASS_OFFLINE_PAIR_ACTUAL_RV32_MATRIX_INSTRUCTIONS 2_QKV_BANDS 128_EXACT_OUTPUT_WORDS; NO_PHYSICAL_OVERLAP_OR_SPEED_PROOF");
}
'''
with tempfile.TemporaryDirectory(prefix='pioneer-pair-emulated-') as tmp:
    folder = pathlib.Path(tmp)
    subprocess.run([sys.executable, str(here.parent / 'build_native_bfp_band.py'), str(sdk), str(folder / 'image'), '5120', '4', '--pair-overlap-work'], check=True)
    if hashlib.sha256((folder / 'image/text.bin').read_bytes()).hexdigest() != '2b1527ba98e0a4c276bb9271daf48867c821ecaa83166f8b1399185aa9aaf3ef':
        raise SystemExit('generated image identity mismatch')
    (folder / 'test.cpp').write_text(prefix + core + test)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
        '-ffp-contract=off', '-fsanitize=address,undefined', '-I' + str(here), '-I' + str(here.parent),
        '-I' + str(private), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
    subprocess.run([str(folder / 'test'), str(folder / 'image')], check=True,
        env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
for name, digest in digests.items():
    if hashlib.sha256((private / name).read_bytes()).hexdigest() != digest:
        raise SystemExit('private export changed during test')
