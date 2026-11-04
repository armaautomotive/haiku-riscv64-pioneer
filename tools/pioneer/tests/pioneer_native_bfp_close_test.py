#!/usr/bin/env python3
"""Execute ACTUAL native FinishMmio/Free bodies against CPU-only failing endpoints."""
import os
import pathlib
import subprocess
import sys
import tempfile

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Test assertions must not be optimized away.')

here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
assert len(sys.argv)==1 or sys.argv[1:]==['--band']
native = root / ('src/add-ons/kernel/drivers/misc/pioneer_native_bfp_band_driver.cpp' if len(sys.argv)==2 else 'src/add-ons/kernel/drivers/misc/pioneer_native_bfp_driver.cpp')
source = native.read_text()
def body(start, end):
    first = source.index(start)
    return source[first:source.index(end, first)]

finish = body('static bool FinishMmio(', 'static void DisposeKnown(')
fault = body('static void Fault(', 'static void Reply(')
free = body('static status_t Free(', 'static status_t Control(')
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <map>
#define B_DEVICE_OP_CODES_END 10000
#include "BlackholePioneerMmioProbe.h"
using uint32=uint32_t;using status_t=int;constexpr int B_OK=0,B_BAD_VALUE=-1;
struct BaseCookie {};
struct NativeCookie {BaseCookie* base=nullptr;};
struct FakeIO {
 std::map<unsigned,uint32_t> card,root;
 unsigned writes=0,reads=0,unmaps=0,identityReads=0,failWrite=0,failIdentity=0;
 bool failUnmap=false,failed=false;
 int pending=0;unsigned pendingOffset=0;
 FakeIO() {
  card[0]=0xb1401e52;card[0x2c]=0x00431e52;card[4]=0x402;
  root[0]=0x20421e30;root[0x18]=0x003f0100;root[4]=0x402;
  const unsigned trial[]={0x0c,0x49,0x2000000c,0x49,0x0c,0xfffffff8};
  for(unsigned i=0;i<6;i++)card[0x10+i*4]=trial[i];
  root[0x24]=0x20010001;root[0x28]=root[0x2c]=0x49;
 }
 bool Unmap(){assert(!failed);unmaps++;if(failUnmap){failed=true;return false;}return true;}
 uint32_t Read(bool isRoot,unsigned offset) {
  assert(!failed);reads++;
  if(!isRoot && !offset && ++identityReads==failIdentity){failed=true;return 0xffffffff;}
  auto value=(isRoot?root:card)[offset];
  if(pending==(isRoot?2:1) && pendingOffset==offset){failed=true;return value^1;}
  return value;
 }
 uint32_t CardRead(unsigned o,unsigned){return Read(false,o);}
 uint32_t RootRead(unsigned o,unsigned){return Read(true,o);}
 void Write(bool isRoot,unsigned offset,uint32_t value) {
  assert(!failed);writes++;(isRoot?root:card)[offset]=value;
  if(writes==failWrite){pending=isRoot?2:1;pendingOffset=offset;}
 }
 void CardWrite(unsigned o,unsigned,uint32_t v){Write(false,o,v);}
 void RootWrite(unsigned o,unsigned,uint32_t v){Write(true,o,v);}
};
struct Lease {unsigned abandons=0;void Abandon(){abandons++;}};
struct NativeOwner {
 NativeCookie* cookie=nullptr;FakeIO io;Lease lease;tt_pioneer_mmio_transaction transaction={true,true};tt_pioneer_mmio_result mmio={};
 NativeOwner(){for(unsigned i=0;i<6;i++)mmio.bars_before[i]=i&1?0:0xc;mmio.bridge_before[0]=0x0001fff1;}
};
static bool sColdRecoveryRequired=false;static NativeOwner* owner=nullptr;
static unsigned sOpenCookies=1,probeFrees=0;static int sLock=1,lockError=0;static bool locked=false;
static int acquire_sem(int){if(lockError)return lockError;assert(!locked);locked=true;return 0;}
static void release_sem(int){assert(locked);locked=false;}
static int ProbeFree(BaseCookie* c){assert(!locked && c);probeFrees++;sOpenCookies--;delete c;return 0;}
'''
tests = r'''
int main(){unsigned cases=0;
 {NativeOwner o;assert(FinishMmio(o) && o.io.writes==11 && o.io.unmaps==1 && o.mmio.restored && !o.transaction.config_attempted);
  const auto writes=o.io.writes,reads=o.io.reads;assert(FinishMmio(o) && o.io.writes==writes && o.io.reads==reads && o.io.unmaps==1);cases++;}
 {NativeOwner o;o.transaction={};assert(FinishMmio(o) && !o.io.reads && !o.io.writes && !o.io.unmaps);cases++;}
 {NativeOwner o;o.io.failUnmap=true;assert(!FinishMmio(o) && !o.io.reads && !o.io.writes && o.io.unmaps==1 && !o.mmio.restored);cases++;}
 for(unsigned failure=1;failure<=11;failure++){NativeOwner o;o.io.failWrite=failure;assert(!FinishMmio(o) && o.io.failed && o.io.writes==failure && !o.mmio.restored && o.transaction.config_attempted);cases++;}
 for(unsigned failure=1;failure<=12;failure++){NativeOwner o;o.io.failIdentity=failure;assert(!FinishMmio(o) && o.io.failed && o.io.identityReads==failure && !o.mmio.restored);cases++;}
 assert(Free(nullptr)==B_BAD_VALUE);NativeCookie empty;assert(Free(&empty)==B_BAD_VALUE);cases++;
 for(unsigned closeMode=0;closeMode<3;closeMode++){
  auto* n=new NativeCookie;n->base=new BaseCookie;NativeOwner o;o.cookie=n;owner=&o;sOpenCookies=1;sColdRecoveryRequired=false;
  const auto probes=probeFrees;
  if(closeMode==2){lockError=-99;assert(Free(n)==-99 && !sColdRecoveryRequired && !o.lease.abandons && sOpenCookies==1);lockError=0;}
  else {if(closeMode==1){o.lease.Abandon();sColdRecoveryRequired=true;}
   assert(Free(n)==0 && sColdRecoveryRequired && sOpenCookies==0 && o.lease.abandons==closeMode+1);
   assert(owner==&o && o.cookie==n && n->base && !o.io.unmaps && !o.io.writes && !o.io.reads && probeFrees==probes);}
  // CPU fixture teardown ONLY, not production recovery or a device cleanup.
  owner=nullptr;delete n->base;delete n;cases++;
 }
 {auto* n=new NativeCookie;n->base=new BaseCookie;NativeOwner other;owner=&other;sOpenCookies=2;const auto probes=probeFrees;
  assert(Free(n)==0 && probeFrees==probes+1 && sOpenCookies==1 && owner==&other && !other.lease.abandons);owner=nullptr;cases++;}
 printf("PASS actual extracted native BFP FinishMmio/Free:%u cases;each VM/PCI/identity failure stops at FIRST observation;known End idempotent;close/quarantined close retains owner/cookie with zero IO;NO physical proof\n",cases);
}
'''
temporary = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-native-bfp-close.'))
program = temporary / 'test.cpp'
program.write_text(prefix + finish + fault + free + tests)
compiler = os.environ.get('CXX', 'c++')
subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I' + str(native.parent), str(program), '-o', str(temporary / 'test')], check=True)
environment = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
subprocess.run([str(temporary / 'test')], check=True, env=environment)
print('CPU-only actual close bodies retained:', temporary)
