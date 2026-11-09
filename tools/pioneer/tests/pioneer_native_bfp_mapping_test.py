#!/usr/bin/env python3
"""Actual NativeIO Map/Unmap/Pointer with CPU-only virtual-memory endpoints."""
import os
import pathlib
import subprocess
import sys
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
assert len(sys.argv)==1 or sys.argv[1:] in (['--band'],['--exact-q4-affine'],['--srca']) or (len(sys.argv)==3 and sys.argv[1]=='--model')
srca=sys.argv[1:]==['--srca']
affine=sys.argv[1:]==['--exact-q4-affine']
band = sys.argv[1:]==['--band'] or (len(sys.argv)==3 and sys.argv[1]=='--model')
model = len(sys.argv)==3
name = 'pioneer_native_model_band_driver.cpp' if model else 'pioneer_native_bfp_band_driver.cpp' if band else 'pioneer_native_bfp_driver.cpp'
native = here.parents[2] / 'src/add-ons/kernel/drivers/misc' / name
text = native.read_text()
first = text.index('\tbool Map() {', text.index('struct NativeIO:'))
last = text.index('\tuint32 BfpRead(', first)
methods = text[first:last]
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <set>
#define B_DEVICE_OP_CODES_END 10000
#include "PioneerNativeBfpLease.h"
#include "PioneerPciProfile.h"
using uint8=uint8_t;using uint32=uint32_t;using area_id=int;
enum{B_OK=0,B_ANY_KERNEL_ADDRESS=0,B_KERNEL_READ_AREA=1,B_KERNEL_WRITE_AREA=2,B_UNCACHED_MEMORY=4};
struct Mapping{uint64_t physical;unsigned size,flags;void* pointer;};
static std::map<int,Mapping> maps;static unsigned mapCalls=0,deleteCalls=0,failMap=0,failDelete=0;static bool parentFail=false,failed=false;
static area_id map_physical_memory(const char*,uint64_t physical,unsigned size,int,unsigned flags,void** output){
 assert(!failed);if(++mapCalls==failMap)return -1;void* p=calloc(size,1);assert(p);*output=p;maps[mapCalls]={physical,size,flags,p};return int(mapCalls);
}
static int delete_area(int id){assert(!failed && maps.count(id));if(++deleteCalls==failDelete){failed=true;return -1;}free(maps[id].pointer);maps.erase(id);return 0;}
struct PioneerComputeNocIO{
 tt_pioneer_profile profile={};area_id nocArea=-1,dataArea=-1,dramArea=-1,resetArea=-1,csmArea=-1,scratchArea=-1,tlbArea=-1,area=-1;
 volatile uint32_t* data=nullptr;volatile uint32_t* tlb=nullptr;
 const volatile uint32_t *nocRegisters=nullptr,*dramNiu=nullptr,*reset=nullptr,*csm=nullptr,*scratch=nullptr,*nodeID=nullptr;
 PioneerComputeNocIO(){profile.bar0_host_start=UINT64_C(0x4900000000);}
 bool Map(){if(parentFail)return false;area_id* all[]={&nocArea,&dataArea,&dramArea,&resetArea,&csmArea,&scratchArea,&tlbArea,&area};
  for(unsigned i=0;i<8;i++){void* pointer=nullptr;*all[i]=map_physical_memory("fake-parent",0,4096,0,1,&pointer);if(*all[i]<0)return false;}return true;}
};
struct IO:PioneerComputeNocIO{area_id views[7]={-1,-1,-1,-1,-1,-1,-1};void* pages[7]={};bool invalid=false;
'''
tests = r'''
};
static void Reset(){for(auto& entry:maps)free(entry.second.pointer);maps.clear();mapCalls=deleteCalls=failMap=failDelete=0;parentFail=failed=false;}
int main(){unsigned cases=0;
 {IO io;assert(io.Map() && maps.size()==15);std::set<uint32_t> owned;
  for(unsigned i=0;i<PioneerNativeBfpLease::SavedWords;i++)owned.insert(PioneerNativeBfpLease::Address(i));
  const uint32_t offsets[]={0x19006000,0x19070000,0x1906e000,0x1908f000,0x1917f000,0x19312000,0x19313000};
  const unsigned sizes[]={4096,4096,8192,16384,4096,4096,4096};
  for(unsigned i=0;i<7;i++){const auto& m=maps.at(io.views[i]);assert(m.physical==io.profile.bar0_host_start+offsets[i] && m.size==sizes[i] && m.flags==unsigned(1|4|(i==6?0:2)));}
  for(uint32_t a=0;a<0x180000;a+=4){io.invalid=false;auto* p=io.Pointer(a);assert(bool(p)==bool(owned.count(a)));
   if(p){bool matched=false;for(const auto& entry:maps){const auto& m=entry.second;uintptr_t delta=reinterpret_cast<uintptr_t>(p)-reinterpret_cast<uintptr_t>(m.pointer);
     if(delta<m.size){assert(m.physical+delta==io.profile.bar0_host_start+0x19000000+a);matched=true;break;}}assert(matched && !io.invalid);}else assert(io.invalid);
  }
  for(uint32_t a:{0x6001U,0x90001U,0xffb121b0U,UINT32_MAX})assert(!io.Pointer(a));
  assert(io.Unmap() && maps.empty() && deleteCalls==15);assert(io.Unmap() && deleteCalls==15);cases++;
 }
 for(unsigned failure=9;failure<=15;failure++){Reset();IO io;failMap=failure;assert(!io.Map() && mapCalls==failure);assert(io.Unmap() && maps.empty() && deleteCalls==failure-1);cases++;}
 for(unsigned failure=1;failure<=15;failure++){Reset();IO io;assert(io.Map());failDelete=failure;assert(!io.Unmap() && failed && deleteCalls==failure && maps.size()==16-failure);cases++;}
 Reset();{IO io;parentFail=true;assert(!io.Map() && !mapCalls && !deleteCalls);cases++;}Reset();
 printf("PASS ACTUAL NativeIO VM/range methods:%u cases;all393216 aligned L1 addresses,exact3548 owned words/physical offsets/protections;7 partial native-map failures/15 fail-first disposals retain remaining views;CPU fixture teardown only,NO physical proof\n",cases);
}
'''
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-native-bfp-mapping.'))
if srca:
    prefix=prefix.replace('#include "PioneerNativeBfpLease.h"',
                          '#define PIONEER_NATIVE_SRCA_MAP_TEST_ONLY 1\n#define PIONEER_NATIVE_SRCA_PROBE_DIAGNOSTIC 1\n#define PIONEER_NATIVE_SRCA_PROBE_OWNER_TEST_ONLY 1\n#include "PioneerNativeBfpLease.h"')
    tests=tests.replace('0x1908f000', '0x19080000')
    tests=tests.replace('4096,4096,8192,16384,4096,4096,4096', '4096,4096,8192,0x28000,4096,4096,4096')
    tests=tests.replace('exact3548 owned words', 'exact4565 owned words').replace('PASS ACTUAL NativeIO', 'PASS OFFLINE SrcA NativeIO')
if affine:
    prefix=prefix.replace('#include "PioneerNativeBfpLease.h"','#define PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC 1\n#include "PioneerNativeBfpLease.h"')
    tests=tests.replace('4096,4096,8192,16384,4096,4096,4096','4096,8192,8192,32768,4096,4096,4096')
    tests=tests.replace('exact3548 owned words','exact4872 owned words').replace('PASS ACTUAL NativeIO','PASS ACTUAL fixed full-K affine NativeIO')
if band:
    prefix = prefix.replace('#include "PioneerNativeBfpLease.h"', '#include "PioneerNativeBfpBandLease.h"')
    prefix = prefix.replace('bool invalid=false;\n', 'bool invalid=false;unsigned width;explicit IO(unsigned k=5120):width(k){}\n')
    tests = tests.replace('{IO io;assert(io.Map()', 'for(unsigned width:{5120U,6144U,17408U}){IO io(width);assert(io.Map()', 1)
    tests = tests.replace('PioneerNativeBfpLease::SavedWords', 'PioneerNativeBfpBandLease::SavedWords(width)')
    tests = tests.replace('PioneerNativeBfpLease::Address(i)', 'PioneerNativeBfpBandLease::Address(width,i)')
    tests = tests.replace('4096,4096,8192,16384,4096,4096,4096', '4096,4096,8192,0xa2000,4096,4096,4096')
    tests = tests.replace('assert(io.Unmap() && maps.empty() && deleteCalls==15);assert(io.Unmap() && deleteCalls==15);cases++;',
                          'assert(io.Unmap() && maps.empty() && deleteCalls==15);assert(io.Unmap() && deleteCalls==15);cases++;Reset();')
    tests = tests.replace('printf("PASS ACTUAL NativeIO', '''for(unsigned k:{0U,32U,5119U,17409U,UINT32_MAX}){IO io(k);assert(!io.Map() && !mapCalls && !deleteCalls && !io.Pointer(0x6000) && io.invalid);cases++;Reset();}
 printf("PASS ACTUAL full-K NativeIO''')
    tests = tests.replace('all393216 aligned L1 addresses,exact3548 owned words', 'all1179648 aligned L1 addresses/across3 widths,exact50500/59972/164164 owned words')
if model:
    prefix = prefix.replace('PioneerNativeBfpBandLease.h', 'PioneerNativeModelBandLease.h')
    tests = tests.replace('PioneerNativeBfpBandLease', 'PioneerNativeModelBandLease')
    tests = tests.replace('for(unsigned width:{5120U,6144U,17408U})', 'for(unsigned width:{5120U})')
    tests = tests.replace('for(unsigned k:{0U,32U,5119U,17409U,UINT32_MAX})', 'for(unsigned k:{0U,32U,5119U,6144U,17408U,17409U,UINT32_MAX})')
    tests = tests.replace('all1179648 aligned L1 addresses/across3 widths,exact50500/59972/164164 owned words', 'all393216 aligned L1 addresses/fixed learned width,exact50500 owned words')
(folder / 'test.cpp').write_text(prefix + methods + tests)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I' + str(native.parent), '-I' + str(here.parent),
                *(['-I' + sys.argv[2]] if model else []), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
subprocess.run([str(folder / 'test')], check=True, env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
print('CPU-only actual mapping evidence:', folder)
