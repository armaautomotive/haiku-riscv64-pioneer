#!/usr/bin/env python3
"""Actual HOST observer geometry/poison loop ONLY;NO processor or owner state."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
def method(source,signature):
    assert source.count(signature)==1
    start=source.index(signature);brace=source.index('{',start);end=brace+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
rows=(here/'pioneer_qwen38_rows_image_test.cpp').read_text()
native=(here/'pioneer_q38_native_peer_test_bridge.cpp').read_text()
global_source=(here/'pioneer_q38_global_chain_instruction_test.cpp').read_text()
base='\n'.join(method(rows,s) for s in ('virtual bool PrivateObservationRow(', 'virtual bool UnusedRowValue('))
derived='\n'.join(method(native,s) for s in ('bool PrivateObservationRow(', 'bool UnusedRowValue('))
loops=[line.strip() for line in global_source.splitlines() if 'Poison();PoisonTail();for(unsigned i=0;i<4096;i++)' in line]
assert len(loops)==1 and 'PrivateObservationRow(i)' in loops[0]
program='''#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
struct NativePeerBase {'''+base+'''};
struct Observer:NativePeerBase {
 uint32_t matrixMetricObserved[16]={};bool matrixMetricAtEntry=false;
 unsigned matrixMetricNext=0,poison=0,tail=0;bool active=false;
 std::vector<uint32_t> buffers=std::vector<uint32_t>(8192);
 void Poison(){poison++;}void PoisonTail(){tail++;}
'''+derived+'\n void Prime(){'+loops[0]+'''}
};
int main(){
 NativePeerBase legacy;Observer observed;unsigned reserved=0,refused=0;
 for(unsigned row=0;row<4096;row++){
  assert(!legacy.PrivateObservationRow(row) && legacy.UnusedRowValue(row,0x7fc00000)
   && !legacy.UnusedRowValue(row,0));
  const bool expected=row>=320 && row<336;
  assert(observed.PrivateObservationRow(row)==expected);reserved+=expected;
  observed.buffers[4096+row]=0x12340000+row;
  if(expected){
   observed.matrixMetricObserved[row-320]=observed.buffers[4096+row];
   assert(!observed.UnusedRowValue(row,observed.buffers[4096+row]));refused++;
  }
 }
 assert(reserved==16);observed.matrixMetricAtEntry=true;observed.Prime();
 assert(observed.active && observed.poison==1 && observed.tail==1);
 for(unsigned row=0;row<4096;row++){
  const uint32_t expected=observed.PrivateObservationRow(row)?0x12340000+row:0x7fc00000;
  assert(observed.buffers[4096+row]==expected && observed.UnusedRowValue(row,expected)
   && !observed.UnusedRowValue(row,expected^1));refused++;
 }
 for(unsigned groups=1;groups<=16;groups++)for(unsigned row=0;row<groups*16;row++)
  assert(!observed.PrivateObservationRow(row) && observed.buffers[4096+row]==0x7fc00000);
 for(unsigned pending=1;pending<16;pending++){
  observed.matrixMetricNext=pending;
  for(unsigned row=320;row<336;row++){assert(!observed.UnusedRowValue(row,observed.matrixMetricObserved[row-320]));refused++;}
 }
 puts("PASS actual HOST observer methods+actual poison loop/exact16 reserved words320..335/no supported computed row excluded/all legacy words still poison-only/4352 unknown,changed,partial-record refusals;NO CPU products,processor/native-owner history,HOLD,model,physical or TPS proof");
 assert(refused==4352);
}
'''
with tempfile.TemporaryDirectory(prefix='pioneer-native-matrix-observer-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);source=out/'test.cpp';binary=out/'test'
    source.write_text(program)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
