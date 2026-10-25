#!/usr/bin/env python3
"""Bounded footprint geometry and real combined-image write trace, NO card."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
here = pathlib.Path(__file__).resolve().parent
image = pathlib.Path(sys.argv[1])
tree = ast.parse((here/'run_pioneer_q38_resident_output_cohort_image_tests.py').read_text())
root_pins = [ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
             and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets)]
peer_pins = [ast.literal_eval(n.iter) for n in tree.body if isinstance(n, ast.For) and isinstance(n.iter, ast.Tuple)]
assert len(root_pins) == len(peer_pins) == 1 and len(root_pins[0]) == 5 and len(peer_pins[0]) == 6
for section, size, digest in root_pins[0]:
    data = (image/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
for section, digest in peer_pins[0]:
    assert hashlib.sha256((image/'exact-companion'/(section+'.bin')).read_bytes()).hexdigest() == digest
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
source = (here/'pioneer_q38_resident_output_cohort_image_test.cpp').read_text()
entry = '\nint main(int argc,char** argv){'
assert source.count(entry) == 1
trace = r'''
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputFootprint.h"
struct FootprintFabric:OutputCohortFabric {
 using OutputCohortFabric::OutputCohortFabric;
 void Store(uint32_t at,uint32_t value)override{
  if(at<0x180000)assert(PioneerQ38ResidentOutputSnapshotContains(0,at) && !PioneerQ38ResidentOutputProgramImmutable(0,at));
  OutputCohortFabric::Store(at,value);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 OutputParts parts(0,0);FootprintFabric f(argv[1],(std::string(argv[1])+"/exact-companion").c_str(),parts);
 assert(f.Run());f.Verify();f.Inert();unsigned observed=0;
 for(unsigned w=0;w<119;w++){
  auto& p=*f.peers[w];for(const auto& item:p.memory)if(item.first<0x180000){
   assert(PioneerQ38ResidentOutputSnapshotContains(w+1,item.first) && !PioneerQ38ResidentOutputProgramImmutable(w+1,item.first));observed++;
  }
  for(unsigned row=0;row<p.RowsMachine::groups*16;row++)assert(PioneerQ38ResidentOutputSnapshotContains(w+1,0x84000+row*4));
 }
 printf("PASS actual combined ROOT write trace+ALL119 final L1 maps/%u observed runtime words/complete5120-row pipeline fits NEW snapshot without immutable program writes;NOT native/clock/initialized owner or restore-after-compute authority\n",observed);
 return 0;
}
'''
base = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
        '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined']
environment = {**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'}
with tempfile.TemporaryDirectory(prefix='pioneer-output-footprint-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'geometry'
    subprocess.run([*base, str(here/'pioneer_q38_resident_output_footprint_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(image)], check=True, env=environment)
    binary = pathlib.Path(temporary)/'trace'
    subprocess.run([*base, *flags, '-I', str(here), '-x', 'c++', '-', '-o', str(binary)],
                   input=source.split(entry)[0]+trace, text=True, check=True)
    subprocess.run([str(binary), str(image)], check=True, env=environment)
